// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef DATASET_H
#define DATASET_H

#include <vector>

#include <boost/iterator/transform_iterator.hpp>

#include "dimension.h"
#include "tags.h"
#include "variable.h"

namespace scipp::core {

class ConstDatasetSlice;
class DatasetSlice;

/// Dataset is a set of Variables, identified with a unique (tag, name)
/// identifier.
class Dataset {
public:
  using Item = std::tuple<std::string, Tag, Variable>;
  using ItemConstProxy =
      std::tuple<const std::string &, Tag, ConstVariableSlice>;
  using ItemProxy = std::tuple<const std::string &, Tag, VariableSlice>;

  static ItemConstProxy itemProxy(const Item &item) {
    auto & [ name, tag, data ] = item;
    return {name, tag, ConstVariableSlice(data)};
  }
  static ItemProxy itemProxy(Item &item) {
    auto & [ name, tag, data ] = item;
    return {name, tag, VariableSlice(data)};
  }

private:
  // Helper lambdas for creating iterators.
  static constexpr auto makeConstSlice = [](const Item &item) {
    return itemProxy(item);
  };
  static constexpr auto makeSlice = [](Item &item) { return itemProxy(item); };

public:
  Dataset() = default;
  Dataset(std::vector<Item> items);
  // Allowing implicit construction from views facilitates calling functions
  // that do not explicitly support views. It is open for discussion whether
  // this is a good idea or not.
  Dataset(const ConstDatasetSlice &view);

  scipp::index size() const { return m_variables.size(); }

  // ATTENTION: It is really important to delete any function returning a
  // (Const)VariableSlice or (Const)DatasetSlice for rvalue Dataset. Otherwise
  // the resulting slice will point to free'ed memory.
  ConstVariableSlice operator[](const scipp::index i) const && = delete;
  auto operator[](const scipp::index i) const & {
    return itemProxy(m_variables[i]);
  }
  VariableSlice operator[](const scipp::index i) && = delete;
  auto operator[](const scipp::index i) & { return itemProxy(m_variables[i]); }

  ConstDatasetSlice subset(const std::string &) const && = delete;
  ConstDatasetSlice subset(const Tag, const std::string &) const && = delete;
  ConstDatasetSlice subset(const std::string &name) const &;
  ConstDatasetSlice subset(const Tag tag, const std::string &name) const &;
  DatasetSlice subset(const std::string &) && = delete;
  DatasetSlice subset(const Tag, const std::string &) && = delete;
  DatasetSlice subset(const std::string &name) &;
  DatasetSlice subset(const Tag tag, const std::string &name) &;

  ConstDatasetSlice operator()(const Dim dim, const scipp::index begin,
                               const scipp::index end = -1) const && = delete;
  ConstDatasetSlice operator()(const Dim dim, const scipp::index begin,
                               const scipp::index end = -1) const &;
  Dataset operator()(const Dim dim, const scipp::index begin,
                     const scipp::index end = -1) &&;
  DatasetSlice operator()(const Dim dim, const scipp::index begin,
                          const scipp::index end = -1) &;
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

  void insert(Item item);
  void insert(ItemConstProxy item) {
    const auto & [ name, tag, var ] = item;
    insert(tag, name, var);
  }
  void insert(ItemProxy item) {
    const auto & [ name, tag, var ] = item;
    insert(tag, name, var);
  }
  template <class T> void insert(const std::string &newName, const T &slice) {
    // Note the lack of atomicity
    for (const auto & [ name, tag, var ] : slice) {
      if (tag.isCoord()) {
        if (!contains(tag, name)) {
          throw std::runtime_error(
              "Cannot provide new coordinate variables via subset");
        }
      } else {
        // TODO Should we insert named coordinates? This would be required,
        // e.g., for event data.
        insert(tag, newName, var);
      }
    }
  }
  void insert(const Tag tag, Variable variable) {
    insert(Item{"", tag, std::move(variable)});
  }
  void insert(const Tag tag, const std::string &name, Variable variable) {
    insert(Item{name, tag, std::move(variable)});
  }

  template <class Tag, class... Args>
  void insert(const Tag tag, const Dimensions &dimensions, Args &&... args) {
    insert(tag, makeVariable<typename Tag::type>(std::move(dimensions),
                                                 std::forward<Args>(args)...));
  }

  template <class Tag, class... Args>
  void insert(const Tag tag, const std::string &name,
              const Dimensions &dimensions, Args &&... args) {
    insert(tag, name,
           makeVariable<typename Tag::type>(std::move(dimensions),
                                            std::forward<Args>(args)...));
  }

  template <class Tag, class T>
  void insert(const Tag tag, const Dimensions &dimensions,
              std::initializer_list<T> values) {
    insert(tag,
           makeVariable<typename Tag::type>(std::move(dimensions), values));
  }

