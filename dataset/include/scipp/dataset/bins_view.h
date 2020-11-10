// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/dataset/dataset.h"

namespace scipp::dataset {

namespace bins_view_detail {
template <class T, class View> class BinsCommon {
public:
  BinsCommon(const View &var) : m_var(var) {}
  auto indices() const {
    return std::get<0>(m_var.template constituents<bucket<T>>());
  }
  auto dim() const {
    return std::get<1>(m_var.template constituents<bucket<T>>());
  }
  auto buffer() const {
    return std::get<2>(m_var.template constituents<bucket<T>>());
  }

protected:
  auto make(const View &view) const {
    return make_non_owning_bins(this->indices(), this->dim(), view);
  }

private:
  View m_var;
};

template <class T, class View> class BinsCoords : public BinsCommon<T, View> {
public:
  BinsCoords(const BinsCommon<T, View> base) : BinsCommon<T, View>(base) {}
  auto operator[](const Dim dim) const {
    return this->make(this->buffer().coords()[dim]);
  }
};

template <class T, class View> class Bins : public BinsCommon<T, View> {
public:
  using BinsCommon<T, View>::BinsCommon;
  auto data() const { return this->make(this->buffer().data()); }
  auto coords() const { return BinsCoords<T, View>(*this); }
};
} // namespace bins_view_detail

/// Return helper for accessing bin data and coords as non-owning views
///
/// Usage:
/// auto data = bins_view<DataArray>(var).data();
/// auto coord = bins_view<DataArray>(var).coords()[dim];
template <class T, class View> auto bins_view(const View &var) {
  return bins_view_detail::Bins<T, View>(var);
}

} // namespace scipp::dataset
