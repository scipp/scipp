// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <boost/iterator/transform_iterator.hpp>

#include "scipp/core/dimensions.h"
#include "scipp/core/element_array_view.h"
#include "scipp/core/except.h"
#include "scipp/variable/except.h"
#include "scipp/variable/variable.tcc"

namespace scipp::variable {
struct bucket_base {
  using range_type = std::pair<scipp::index, scipp::index>;
};
template <class T> struct bucket : bucket_base {
  using element_type = typename T::view_type;
  using const_element_type = typename T::const_view_type;
};
} // namespace scipp::variable

namespace scipp::core {

template <class T>
class bucket_array_view
    : public ElementArrayView<const variable::bucket_base::range_type> {
public:
  using value_type = const typename variable::bucket_base::range_type;
  using base = ElementArrayView<const value_type>;

  bucket_array_view(value_type *buckets, const scipp::index offset,
                    const Dimensions &iterDims, const Dimensions &dataDims,
                    const Dim dim, T &buffer)
      : ElementArrayView<value_type>(buckets, offset, iterDims, dataDims),
        m_transform{dim, &buffer} {}

  auto begin() const {
    return boost::make_transform_iterator(base::begin(), m_transform);
  }
  auto end() const {
    return boost::make_transform_iterator(base::end(), m_transform);
  }

private:
  struct make_item {
    auto operator()(typename bucket_array_view::value_type &range) const {
      return m_buffer->slice({m_dim, range.first, range.second});
    }
    Dim m_dim;
    T *m_buffer;
  };
  make_item m_transform;
};

template <class T>
class ElementArrayView<variable::bucket<T>> : public bucket_array_view<T> {
  using bucket_array_view<T>::bucket_array_view;
};

template <class T>
class ElementArrayView<const variable::bucket<T>>
    : public bucket_array_view<const T> {
  using bucket_array_view<const T>::bucket_array_view;
};

} // namespace scipp::core

namespace scipp::variable {

template <class T> class DataModel<bucket<T>> : public VariableConcept {
public:
  using value_type = typename bucket<T>::range_type;

  DataModel(const Dimensions &dimensions, element_array<value_type> buckets,
            const Dim dim, T buffer)
      : VariableConcept(dimensions), m_buckets(std::move(buckets)), m_dim(dim),
        m_buffer(std::move(buffer)) {
    if (!m_buffer.dims().contains(m_dim))
      throw except::DimensionError("Buffer must contain bucket slicing dim.");
  }

  VariableConceptHandle clone() const override {
    return std::make_unique<DataModel>(this->dims(), m_buckets, m_dim,
                                       m_buffer);
  }

  bool operator==(const DataModel &other) const noexcept {
    return dims() == other.dims() && equal(m_buckets, other.m_buckets) &&
           m_dim == other.m_dim && m_buffer == other.m_buffer;
  }
  bool operator!=(const DataModel &other) const noexcept {
    return !(*this == other);
  }

  VariableConceptHandle
  makeDefaultFromParent(const Dimensions &dims) const override {
    return std::make_unique<DataModel>(dims,
                                       element_array<value_type>(dims.volume()),
                                       m_dim, T(m_buffer, m_buffer.dims()));
  }

  constexpr DType dtype() const noexcept override {
    return scipp::dtype<bucket<T>>; // or bucket<T>::view_type?
  }

  constexpr bool hasVariances() const noexcept override { return false; }
  void setVariances(Variable &&) override {
    if (!core::canHaveVariances<T>())
      throw except::VariancesError("This data type cannot have variances.");
  }

  bool equals(const VariableConstView &,
              const VariableConstView &) const override {
    return false;
  }
  void copy(const VariableConstView &, const VariableView &) const override {}
  void assign(const VariableConcept &) override {}

  ElementArrayView<bucket<T>> values() {
    return {m_buckets.data(), 0, dims(), dims(), m_dim, m_buffer};
  }
  ElementArrayView<const bucket<T>> values() const {
    return {m_buckets.data(), 0, dims(), dims(), m_dim, m_buffer};
  }

private:
  element_array<value_type> m_buckets;
  Dim m_dim;
  T m_buffer;
};

} // namespace scipp::variable