  template <class Tag, class T>
  void insert(const Tag tag, const std::string &name,
              const Dimensions &dimensions, std::initializer_list<T> values) {
    Variable a(std::move(dimensions), values);
    insert(tag, name, std::move(a));
  }

  // Insert variants with custom type
  template <class T, class Tag, class... Args>
  void insert(const Tag tag, const Dimensions &dimensions, Args &&... args) {
    auto a =
        makeVariable<T>(std::move(dimensions), std::forward<Args>(args)...);
    insert(tag, std::move(a));
  }

  template <class T, class Tag, class... Args>
  void insert(const Tag tag, const std::string &name,
              const Dimensions &dimensions, Args &&... args) {
    auto a =
        makeVariable<T>(std::move(dimensions), std::forward<Args>(args)...);
    insert(tag, name, std::move(a));
  }

  template <class T, class Tag, class T2>
  void insert(const Tag tag, const Dimensions &dimensions,
              std::initializer_list<T2> values) {
    auto a = makeVariable<T>(std::move(dimensions), values);
    insert(tag, std::move(a));
  }

  template <class T, class Tag, class T2>
  void insert(const Tag tag, const std::string &name,
              const Dimensions &dimensions, std::initializer_list<T2> values) {
    auto a = makeVariable<T, T2>(std::move(dimensions), values);
    insert(tag, name, std::move(a));
  }

  bool contains(const Tag tag, const std::string &name = "") const;
  Item erase(const Tag tag, const std::string &name = "");

  // TODO This should probably also include a copy of all or all relevant
  // coordinates.
  Dataset extract(const std::string &name);

  void merge(const Dataset &other);

  template <class TagT>
  auto get(const TagT,
           const std::string &name = std::string{}) const && = delete;
  template <class TagT>
  auto get(const TagT tag, const std::string &name = std::string{}) const & {
    return span<typename TagT::type>(tag, name);
  }

  template <class TagT>
  auto get(const TagT, const std::string &name = std::string{}) && = delete;
  template <class TagT>
  auto get(const TagT tag, const std::string &name = std::string{}) & {
    return span<typename TagT::type>(tag, name);
  }

  template <class T>
  auto span(const Tag,
            const std::string &name = std::string{}) const && = delete;
  template <class T>
  auto span(const Tag tag, const std::string &name = std::string{}) const & {
    return std::get<Variable>(m_variables[find(tag, name)]).template span<T>();
  }

  template <class T>
  auto span(const Tag, const std::string &name = std::string{}) && = delete;
  template <class T>
  auto span(const Tag tag, const std::string &name = std::string{}) & {
    return std::get<Variable>(m_variables[find(tag, name)]).template span<T>();
  }

  // Currently `Dimensions` does not allocate memory so we could return by
  // value instead of disabling this, but this way leaves more room for
  // changes, I think.
  const Dimensions &dimensions() const && = delete;
  const Dimensions &dimensions() const & { return m_dimensions; }

  bool operator==(const Dataset &other) const;
  bool operator==(const ConstDatasetSlice &other) const;
  bool operator!=(const Dataset &other) const;
  bool operator!=(const ConstDatasetSlice &other) const;
  Dataset operator-() const;
  Dataset &operator+=(const Dataset &other);
  Dataset &operator+=(const ConstDatasetSlice &other);
  Dataset &operator+=(const Variable &other);
  Dataset &operator+=(const double value);
  Dataset &operator-=(const Dataset &other);
  Dataset &operator-=(const ConstDatasetSlice &other);
  Dataset &operator-=(const Variable &other);
  Dataset &operator-=(const double value);
  Dataset &operator*=(const Dataset &other);
  Dataset &operator*=(const ConstDatasetSlice &other);
  Dataset &operator*=(const double value);
  Dataset &operator*=(const Variable &other);
  Dataset &operator/=(const Dataset &other);
  Dataset &operator/=(const ConstDatasetSlice &other);
  Dataset &operator/=(const Variable &other);
  Dataset &operator/=(const double value);

private:
  scipp::index find(const Tag tag, const std::string &name) const;
  void mergeDimensions(const Dimensions &dims, const Dim coordDim);

