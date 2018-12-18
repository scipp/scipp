/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#ifndef DATASET_H
#define DATASET_H

#include <vector>

#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <gsl/gsl_util>

#include "dimension.h"
#include "tags.h"
#include "variable.h"

class ConstDatasetSlice;
class DatasetSlice;

class Dataset {
private:
  static constexpr auto makeConstSlice = [](const Variable &var) {
    return ConstVariableSlice(var);
  };
  static constexpr auto makeSlice = [](Variable &var) {
    return VariableSlice(var);
  };

public:
  Dataset() = default;
  // Allowing implicit construction from views facilities calling functions that
  // do not explicitly support views. It is open for discussion whether this is
  // a good idea or not.
  Dataset(const ConstDatasetSlice &view);

  gsl::index size() const { return m_variables.size(); }
  ConstVariableSlice operator[](const gsl::index i) const {
    return ConstVariableSlice{m_variables[i]};
  }
  VariableSlice operator[](const gsl::index i) {
    return VariableSlice{m_variables[i]};
  }
  ConstDatasetSlice operator[](const std::string &name) const;
  DatasetSlice operator[](const std::string &name);
  ConstDatasetSlice operator()(const Dim dim, const gsl::index begin,
                               const gsl::index end = -1) const;
  DatasetSlice operator()(const Dim dim, const gsl::index begin,
                          const gsl::index end = -1);
  VariableSlice operator()(const Tag tag, const std::string &name = "");

  auto begin() const {
    return boost::make_transform_iterator(m_variables.begin(), makeConstSlice);
  }
  auto end() const {
    return boost::make_transform_iterator(m_variables.end(), makeConstSlice);
  }
  auto begin() {
    return boost::make_transform_iterator(m_variables.begin(), makeSlice);
  }
  auto end() {
    return boost::make_transform_iterator(m_variables.end(), makeSlice);
  }

  void insert(Variable variable);

  template <class Tag, class... Args>
  void insert(const Dimensions &dimensions, Args &&... args) {
    static_assert(is_coord<Tag>, "Non-coordinate variable must have a name.");
    auto a =
        makeVariable<Tag>(std::move(dimensions), std::forward<Args>(args)...);
    insert(std::move(a));
  }

  template <class Tag, class... Args>
  void insert(const std::string &name, const Dimensions &dimensions,
              Args &&... args) {
    static_assert(!is_coord<Tag>, "Coordinate variable cannot have a name.");
    auto a =
        makeVariable<Tag>(std::move(dimensions), std::forward<Args>(args)...);
    a.setName(name);
    insert(std::move(a));
  }

  template <class Tag, class T>
  void insert(const Dimensions &dimensions, std::initializer_list<T> values) {
    static_assert(is_coord<Tag>, "Non-coordinate variable must have a name.");
    auto a = makeVariable<Tag>(std::move(dimensions), values);
    insert(std::move(a));
  }

  template <class Tag, class T>
  void insert(const std::string &name, const Dimensions &dimensions,
              std::initializer_list<T> values) {
    static_assert(!is_coord<Tag>, "Coordinate variable cannot have a name.");
    auto a = makeVariable<Tag>(std::move(dimensions), values);
    a.setName(name);
    insert(std::move(a));
  }

  bool contains(const Tag tag, const std::string &name = "") const;
  void erase(const Tag tag, const std::string &name = "");

  template <class Tag> void erase() {
    const auto it = m_variables.begin() + findUnique(Tag{});
    const auto dims = it->dimensions();
    m_variables.erase(it);
    for (const auto dim : dims.labels()) {
      bool found = false;
      for (const auto &var : m_variables)
        if (var.dimensions().contains(dim))
          found = true;
      if (!found)
        m_dimensions.erase(dim);
    }
  }

  Dataset extract(const std::string &name);

  void merge(const Dataset &other) {
    for (const auto &var : other)
      insert(var);
  }

  template <class Tag> auto get() const {
    return m_variables[findUnique(Tag{})].template get<Tag>();
  }

