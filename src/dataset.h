/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#ifndef DATASET_H
#define DATASET_H

#include <vector>

#include <boost/iterator/transform_iterator.hpp>
#include <gsl/gsl_util>

#include "dimension.h"
#include "tags.h"
#include "variable.h"

class ConstDatasetSlice;
class DatasetSlice;

/// Dataset is a set of Variables, identified with a unique (tag, name)
/// identifier.
class Dataset {
private:
  // Helper lambdas for creating iterators.
  static constexpr auto makeConstSlice = [](const Variable &var) {
    return ConstVariableSlice(var);
  };
  static constexpr auto makeSlice = [](Variable &var) {
    return VariableSlice(var);
  };

public:
  Dataset() = default;
  // Allowing implicit construction from views facilitates calling functions
  // that do not explicitly support views. It is open for discussion whether
  // this is a good idea or not.
  Dataset(const ConstDatasetSlice &view);

  gsl::index size() const { return m_variables.size(); }

  // ATTENTION: It is really important to delete any function returning a
  // (Const)VariableSlice or (Const)DatasetSlice for rvalue Dataset. Otherwise
  // the resulting slice will point to free'ed memory.
  ConstVariableSlice operator[](const gsl::index i) const && = delete;
  ConstVariableSlice operator[](const gsl::index i) const & {
    return ConstVariableSlice{m_variables[i]};
  }
  VariableSlice operator[](const gsl::index i) && = delete;
  VariableSlice operator[](const gsl::index i) & {
    return VariableSlice{m_variables[i]};
  }
  ConstDatasetSlice subset(const std::string &name) const && = delete;
  ConstDatasetSlice subset(const std::string &name) const &;
  DatasetSlice subset(const std::string &name) && = delete;
  DatasetSlice subset(const std::string &name) &;
  ConstDatasetSlice operator()(const Dim dim, const gsl::index begin,
                               const gsl::index end = -1) const && = delete;
  ConstDatasetSlice operator()(const Dim dim, const gsl::index begin,
                               const gsl::index end = -1) const &;
  Dataset operator()(const Dim dim, const gsl::index begin,
                     const gsl::index end = -1) &&;
  DatasetSlice operator()(const Dim dim, const gsl::index begin,
                          const gsl::index end = -1) &;
  ConstVariableSlice
  operator()(const Tag tag,
             const std::string &name = std::string{}) const && = delete;
  ConstVariableSlice
  operator()(const Tag tag, const std::string &name = std::string{}) const &;
  VariableSlice operator()(const Tag tag,
                           const std::string &name = std::string{}) && = delete;
  VariableSlice operator()(const Tag tag,
                           const std::string &name = std::string{}) &;

  // The iterators (and in fact all other public accessors to variables in
  // Dataset) return *views* and *not* a `Variable &`. This is necessary to
  // ensure that the dataset cannot be broken by modifying the name of a
  // variable (which could lead to duplicate names in the dataset) or the
  // dimensions of a variable (which could lead to inconsistent dimension
  // extents in the dataset). By exposing variables via views we are limiting
  // modifications to those that cannot break guarantees given by dataset.
  auto begin() const && = delete;
  auto begin() const & {
    return boost::make_transform_iterator(m_variables.begin(), makeConstSlice);
  }
  auto end() const && = delete;
  auto end() const & {
    return boost::make_transform_iterator(m_variables.end(), makeConstSlice);
  }
  auto begin() && = delete;
  auto begin() & {
    return boost::make_transform_iterator(m_variables.begin(), makeSlice);
  }
  auto end() && = delete;
  auto end() & {
    return boost::make_transform_iterator(m_variables.end(), makeSlice);
  }

  void insert(Variable variable);
  void insert(const Tag tag, Variable variable) {
    variable.setTag(tag);
    variable.setName("");
    insert(std::move(variable));
  }
  void insert(const Tag tag, const std::string &name, Variable variable) {
    variable.setTag(tag);
    variable.setName(name);
    insert(std::move(variable));
  }

  template <class Tag, class... Args>
  void insert(const Tag tag, const Dimensions &dimensions, Args &&... args) {
    static_assert(is_coord<Tag>, "Non-coordinate variable must have a name.");
    Variable a(tag, std::move(dimensions), std::forward<Args>(args)...);
    insert(std::move(a));
  }

  template <class Tag, class... Args>
  void insert(const Tag tag, const std::string &name,
              const Dimensions &dimensions, Args &&... args) {
    static_assert(!is_coord<Tag>, "Coordinate variable cannot have a name.");
    Variable a(tag, std::move(dimensions), std::forward<Args>(args)...);
    a.setName(name);
    insert(std::move(a));
  }