  // TODO These dimensions do not imply any ordering, should use another class
  // in place of `Dimensions`, which *does* imply an order.
  Dimensions m_dimensions;
  boost::container::small_vector<Item, 4> m_variables;
};

template <class T> scipp::index count(const T &dataset, const Tag tag) {
  scipp::index n = 0;
  for (const auto & [ n, t, var ] : dataset)
    if (t == tag)
      ++n;
  return n;
}

template <class T>
scipp::index count(const T &dataset, const Tag tag, const std::string &name) {
  scipp::index n = 0;
  for (const auto & [ n, t, var ] : dataset)
    if (t == tag && n == name)
      ++n;
  return n;
}

// T can be Dataset or Slice.
template <class T>
scipp::index find(const T &dataset, const Tag tag, const std::string &name) {
  for (scipp::index i = 0; i < dataset.size(); ++i) {
    const auto & [ n, t, var ] = dataset[i];
    if (t == tag && n == name)
      return i;
  }
  throw except::VariableNotFoundError(dataset, tag, name);
}

namespace detail {
template <class VarSlice>
auto makeSlice(
    VarSlice slice,
    const std::vector<std::tuple<Dim, scipp::index, scipp::index, scipp::index>>
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
    auto operator()(const scipp::index i) const {
      const auto & [ name, tag, var ] = m_view.m_dataset[i];
      return std::tuple<const std::string &, Tag, ConstVariableSlice>(
          name, tag, detail::makeSlice(var, m_view.m_slices));
    }
    const ConstDatasetSlice &m_view;
  };

  friend struct IterAccess;

protected:
  std::vector<scipp::index> makeIndices(const ConstDatasetSlice &base,
                                        const std::string &select) const {
    std::vector<scipp::index> indices;
    bool foundData = false;
    for (const auto i : base.m_indices) {
      const auto & [ name, tag, var ] = base.m_dataset[i];
      static_cast<void>(var);
      // TODO Should we also keep attributes? Probably yes?
      if (tag.isCoord() || name == select) {
        foundData |= tag.isData();
        indices.push_back(i);
      }
    }
    if (!foundData)
      throw except::VariableNotFoundError(base, select);
    return indices;
  }
  std::vector<scipp::index> makeIndices(const ConstDatasetSlice &base,
                                        const Tag selectTag,
                                        const std::string &selectName) const {
    std::vector<scipp::index> indices;
    bool foundData = false;
    for (const auto i : base.m_indices) {
      const auto & [ name, tag, var ] = base.m_dataset[i];
      static_cast<void>(var);
      if (tag.isCoord() || (tag == selectTag && name == selectName)) {
        foundData |= tag.isData();
        indices.push_back(i);
      }
    }
    if (!foundData)
      throw except::VariableNotFoundError(base, selectTag, selectName);
    return indices;
  }

public:
  ConstDatasetSlice(const Dataset &dataset) : m_dataset(dataset) {
    // Select everything.
    for (scipp::index i = 0; i < dataset.size(); ++i)
      m_indices.push_back(i);
  }

  ConstDatasetSlice(const Dataset &dataset, std::vector<scipp::index> indices)
      : m_dataset(dataset), m_indices(std::move(indices)) {}

  ConstDatasetSlice(const Dataset &dataset, const std::string &select)
      : ConstDatasetSlice(dataset, makeIndices(dataset, select)) {}

  ConstDatasetSlice(const Dataset &dataset, const Tag selectTag,
                    const std::string &selectName)
      : ConstDatasetSlice(dataset,
                          makeIndices(dataset, selectTag, selectName)) {}

  ConstDatasetSlice operator()(const Dim dim, const scipp::index begin,
                               const scipp::index end = -1) const {
    return makeSubslice(*this, dim, begin, end);
  }

  ConstDatasetSlice subset(const std::string &name) const & {
    ConstDatasetSlice ret(m_dataset, makeIndices(*this, name));
    ret.m_slices = m_slices;
    return ret;
  }
  ConstDatasetSlice subset(const Tag tag, const std::string &name) const & {
    ConstDatasetSlice ret(m_dataset, makeIndices(*this, tag, name));
    ret.m_slices = m_slices;
    return ret;
  }

  bool contains(const Tag tag, const std::string &name = "") const;

  Dimensions dimensions() const {
    Dimensions dims;
    for (scipp::index i = 0; i < m_dataset.dimensions().count(); ++i) {
      const Dim dim = m_dataset.dimensions().label(i);
      scipp::index size = m_dataset.dimensions().size(i);
      for (const auto &slice : m_slices)
        if (std::get<Dim>(slice) == dim) {
          if (std::get<3>(slice) == -1)
            size = -1;
          else
            size = std::get<3>(slice) - std::get<2>(slice);
        }
      if (size != -1)
        dims.add(dim, size);
    }
    return dims;
  }

  scipp::index size() const { return m_indices.size(); }