  template <class Tag> auto get(const std::string &name) const {
    return m_variables[find(Tag{}, name)].template get<Tag>();
  }

  template <class Tag> auto get() {
    return m_variables[findUnique(Tag{})].template get<Tag>();
  }

  template <class Tag> auto get(const std::string &name) {
    return m_variables[find(Tag{}, name)].template get<Tag>();
  }

  const Dimensions &dimensions() const { return m_dimensions; }

  template <class Tag> const Dimensions &dimensions() const {
    return m_variables[findUnique(Tag{})].dimensions();
  }

  template <class Tag>
  const Dimensions &dimensions(const std::string &name) const {
    return m_variables[find(Tag{}, name)].dimensions();
  }

  template <class Tag> const Unit &unit() const {
    return m_variables[findUnique(Tag{})].unit();
  }

  template <class Tag> const Unit &unit(const std::string &name) const {
    return m_variables[find(Tag{}, name)].unit();
  }

  gsl::index find(const Tag tag, const std::string &name) const;
  gsl::index findUnique(const Tag tag) const;

  bool operator==(const Dataset &other) const;
  Dataset &operator+=(const Dataset &other);
  Dataset &operator+=(const ConstDatasetSlice &other);
  Dataset &operator-=(const Dataset &other);
  Dataset &operator-=(const ConstDatasetSlice &other);
  Dataset &operator*=(const Dataset &other);
  Dataset &operator*=(const ConstDatasetSlice &other);

private:
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

template <class Value>
class dataset_slice_iterator
    : public boost::iterator_facade<
          dataset_slice_iterator<Value>,
          std::conditional_t<std::is_const<Value>::value, ConstVariableSlice,
                             VariableSlice>,
          boost::random_access_traversal_tag,
          std::conditional_t<std::is_const<Value>::value, ConstVariableSlice,
                             VariableSlice>> {
public:
  dataset_slice_iterator(
      Value &dataset, const std::vector<gsl::index> &indices,
      const std::vector<std::tuple<Dim, gsl::index, gsl::index>> &slices,
      const gsl::index index)
      : m_dataset(dataset), m_indices(indices), m_slices(slices),
        m_index(index) {}

private:
  friend class boost::iterator_core_access;

  bool equal(const dataset_slice_iterator &other) const {
    return m_index == other.m_index;
  }
  void increment() { ++m_index; }
  std::conditional_t<std::is_const<Value>::value, ConstVariableSlice,
                     VariableSlice>
  dereference() const;
  void decrement() { --m_index; }
  void advance(int64_t delta) { m_index += delta; }
  int64_t distance_to(const dataset_slice_iterator &other) const {
    return other.m_index - m_index;
  }

  // TODO Just reference Slice instead of all its contents here.
  Value &m_dataset;
  const std::vector<gsl::index> &m_indices;
  const std::vector<std::tuple<Dim, gsl::index, gsl::index>> &m_slices;
  gsl::index m_index;
};

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
    const std::vector<std::tuple<Dim, gsl::index, gsl::index>> &slices) {
  for (const auto &s : slices) {
    if (slice.dimensions().contains(std::get<Dim>(s)))
      slice = slice(std::get<0>(s), std::get<1>(s), std::get<2>(s));
  }
  return slice;
}
} // namespace detail

class ConstDatasetSlice {
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

  const Dataset &dataset() const { return m_dataset; }

  std::vector<std::tuple<Dim, gsl::index>> dimensions() const {
    std::vector<std::tuple<Dim, gsl::index>> dims;
    for (gsl::index i = 0; i < m_dataset.dimensions().count(); ++i) {
      const Dim dim = m_dataset.dimensions().label(i);
      gsl::index size = m_dataset.dimensions().size(i);
      for (const auto &slice : m_slices)
        if (std::get<Dim>(slice) == dim) {
          if (std::get<2>(slice) == -1)
            size = -1;
          else
            size = std::get<2>(slice) - std::get<1>(slice);
        }
      dims.emplace_back(dim, size);
    }
    return dims;
  }

  gsl::index size() const { return m_indices.size(); }

