// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <boost/iterator/transform_iterator.hpp>

#include "scipp/dataset/dataset.h"
#include "scipp/variable/bins.h"
#include "scipp/variable/except.h"

namespace scipp::dataset {

namespace bins_view_detail {
template <class T, class View> class BinsCommon {
public:
  BinsCommon(const View &var) : m_var(var) {}
  auto indices() const { return std::get<0>(get()); }
  auto dim() const { return std::get<1>(get()); }
  auto buffer() const { return std::get<2>(get()); }

protected:
  auto make(const View &view) const {
    return make_non_owning_bins(this->indices(), this->dim(), view);
  }
  auto check_and_get_buf(const VariableConstView &var) const {
    const auto &[i, d, buf] = var.dtype() == dtype<bucket<VariableView>>
                                  ? var.constituents<bucket<VariableView>>()
                                  : var.constituents<bucket<Variable>>();
    core::expect::equals(i, this->indices());
    core::expect::equals(d, this->dim());
    return buf;
  }

private:
  auto get() const { return m_var.template constituents<bucket<T>>(); }
  View m_var;
};

template <class T, class View, class MapView>
class BinsMapView : public BinsCommon<T, View> {
  struct make_item {
    const BinsMapView *view;
    template <class Item> auto operator()(const Item &item) const {
      if (item.second.dims().contains(view->dim()))
        return std::pair(item.first, view->make(item.second));
      else
        return std::pair(item.first, copy(item.second));
    }
  };

public:
  using key_type = typename MapView::key_type;
  using mapped_type = typename MapView::mapped_type;
  BinsMapView(const BinsCommon<T, View> base, MapView mapView)
      : BinsCommon<T, View>(base), m_mapView(std::move(mapView)) {}
  scipp::index size() const noexcept { return m_mapView.size(); }
  auto operator[](const key_type &key) const {
    return this->make(m_mapView[key]);
  }
  void erase(const key_type &key) const { return m_mapView.erase(key); }
  void set(const key_type &key, const VariableConstView &var) const {
    m_mapView.set(key, this->check_and_get_buf(var));
  }
  auto begin() const noexcept {
    return boost::make_transform_iterator(m_mapView.begin(), make_item{this});
  }
  auto end() const noexcept {
    return boost::make_transform_iterator(m_mapView.end(), make_item{this});
  }
  bool contains(const key_type &key) const noexcept {
    return m_mapView.contains(key);
  }
  scipp::index count(const key_type &key) const noexcept {
    return m_mapView.count(key);
  }

private:
  MapView m_mapView;
};

template <class T, class View> class Bins : public BinsCommon<T, View> {
public:
  using BinsCommon<T, View>::BinsCommon;
  auto data() const { return this->make(this->buffer().data()); }
  void setData(const VariableConstView &var) {
    this->buffer().setData(Variable(this->check_and_get_buf(var)));
  }
  auto meta() const { return BinsMapView(*this, this->buffer().meta()); }
  auto coords() const { return BinsMapView(*this, this->buffer().coords()); }
  auto attrs() const { return BinsMapView(*this, this->buffer().attrs()); }
  auto masks() const { return BinsMapView(*this, this->buffer().masks()); }
  auto &name() const { return this->buffer().name(); }
};
} // namespace bins_view_detail

/// Return helper for accessing bin data and coords as non-owning views
///
/// Usage:
/// auto data = bins_view<DataArray>(var).data();
/// auto coord = bins_view<DataArray>(var).coords()[dim];
///
/// The returned objects are variables referencing data in `var`. They do not
/// own or share ownership of any data.
template <class T, class View> auto bins_view(const View &var) {
  return bins_view_detail::Bins<T, View>(var);
}
template <class T> auto bins_view(const Variable &var) {
  return bins_view<T>(VariableConstView(var));
}
template <class T> auto bins_view(Variable &var) {
  return bins_view<T>(VariableView(var));
}

} // namespace scipp::dataset