  template <class Tag, class T>
  void insert(const Tag tag, const Dimensions &dimensions,
              std::initializer_list<T> values) {
    static_assert(is_coord<Tag>, "Non-coordinate variable must have a name.");
    Variable a(tag, std::move(dimensions), values);
    insert(std::move(a));
  }

  template <class Tag, class T>
  void insert(const Tag tag, const std::string &name,
              const Dimensions &dimensions, std::initializer_list<T> values) {
    static_assert(!is_coord<Tag>, "Coordinate variable cannot have a name.");
    Variable a(tag, std::move(dimensions), values);
    a.setName(name);
    insert(std::move(a));
  }

  // Insert variants with custom type
  template <class T, class Tag, class... Args>
  void insert(const Tag tag, const Dimensions &dimensions, Args &&... args) {
    static_assert(is_coord<Tag>, "Non-coordinate variable must have a name.");
    auto a = makeVariable<T>(tag, std::move(dimensions),
                             std::forward<Args>(args)...);
    insert(std::move(a));
  }

  template <class T, class Tag, class... Args>
  void insert(const Tag tag, const std::string &name,
              const Dimensions &dimensions, Args &&... args) {
    static_assert(!is_coord<Tag>, "Coordinate variable cannot have a name.");
    auto a = makeVariable<T>(tag, std::move(dimensions),
                             std::forward<Args>(args)...);
    a.setName(name);
    insert(std::move(a));
  }

  template <class T, class Tag, class T2>
  void insert(const Tag tag, const Dimensions &dimensions,
              std::initializer_list<T2> values) {
    static_assert(is_coord<Tag>, "Non-coordinate variable must have a name.");
    auto a = makeVariable<T>(tag, std::move(dimensions), values);
    insert(std::move(a));
  }

  template <class T, class Tag, class T2>
  void insert(const Tag tag, const std::string &name,
              const Dimensions &dimensions, std::initializer_list<T2> values) {
    static_assert(!is_coord<Tag>, "Coordinate variable cannot have a name.");
    auto a = makeVariable<T>(tag, std::move(dimensions), values);
    a.setName(name);
    insert(std::move(a));
  }

  bool contains(const Tag tag, const std::string &name = "") const;
  Variable erase(const Tag tag, const std::string &name = "");

  // TODO This should probably also include a copy of all or all relevant
  // coordinates.
  Dataset extract(const std::string &name);

  void merge(const Dataset &other);

  template <class TagT>
  auto get(const TagT,
           const std::string &name = std::string{}) const && = delete;
  template <class TagT>
  auto get(const TagT tag, const std::string &name = std::string{}) const & {
    return m_variables[find(tag, name)].get(tag);
  }

  template <class TagT>
  auto get(const TagT, const std::string &name = std::string{}) && = delete;
  template <class TagT>
  auto get(const TagT tag, const std::string &name = std::string{}) & {
    return m_variables[find(tag, name)].get(tag);
  }

  template <class T>
  auto span(const Tag,
            const std::string &name = std::string{}) const && = delete;
  template <class T>
  auto span(const Tag tag, const std::string &name = std::string{}) const & {
    return m_variables[find(tag, name)].template span<T>();
  }

  template <class T>
  auto span(const Tag, const std::string &name = std::string{}) && = delete;
  template <class T>
  auto span(const Tag tag, const std::string &name = std::string{}) & {
    return m_variables[find(tag, name)].template span<T>();
  }

  // Currently `Dimensions` does not allocate memory so we could return by value
  // instead of disabling this, but this way leaves more room for changes, I
  // think.
  const Dimensions &dimensions() const && = delete;
  const Dimensions &dimensions() const & { return m_dimensions; }

  bool operator==(const Dataset &other) const;
  bool operator==(const ConstDatasetSlice &other) const;
  bool operator!=(const Dataset &other) const;
  bool operator!=(const ConstDatasetSlice &other) const;
  Dataset operator-() const;
  Dataset &operator+=(const Dataset &other);
  Dataset &operator+=(const ConstDatasetSlice &other);
  Dataset &operator+=(const double value);
  Dataset &operator-=(const Dataset &other);
  Dataset &operator-=(const ConstDatasetSlice &other);
  Dataset &operator-=(const double value);
  Dataset &operator*=(const Dataset &other);
  Dataset &operator*=(const ConstDatasetSlice &other);
  Dataset &operator*=(const double value);

private:
  gsl::index find(const Tag tag, const std::string &name) const;
  void mergeDimensions(const Dimensions &dims, const Dim coordDim);