  ConstVariableSlice operator[](const gsl::index i) const {
    return detail::makeSlice(m_dataset[m_indices[i]], m_slices);
  }

  dataset_slice_iterator<const Dataset> begin() const {
    return {dataset(), m_indices, m_slices, 0};
  }
  dataset_slice_iterator<const Dataset> end() const {
    return {dataset(), m_indices, m_slices,
            static_cast<gsl::index>(m_indices.size())};
  }

  bool operator==(const ConstDatasetSlice &other) const {
    if ((m_dataset == other.m_dataset) && (m_indices == other.m_indices) &&
        (m_slices == other.m_slices))
      return true;
    return std::equal(begin(), end(), other.begin(), other.end());
  }

protected:
  const Dataset &m_dataset;
  std::vector<gsl::index> m_indices;
  std::vector<std::tuple<Dim, gsl::index, gsl::index>> m_slices;

  template <class D>
  D makeSubslice(D slice, const Dim dim, const gsl::index begin,
                 const gsl::index end) const {
    if (!m_dataset.dimensions().contains(dim) &&
        (begin != 0 || !(end == 1 || end == -1)))
      throw dataset::except::DimensionNotFoundError(m_dataset.dimensions(),
                                                    dim);
    for (auto &s : slice.m_slices) {
      if (std::get<Dim>(s) == dim) {
        std::get<1>(s) = begin;
        std::get<2>(s) = end;
        return slice;
      }
    }
    slice.m_slices.emplace_back(dim, begin, end);
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

class DatasetSlice : public ConstDatasetSlice {
public:
  DatasetSlice(Dataset &dataset)
      : ConstDatasetSlice(dataset), m_mutableDataset(dataset) {}
  DatasetSlice(Dataset &dataset, const std::string &select)
      : ConstDatasetSlice(dataset, select), m_mutableDataset(dataset) {}

  using ConstDatasetSlice::begin;
  using ConstDatasetSlice::end;
  dataset_slice_iterator<Dataset> begin();
  dataset_slice_iterator<Dataset> end();

  using ConstDatasetSlice::operator[];
  VariableSlice operator[](const gsl::index i) {
    return detail::makeSlice(m_mutableDataset[m_indices[i]], m_slices);
  }

  DatasetSlice operator()(const Dim dim, const gsl::index begin,
                          const gsl::index end = -1) const {
    return makeSubslice(*this, dim, begin, end);
  }

  DatasetSlice &assign(const Dataset &other);
  DatasetSlice &assign(const ConstDatasetSlice &other);
  DatasetSlice &operator+=(const Dataset &other);
  DatasetSlice &operator+=(const ConstDatasetSlice &other);
  DatasetSlice &operator-=(const Dataset &other);
  DatasetSlice &operator-=(const ConstDatasetSlice &other);
  DatasetSlice &operator*=(const Dataset &other);
  DatasetSlice &operator*=(const ConstDatasetSlice &other);

  VariableSlice operator()(const Tag tag, const std::string &name = "");

private:
  Dataset &m_mutableDataset;
};

Dataset operator+(Dataset a, const Dataset &b);
Dataset operator-(Dataset a, const Dataset &b);
Dataset operator*(Dataset a, const Dataset &b);
std::vector<Dataset> split(const Dataset &d, const Dim dim,
                           const std::vector<gsl::index> &indices);
Dataset concatenate(const Dataset &d1, const Dataset &d2, const Dimension dim);
// Not implemented
Dataset convert(const Dataset &d, const Dimension from, const Dimension to);
// Not verified, likely wrong in some cases
Dataset rebin(const Dataset &d, const Variable &newCoord);

Dataset sort(const Dataset &d, const Tag t, const std::string &name = "");
// Note: Can provide stable_sort for sorting by multiple columns, e.g., for a
// QTableView.

Dataset filter(const Dataset &d, const Variable &select);
Dataset sum(const Dataset &d, const Dim dim);
Dataset mean(const Dataset &d, const Dim dim);

#endif // DATASET_H