  auto operator[](const scipp::index i) const {
    const auto & [ name, tag, var ] = m_dataset[m_indices[i]];
    return std::tuple<const std::string &, Tag, ConstVariableSlice>(
        name, tag, detail::makeSlice(var, m_slices));
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
  std::vector<scipp::index> m_indices;
  // TODO Use a struct here. Tuple contains <Dim, size, begin, end>.
  std::vector<std::tuple<Dim, scipp::index, scipp::index, scipp::index>>
      m_slices;

  template <class D>
  D makeSubslice(D slice, const Dim dim, const scipp::index begin,
                 const scipp::index end) const {
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
        if (coordDimension[std::get<Tag>(slice.m_dataset[*it]).value()] == dim)
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
    auto operator()(const scipp::index i) const {
      const auto & [ name, tag, var ] = m_view.m_mutableDataset[i];
      return std::tuple<const std::string &, Tag, VariableSlice>(
          name, tag, detail::makeSlice(var, m_view.m_slices));
    }
    const DatasetSlice &m_view;
  };

  friend struct IterAccess;

public:
  DatasetSlice(Dataset &dataset)
      : ConstDatasetSlice(dataset), m_mutableDataset(dataset) {}
  DatasetSlice(Dataset &dataset, std::vector<scipp::index> indices)
      : ConstDatasetSlice(dataset, std::move(indices)),
        m_mutableDataset(dataset) {}
  DatasetSlice(Dataset &dataset, const std::string &select)
      : ConstDatasetSlice(dataset, select), m_mutableDataset(dataset) {}
  DatasetSlice(Dataset &dataset, const Tag selectTag,
               const std::string &selectName)
      : ConstDatasetSlice(dataset, selectTag, selectName),
        m_mutableDataset(dataset) {}

  using ConstDatasetSlice::operator[];
  auto operator[](const scipp::index i) const {
    const auto & [ name, tag, var ] = m_mutableDataset[m_indices[i]];
    return std::tuple<const std::string &, Tag, VariableSlice>(
        name, tag, detail::makeSlice(var, m_slices));
  }

  DatasetSlice operator()(const Dim dim, const scipp::index begin,
                          const scipp::index end = -1) const {
    return makeSubslice(*this, dim, begin, end);
  }

  DatasetSlice subset(const std::string &name) const & {
    DatasetSlice ret(m_mutableDataset, makeIndices(*this, name));
    ret.m_slices = m_slices;
    return ret;
  }
  DatasetSlice subset(const Tag tag, const std::string &name) const & {
    DatasetSlice ret(m_mutableDataset, makeIndices(*this, tag, name));
    ret.m_slices = m_slices;
    return ret;
  }
  template <class T> void insert(const std::string &name, const T &slice) {
    m_mutableDataset.insert(name, slice);
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
  DatasetSlice operator+=(const Variable &other) const;
  DatasetSlice operator+=(const double value) const;
  DatasetSlice operator-=(const Dataset &other) const;
  DatasetSlice operator-=(const ConstDatasetSlice &other) const;
  DatasetSlice operator-=(const Variable &other) const;
  DatasetSlice operator-=(const double value) const;
  DatasetSlice operator*=(const Dataset &other) const;
  DatasetSlice operator*=(const ConstDatasetSlice &other) const;
  DatasetSlice operator*=(const Variable &other) const;
  DatasetSlice operator*=(const double value) const;
  DatasetSlice operator/=(const Dataset &other) const;
  DatasetSlice operator/=(const ConstDatasetSlice &other) const;
  DatasetSlice operator/=(const Variable &other) const;
  DatasetSlice operator/=(const double value) const;

  VariableSlice operator()(const Tag tag, const std::string &name = "") const;

private:
  Dataset &m_mutableDataset;
};

Dataset operator+(Dataset a, const Dataset &b);
Dataset operator+(Dataset a, const ConstDatasetSlice &b);
Dataset operator+(Dataset a, const Variable &b);
Dataset operator+(Dataset a, const double b);
Dataset operator+(const double a, Dataset b);
Dataset operator-(Dataset a, const Dataset &b);
Dataset operator-(Dataset a, const ConstDatasetSlice &b);
Dataset operator-(Dataset a, const Variable &b);
Dataset operator-(Dataset a, const double b);
Dataset operator-(const double a, Dataset b);
Dataset operator*(Dataset a, const Dataset &b);
Dataset operator*(Dataset a, const ConstDatasetSlice &b);
Dataset operator*(Dataset a, const Variable &b);
Dataset operator*(Dataset a, const double b);
Dataset operator*(const double a, Dataset b);
Dataset operator/(Dataset a, const double b);
Dataset operator/(Dataset a, const ConstDatasetSlice &b);
Dataset operator/(Dataset a, const Variable &b);
Dataset operator/(Dataset a, const double b);
std::vector<Dataset> split(const Dataset &d, const Dim dim,
                           const std::vector<scipp::index> &indices);
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
Dataset reverse(const Dataset &d, const Dim dim);

} // namespace scipp::core

#endif // DATASET_H