  // TODO These dimensions do not imply any ordering, should use another class
  // in place of `Dimensions`, which *does* imply an order.
  Dimensions m_dimensions;
  boost::container::small_vector<Variable, 4> m_variables;
};

template <class T> gsl::index count(const T &dataset, const Tag tag) {
  gsl::index n = 0;
  for (const auto &item : dataset)
    if (item.tag() == tag)
      ++n;
  return n;
}

template <class T>
gsl::index count(const T &dataset, const Tag tag, const std::string &name) {
  gsl::index n = 0;
  for (const auto item : dataset)
    if (item.tag() == tag && item.name() == name)
      ++n;
  return n;
}

// T can be Dataset or Slice.
template <class T>
gsl::index find(const T &dataset, const Tag tag, const std::string &name) {
  for (gsl::index i = 0; i < dataset.size(); ++i)
    if (dataset[i].tag() == tag && dataset[i].name() == name)
      return i;
  throw dataset::except::VariableNotFoundError(dataset, tag, name);
}

namespace detail {
template <class VarSlice>
auto makeSlice(
    VarSlice slice,
    const std::vector<std::tuple<Dim, gsl::index, gsl::index, gsl::index>>
        &slices) {
  for (const auto &s : slices) {
    const auto dim = std::get<Dim>(s);
    if (slice.dimensions().contains(dim)) {
      if (slice.dimensions()[dim] == std::get<1>(s))
        slice = slice(dim, std::get<2>(s), std::get<3>(s));
      else
        slice = slice(dim, std::get<2>(s), std::get<3>(s) + 1);
    }
  }
  return slice;
}
} // namespace detail

/// Non-mutable view into (a subset of) a Dataset. It can be a subset both in
/// terms of containing only a subset of the variables, as well as containing
/// only a certain subspace (slice) of the dimension extents.
class ConstDatasetSlice {
private:
  struct IterAccess {
    auto operator()(const gsl::index i) const {
      return detail::makeSlice(m_view.m_dataset[i], m_view.m_slices);
    }
    const ConstDatasetSlice &m_view;
  };

  friend struct IterAccess;

public:
  ConstDatasetSlice(const Dataset &dataset) : m_dataset(dataset) {
    // Select everything.
    for (gsl::index i = 0; i < dataset.size(); ++i)
      m_indices.push_back(i);
  }

  ConstDatasetSlice(const Dataset &dataset, const std::string &select)
      : m_dataset(dataset) {
    for (gsl::index i = 0; i < dataset.size(); ++i) {
      const auto &var = dataset[i];
      if (var.isCoord() || var.name() == select)
        m_indices.push_back(i);
    }
  }

  ConstDatasetSlice operator()(const Dim dim, const gsl::index begin,
                               const gsl::index end = -1) const {
    return makeSubslice(*this, dim, begin, end);
  }

  bool contains(const Tag tag, const std::string &name = "") const;

  std::map<Dim, gsl::index> dimensions() const {
    std::map<Dim, gsl::index> dims;
    for (gsl::index i = 0; i < m_dataset.dimensions().count(); ++i) {
      const Dim dim = m_dataset.dimensions().label(i);
      gsl::index size = m_dataset.dimensions().size(i);
      for (const auto &slice : m_slices)
        if (std::get<Dim>(slice) == dim) {
          if (std::get<3>(slice) == -1)
            size = -1;
          else
            size = std::get<3>(slice) - std::get<2>(slice);
        }
      if (size != -1)
        dims[dim] = size;
    }
    return dims;
  }

  gsl::index size() const { return m_indices.size(); }

  ConstVariableSlice operator[](const gsl::index i) const {
    return detail::makeSlice(m_dataset[m_indices[i]], m_slices);
  }

  auto begin() const {
    return boost::make_transform_iterator(m_indices.begin(), IterAccess{*this});
  }
  auto end() const {
    return boost::make_transform_iterator(m_indices.end(), IterAccess{*this});
  }

  bool operator==(const Dataset &other) const;
  bool operator==(const ConstDatasetSlice &other) const;
  bool operator!=(const Dataset &other) const;
  bool operator!=(const ConstDatasetSlice &other) const;

  Dataset operator-() const;

  ConstVariableSlice operator()(const Tag tag,
                                const std::string &name = "") const;

protected:
  const Dataset &m_dataset;
  std::vector<gsl::index> m_indices;
  // TODO Use a struct here. Tuple contains <Dim, size, begin, end>.
  std::vector<std::tuple<Dim, gsl::index, gsl::index, gsl::index>> m_slices;

