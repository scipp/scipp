/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#ifndef DATASET_H
#define DATASET_H

#include <vector>

#include <boost/iterator/iterator_facade.hpp>
#include <gsl/gsl_util>

#include "dimension.h"
#include "tags.h"
#include "variable.h"

class Dataset;
namespace detail {
template <class Tag> class VariableView;
template <class Tag> VariableView<Tag> getCoord(Dataset &, const Tag);
template <class Tag>
VariableView<Tag> getData(Dataset &,
                          const std::pair<const Tag, const std::string> &);
template <class Data> class Access;
} // namespace detail

template <class T> class Slice;

class Dataset {
public:
  gsl::index size() const { return m_variables.size(); }
  const Variable &operator[](const gsl::index i) const {
    return m_variables[i];
  }
  // WARNING: This returns `const Variable &` ON PURPOSE. We do not provide
  // non-const access to Variable since it could break the dataset, e.g., by
  // assigning a variable with a different shape. Nevetheless we need to
  // non-const overload to avoid compiler warnings about ambiguous overloads
  // with the std::string version.
  const Variable &operator[](const gsl::index i) { return m_variables[i]; }
  Slice<const Dataset> operator[](const std::string &name) const;
  Slice<Dataset> operator[](const std::string &name);

  auto begin() const { return m_variables.begin(); }
  auto end() const { return m_variables.end(); }

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

  template <class Tag> void erase() {
    const auto it = m_variables.begin() + findUnique(tag_id<Tag>);
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
    return m_variables[findUnique(tag_id<Tag>)].template get<Tag>();
  }

  template <class Tag> auto get(const std::string &name) const {
    return m_variables[find(tag_id<Tag>, name)].template get<Tag>();
  }

  template <class Tag> auto get() {
    return m_variables[findUnique(tag_id<Tag>)].template get<Tag>();
  }

  template <class Tag> auto get(const std::string &name) {
    return m_variables[find(tag_id<Tag>, name)].template get<Tag>();
  }

  const Dimensions &dimensions() const { return m_dimensions; }

  template <class Tag> const Dimensions &dimensions() const {
    return m_variables[findUnique(tag_id<Tag>)].dimensions();
  }

  template <class Tag>
  const Dimensions &dimensions(const std::string &name) const {
    return m_variables[find(tag_id<Tag>, name)].dimensions();
  }

  template <class Tag> const Unit &unit() const {
    return m_variables[findUnique(tag_id<Tag>)].unit();
  }

  template <class Tag> const Unit &unit(const std::string &name) const {
    return m_variables[find(tag_id<Tag>, name)].unit();
  }

  gsl::index find(const uint16_t id, const std::string &name) const;
  gsl::index findUnique(const uint16_t id) const;

  bool operator==(const Dataset &other) const;
  Dataset &operator+=(const Dataset &other);
  template <class T> Dataset &operator-=(const T &other);
  Dataset &operator*=(const Dataset &other);
  void setSlice(const Dataset &slice, const Dimension dim,
                const gsl::index index);

private:
  template <class Data> friend class detail::Access;
  // This is private such that name and dimensions of variables cannot be
  // modified in a way that would break the dataset.
  Variable &get(gsl::index i) { return m_variables[i]; }

  // The way the Python exports are written we require non-const references to
  // variables. Note that setting breaking attributes is not exported, so we
  // should be safe.
  template <class Tag>
  friend detail::VariableView<Tag> detail::getCoord(Dataset &, const Tag);
  template <class Tag>
  friend detail::VariableView<Tag>
  detail::getData(Dataset &, const std::pair<const Tag, const std::string> &);
  template <class... Tags> friend class LinearView;

  gsl::index count(const uint16_t id) const;
  gsl::index count(const uint16_t id, const std::string &name) const;
  void mergeDimensions(const Dimensions &dims, const Dim coordDim);

  // TODO These dimensions do not imply any ordering, should use another class
  // in place of `Dimensions`, which *does* imply an order.
  Dimensions m_dimensions;
  boost::container::small_vector<Variable, 4> m_variables;
};

template <class Value>
class dataset_slice_iterator
    : public boost::iterator_facade<
          dataset_slice_iterator<Value>,
          std::conditional_t<std::is_const<Value>::value, const Variable,
                             Variable>,
          boost::random_access_traversal_tag> {
public:
  dataset_slice_iterator(Value &dataset, const std::vector<gsl::index> &indices,
                         const gsl::index index)
      : m_dataset(dataset), m_indices(indices), m_index(index) {}

private:
  friend class boost::iterator_core_access;

  bool equal(const dataset_slice_iterator &other) const {
    return m_index == other.m_index;
  }
  void increment() { ++m_index; }
  std::conditional_t<std::is_const<Value>::value, const Variable, Variable> &
  dereference() const;
  void decrement() { --m_index; }
  void advance(int64_t delta) { m_index += delta; }
  int64_t distance_to(const dataset_slice_iterator &other) const {
    return other.m_index - m_index;
  }

  Value &m_dataset;
  const std::vector<gsl::index> &m_indices;
  gsl::index m_index;
};

template <class Base> class SliceMutableMixin {};

template <> class SliceMutableMixin<Slice<Dataset>> {
public:
  template <class T> Slice<Dataset> &operator-=(const T &other);

private:
  Variable &get(const gsl::index i);
  dataset_slice_iterator<Dataset> mutableBegin() const;
  dataset_slice_iterator<Dataset> mutableEnd() const;

  friend class detail::Access<Slice<Dataset>>;
  const Slice<Dataset> &base() const;
  Slice<Dataset> &base();
};

// D is either Dataset or const Dataset.
template <class D> class Slice : public SliceMutableMixin<Slice<D>> {
public:
  Slice(D &dataset, const std::string &select) : m_dataset(dataset) {
    for (gsl::index i = 0; i < dataset.size(); ++i) {
      const auto &var = dataset[i];
      if (var.isCoord() || var.name() == select)
        m_indices.push_back(i);
    }
  }

  const Dataset &dataset() const { return m_dataset; }

  gsl::index size() const { return m_indices.size(); }
  const Variable &operator[](const gsl::index i) const {
    return m_dataset[m_indices[i]];
  }

  dataset_slice_iterator<const Dataset> begin() const {
    return {dataset(), m_indices, 0};
  }
  dataset_slice_iterator<const Dataset> end() const {
    return {dataset(), m_indices, static_cast<gsl::index>(m_indices.size())};
  }

private:
  friend class SliceMutableMixin<Slice<D>>;
  D &m_dataset;
  std::vector<gsl::index> m_indices;
};

Dataset operator+(Dataset a, const Dataset &b);
Dataset operator-(Dataset a, const Dataset &b);
Dataset operator*(Dataset a, const Dataset &b);
Dataset slice(const Dataset &d, const Dimension dim, const gsl::index index);
Dataset slice(const Dataset &d, const Dimension dim, const gsl::index begin,
              const gsl::index end);
std::vector<Dataset> split(const Dataset &d, const Dim dim,
                           const std::vector<gsl::index> &indices);
Dataset concatenate(const Dataset &d1, const Dataset &d2, const Dimension dim);
// Not implemented
Dataset convert(const Dataset &d, const Dimension from, const Dimension to);
// Not verified, likely wrong in some cases
Dataset rebin(const Dataset &d, const Variable &newCoord);

Dataset sort(const Dataset &d, Tag t, const std::string &name = "");
// Note: Can provide stable_sort for sorting by multiple columns, e.g., for a
// QTableView.

Dataset filter(const Dataset &d, const Variable &select);

#endif // DATASET_H
