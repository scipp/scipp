// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/string.h"
#include "scipp/variable/bins.h"
#include "scipp/variable/except.h"

namespace scipp::dataset {

namespace bins_view_detail {
template <class T> class BinsCommon {
public:
  explicit BinsCommon(Variable var) : m_var(std::move(var)) {}
  auto indices() const { return std::get<0>(get()); }
  auto dim() const { return std::get<1>(get()); }
  auto &buffer() const { return m_var.bin_buffer<T>(); }
  auto &buffer() { return m_var.bin_buffer<T>(); }

protected:
  auto make(const variable::Variable &view) const {
    return make_bins_no_validate(this->indices(), this->dim(), view);
  }
  auto check_and_get_buf(const Variable &var) const {
    const auto &[i, d, buf] = var.constituents<Variable>();
    core::expect::equals(i, this->indices());
    core::expect::equals(d, this->dim());
    return buf;
  }
  [[nodiscard]] const Variable &get_var() const noexcept { return m_var; }

private:
  auto get() const { return m_var.constituents<T>(); }
  Variable m_var;
};

template <class T, class MapGetter> class BinsMapView : public BinsCommon<T> {
  struct make_value {
    const BinsMapView *view;
    template <class Value> auto operator()(const Value &value) const {
      if (value.dims().contains(view->dim()))
        return view->make(value);
      else
        return copy(value);
    }
  };
  struct make_item {
    const BinsMapView *view;
    template <class Item> auto operator()(const Item &item) const {
      return std::pair(item.first, make_value{view}(item.second));
    }
  };
  using MapView =
      std::decay_t<decltype(std::declval<MapGetter>()(std::declval<T>()))>;

public:
  using key_type = typename MapView::key_type;
  using mapped_type = typename MapView::mapped_type;
  BinsMapView(BinsCommon<T> base, MapGetter map)
      : BinsCommon<T>(std::move(base)), m_map(std::move(map)) {}
  scipp::index size() const noexcept { return mapView().size(); }
  auto operator[](const key_type &key) const {
    return this->make(mapView()[key]);
  }
  void erase(const key_type &key) { return mapView().erase(key); }
  auto extract(const key_type &key) {
    return this->make(mapView().extract(key));
  }
  void set(const key_type &key, const Variable &var) {
    mapView().set(key, this->check_and_get_buf(var));
  }
  auto begin() const noexcept {
    return mapView().begin().transform(make_item{this});
  }
  auto end() const noexcept {
    return mapView().end().transform(make_item{this});
  }
  auto keys_begin() const noexcept { return mapView().keys_begin(); }
  auto keys_end() const noexcept { return mapView().keys_end(); }
  auto values_begin() const noexcept {
    return mapView().values_begin().transform(make_value{this});
  }
  auto values_end() const noexcept {
    return mapView().values_end().transform(make_value{this});
  }
  auto items_begin() const noexcept { return begin(); }
  auto items_end() const noexcept { return end(); }
  bool contains(const key_type &key) const noexcept {
    return mapView().contains(key);
  }
  scipp::index count(const key_type &key) const noexcept {
    return mapView().count(key);
  }
  void set_aligned(const key_type &key, const bool aligned) {
    mapView().set_aligned(key, aligned);
  }
  bool is_edges(const key_type &, std::optional<Dim>) const noexcept {
    return false; // event-coordinates are never edges
  }
  bool operator==(const BinsMapView &other) const noexcept {
    return mapView() == other.mapView();
  }
  bool operator!=(const BinsMapView &other) const noexcept {
    return mapView() != other.mapView();
  }

  friend std::string dict_keys_to_string(const BinsMapView &view) {
    return dict_keys_to_string(view.mapView());
  }
  friend std::string to_string(const BinsMapView &view) {
    return to_string(view.mapView());
  }
  friend BinsMapView copy(const BinsMapView &view) {
    return BinsMapView{BinsCommon<T>{copy(view.get_var())}, view.m_map};
  }

private:
  decltype(auto) mapView() const { return m_map(this->buffer()); }
  decltype(auto) mapView() { return m_map(this->buffer()); }
  MapGetter m_map;
};

template <class T> class Bins : public BinsCommon<T> {
public:
  using BinsCommon<T>::BinsCommon;
  auto data() const { return this->make(this->buffer().data()); }
  // TODO how to handle const-ness?
  void setData(const Variable &var) {
    this->buffer().setData(this->check_and_get_buf(var));
  }
  auto coords() const { return BinsMapView(*this, get_coords); }
  auto masks() const { return BinsMapView(*this, get_masks); }
  auto &name() const { return this->buffer().name(); }
  auto drop_coords(const std::span<const Dim> coord_names) const {
    auto result = *this;
    for (const auto &name : coord_names)
      result.coords().erase(name);
  }
  auto drop_masks(const std::span<const std::string> mask_names) const {
    auto result = *this;
    for (const auto &name : mask_names)
      result.masks().erase(name);
  }
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
template <class T> auto bins_view(Variable var) {
  return bins_view_detail::Bins<T>(std::move(var));
}

} // namespace scipp::dataset