  template <class D>
  D makeSubslice(D slice, const Dim dim, const gsl::index begin,
                 const gsl::index end) const {
    const auto size = m_dataset.dimensions()[dim];
    for (auto &s : slice.m_slices) {
      if (std::get<Dim>(s) == dim) {
        std::get<2>(s) = begin;
        std::get<3>(s) = end;
        return slice;
      }
    }
    slice.m_slices.emplace_back(dim, size, begin, end);
    if (end == -1) {
      for (auto it = slice.m_indices.begin(); it != slice.m_indices.end();) {
        // TODO Should all coordinates with matching dimension be removed, or
        // only dimension-coordinates?
        if (coordDimension[slice.m_dataset[*it].tag().value()] == dim)
          it = slice.m_indices.erase(it);
        else
          ++it;
      }
    }
    return slice;
  }
};

/// Mutable view into (a subset of) a Dataset.
class DatasetSlice : public ConstDatasetSlice {
private:
  struct IterAccess {
    auto operator()(const gsl::index i) const {
      return detail::makeSlice(m_view.m_mutableDataset[i], m_view.m_slices);
    }
    const DatasetSlice &m_view;
  };

  friend struct IterAccess;

public:
  DatasetSlice(Dataset &dataset)
      : ConstDatasetSlice(dataset), m_mutableDataset(dataset) {}
  DatasetSlice(Dataset &dataset, const std::string &select)
      : ConstDatasetSlice(dataset, select), m_mutableDataset(dataset) {}

  using ConstDatasetSlice::operator[];
  VariableSlice operator[](const gsl::index i) const {
    return detail::makeSlice(m_mutableDataset[m_indices[i]], m_slices);
  }

  DatasetSlice operator()(const Dim dim, const gsl::index begin,
                          const gsl::index end = -1) const {
    return makeSubslice(*this, dim, begin, end);
  }

  using ConstDatasetSlice::begin;
  using ConstDatasetSlice::end;

  auto begin() const {
    return boost::make_transform_iterator(m_indices.begin(), IterAccess{*this});
  }
  auto end() const {
    return boost::make_transform_iterator(m_indices.end(), IterAccess{*this});
  }

  // Returning void to avoid potentially returning references to temporaries.
  DatasetSlice assign(const Dataset &other) const;
  DatasetSlice assign(const ConstDatasetSlice &other) const;
  DatasetSlice operator+=(const Dataset &other) const;
  DatasetSlice operator+=(const ConstDatasetSlice &other) const;
  DatasetSlice operator+=(const double value) const;
  DatasetSlice operator-=(const Dataset &other) const;
  DatasetSlice operator-=(const ConstDatasetSlice &other) const;
  DatasetSlice operator-=(const double value) const;
  DatasetSlice operator*=(const Dataset &other) const;
  DatasetSlice operator*=(const ConstDatasetSlice &other) const;
  DatasetSlice operator*=(const double value) const;

  VariableSlice operator()(const Tag tag, const std::string &name = "") const;

private:
  Dataset &m_mutableDataset;
};

Dataset operator+(Dataset a, const Dataset &b);
Dataset operator+(Dataset a, const ConstDatasetSlice &b);
Dataset operator+(Dataset a, const double b);
Dataset operator+(const double a, Dataset b);
Dataset operator-(Dataset a, const Dataset &b);
Dataset operator-(Dataset a, const ConstDatasetSlice &b);
Dataset operator-(Dataset a, const double b);
Dataset operator-(const double a, Dataset b);
Dataset operator*(Dataset a, const Dataset &b);
Dataset operator*(Dataset a, const ConstDatasetSlice &b);
Dataset operator*(Dataset a, const double b);
Dataset operator*(const double a, Dataset b);
std::vector<Dataset> split(const Dataset &d, const Dim dim,
                           const std::vector<gsl::index> &indices);
Dataset concatenate(const Dataset &d1, const Dataset &d2, const Dim dim);
// Not verified, likely wrong in some cases
Dataset rebin(const Dataset &d, const Variable &newCoord);
Dataset histogram(const Dataset &d, const Variable &coord);

Dataset sort(const Dataset &d, const Tag t, const std::string &name = "");
// Note: Can provide stable_sort for sorting by multiple columns, e.g., for a
// QTableView.

Dataset filter(const Dataset &d, const Variable &select);
Dataset sum(const Dataset &d, const Dim dim);
Dataset mean(const Dataset &d, const Dim dim);
Dataset integrate(const Dataset &d, const Dim dim);

#endif // DATASET_H
