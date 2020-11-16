// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/dataset/dataset.h"
#include "scipp/variable/bins.h"

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

private:
  auto get() const { return m_var.template constituents<bucket<T>>(); }
  View m_var;
};

template <class T, class View> class BinsCoords : public BinsCommon<T, View> {
public:
  BinsCoords(const BinsCommon<T, View> base) : BinsCommon<T, View>(base) {}
  auto operator[](const Dim dim) const {
    return this->make(this->buffer().coords()[dim]);
  }
};

template <class T, class View> class BinsMasks : public BinsCommon<T, View> {
public:
  BinsMasks(const BinsCommon<T, View> base) : BinsCommon<T, View>(base) {}
  auto operator[](const std::string &name) const {
    return this->make(this->buffer().masks()[name]);
  }
};

template <class T, class View> class Bins : public BinsCommon<T, View> {
public:
  using BinsCommon<T, View>::BinsCommon;
  auto data() const { return this->make(this->buffer().data()); }
  auto coords() const { return BinsCoords<T, View>(*this); }
  auto masks() const { return BinsMasks<T, View>(*this); }
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
