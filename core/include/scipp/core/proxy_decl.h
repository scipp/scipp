// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Dimitar Tasev
#ifndef SCIPP_CORE_PROXY_DECL_H
#define SCIPP_CORE_PROXY_DECL_H

#include <boost/iterator/transform_iterator.hpp>

#include "scipp/core/except.h"
#include "scipp/core/slice.h"
#include "scipp/core/variable.h"
#include "scipp/units/unit.h"

namespace scipp::core {

namespace detail {
using slice_list =
    boost::container::small_vector<std::pair<Slice, scipp::index>, 2>;

template <class Var> auto makeSlice(Var &var, const slice_list &slices) {
  std::conditional_t<std::is_const_v<Var>, VariableConstProxy, VariableProxy>
      slice(var);
  for (const auto [params, extent] : slices) {
    if (slice.dims().contains(params.dim())) {
      const auto new_end = params.end() + slice.dims()[params.dim()] - extent;
      const auto pointSlice = (new_end == -1);
      if (pointSlice) {
        slice = slice.slice(Slice{params.dim(), params.begin()});
      } else {
        slice = slice.slice(Slice{params.dim(), params.begin(), new_end});
      }
    }
  }
  return slice;
}

static constexpr auto make_key_value = [](auto &&view) {
  using In = decltype(view);
  using View =
      std::conditional_t<std::is_rvalue_reference_v<In>, std::decay_t<In>, In>;
  return std::pair<const std::string &, View>(
      view.name(), std::forward<decltype(view)>(view));
};

static constexpr auto make_key = [](auto &&view) -> decltype(auto) {
  using T = std::decay_t<decltype(view)>;
  if constexpr (std::is_base_of_v<DataConstProxy, T>)
    return view.name();
  else
    return view.first;
};

static constexpr auto make_value = [](auto &&view) -> decltype(auto) {
  return view.second;
};

} // namespace detail

namespace ProxyId {
class Attrs;
class Coords;
class Labels;
class Masks;
} // namespace ProxyId
template <class Id, class Key> class ConstProxy;
template <class Base> class MutableProxy;

/// Proxy for accessing coordinates of const Dataset and DataConstProxy.
using CoordsConstProxy = ConstProxy<ProxyId::Coords, Dim>;
/// Proxy for accessing coordinates of Dataset and DataProxy.
using CoordsProxy = MutableProxy<CoordsConstProxy>;
/// Proxy for accessing labels of const Dataset and DataConstProxy.
using LabelsConstProxy = ConstProxy<ProxyId::Labels, std::string>;
/// Proxy for accessing labels of Dataset and DataProxy.
using LabelsProxy = MutableProxy<LabelsConstProxy>;
/// Proxy for accessing attributes of const Dataset and DataConstProxy.
using AttrsConstProxy = ConstProxy<ProxyId::Attrs, std::string>;
/// Proxy for accessing attributes of Dataset and DataProxy.
using AttrsProxy = MutableProxy<AttrsConstProxy>;
/// Proxy for accessing masks of const Dataset and DataConstProxy
using MasksConstProxy = ConstProxy<ProxyId::Masks, std::string>;
/// Proxy for accessing masks of Dataset and DataProxy
using MasksProxy = MutableProxy<MasksConstProxy>;

/// Common functionality for other const-proxy classes.
template <class Id, class Key> class ConstProxy {
private:
  struct make_item {
    const ConstProxy *proxy;
    template <class T> auto operator()(const T &item) const {
      return std::pair<Key, VariableConstProxy>(
          item.first, detail::makeSlice(*item.second.first, proxy->slices()));
    }
  };

public:
  using key_type = Key;
  using mapped_type = Variable;

  ConstProxy(
      std::unordered_map<Key, std::pair<const Variable *, Variable *>> &&items,
      const detail::slice_list &slices = {})
      : m_items(std::move(items)), m_slices(slices) {
    // TODO This is very similar to the code in makeProxyItems(), provided that
    // we can give a good definion of the `dims` argument (roughly the space
    // spanned by all coords, excluding the dimensions that are sliced away).
    // Remove any items for a non-range sliced dimension. Identified via the
    // item in case of coords, or via the inner dimension in case of labels and
    // attributes.
    for (const auto &s : m_slices) {
      const auto slice = s.first;
      if (!slice.isRange()) { // The slice represents a point not a range.
                              // Dimension removed.
        for (auto it = m_items.begin(); it != m_items.end();) {
          auto erase = [slice](const auto it2) {
            if constexpr (std::is_same_v<Key, Dim>)
              return (it2->first == slice.dim());
            else
              return !it2->second.first->dims().empty() &&
                     (it2->second.first->dims().inner() == slice.dim());
          };
          if (erase(it))
            it = m_items.erase(it);
          else
            ++it;
        }
      }
    }
  }

  /// Return the number of coordinates in the proxy.
  index size() const noexcept { return scipp::size(m_items); }
  /// Return true if there are 0 coordinates in the proxy.
  [[nodiscard]] bool empty() const noexcept { return size() == 0; }

  /// Returns whether a given key is present in the proxy.
  bool contains(const Key &k) const {
    return m_items.find(k) != m_items.cend();
  }

  /// Return a const proxy to the coordinate for given dimension.
  VariableConstProxy operator[](const Key key) const {
    expect::contains(*this, key);
    return detail::makeSlice(*m_items.at(key).first, m_slices);
  }

  auto find(const Key k) const && = delete;
  auto find(const Key k) const &noexcept {
    return boost::make_transform_iterator(m_items.find(k), make_item{this});
  }

  auto begin() const && = delete;
  /// Return const iterator to the beginning of all items.
  auto begin() const &noexcept {
    return boost::make_transform_iterator(m_items.begin(), make_item{this});
  }
  auto end() const && = delete;
  /// Return const iterator to the end of all items.
  auto end() const &noexcept {
    return boost::make_transform_iterator(m_items.end(), make_item{this});
  }

  auto items_begin() const && = delete;
  /// Return const iterator to the beginning of all items.
  auto items_begin() const &noexcept { return begin(); }
  auto items_end() const && = delete;
  /// Return const iterator to the end of all items.
  auto items_end() const &noexcept { return end(); }

  auto keys_begin() const && = delete;
  /// Return const iterator to the beginning of all keys.
  auto keys_begin() const &noexcept {
    return boost::make_transform_iterator(begin(), detail::make_key);
  }
  auto keys_end() const && = delete;
  /// Return const iterator to the end of all keys.
  auto keys_end() const &noexcept {
    return boost::make_transform_iterator(end(), detail::make_key);
  }

  auto values_begin() const && = delete;
  /// Return const iterator to the beginning of all values.
  auto values_begin() const &noexcept {
    return boost::make_transform_iterator(begin(), detail::make_value);
  }
  auto values_end() const && = delete;
  /// Return const iterator to the end of all values.
  auto values_end() const &noexcept {
    return boost::make_transform_iterator(end(), detail::make_value);
  }

  ConstProxy slice(const Slice slice1) const {
    auto slices = m_slices;
    if constexpr (std::is_same_v<Key, Dim>) {
      const auto &coord = *m_items.at(slice1.dim()).first;
      slices.emplace_back(slice1, coord.dims()[slice1.dim()]);
    } else {
      throw std::runtime_error("TODO");
    }
    auto items = m_items;
    return ConstProxy(std::move(items), slices);
  }

  ConstProxy slice(const Slice slice1, const Slice slice2) const {
    return slice(slice1).slice(slice2);
  }
  ConstProxy slice(const Slice slice1, const Slice slice2,
                   const Slice slice3) const {
    return slice(slice1, slice2).slice(slice3);
  }

  bool operator==(const ConstProxy &other) const {
    if (size() != other.size())
      return false;
    for (const auto &[name, data] : *this) {
      try {
        if (data != other[name])
          return false;
      } catch (except::NotFoundError &) {
        return false;
      }
    }
    return true;
  }
  bool operator!=(const ConstProxy &other) const { return !operator==(other); }

  const auto &items() const noexcept { return m_items; }
  const auto &slices() const noexcept { return m_slices; }

protected:
  std::unordered_map<Key, std::pair<const Variable *, Variable *>> m_items;
  detail::slice_list m_slices;
};

SCIPP_CORE_EXPORT Variable masks_merge_if_contains(const MasksConstProxy &masks,
                                                   const Dim dim);

SCIPP_CORE_EXPORT Variable
masks_merge_if_contained(const MasksConstProxy &masks, const Dimensions &dims);

} // namespace scipp::core
#endif // SCIPP_CORE_PROXY_DECL_H
