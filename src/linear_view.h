/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#ifndef LINEAR_VIEW_H
#define LINEAR_VIEW_H

#include "range/v3/view/zip.hpp"

#include "dataset.h"
#include "traits.h"

template <class... Tags> struct AccessHelper {
  static void push_back(std::array<Dimensions *, sizeof...(Tags)> &dimensions,
                        std::tuple<Vector<typename Tags::type> *...> &data,
                        const std::tuple<typename Tags::type...> &value);
};

template <class Tag1> struct AccessHelper<Tag1> {
  static void push_back(std::array<Dimensions *, 1> &dimensions,
                        std::tuple<Vector<typename Tag1::type> *> &data,
                        const std::tuple<typename Tag1::type> &value) {
    std::get<0>(data)->push_back(std::get<0>(value));
    dimensions[0]->resize(0, dimensions[0]->size(0) + 1);
  }
};

template <class Tag1, class Tag2> struct AccessHelper<Tag1, Tag2> {
  static void push_back(
      std::array<Dimensions *, 2> &dimensions,
      std::tuple<Vector<typename Tag1::type> *, Vector<typename Tag2::type> *>
          &data,
      const std::tuple<typename Tag1::type, typename Tag2::type> &value) {
    std::get<0>(data)->push_back(std::get<0>(value));
    std::get<1>(data)->push_back(std::get<1>(value));
    dimensions[0]->resize(0, dimensions[0]->size(0) + 1);
    dimensions[1]->resize(0, dimensions[1]->size(0) + 1);
  }
};

template <class... Tags> class LinearView {
public:
  using value_type = std::tuple<typename Tags::type...>;

  LinearView(detail::MaybeConstDataset<Tags...> &dataset) {
    // As long as we do not support passing names, duplicate tags are not
    // supported, so this check should be enough.
    if (sizeof...(Tags) != dataset.size())
      throw std::runtime_error("LinearView must be constructed based on "
                               "*all* variables in a dataset.");
    // TODO Probably we can also support 0-dimensional variables that are not
    // touched?
    for (const auto &var : dataset)
      if (var.dimensions().count() != 1)
        throw std::runtime_error("LinearView supports only datasets where "
                                 "all variables are 1-dimensional.");
    if (dataset.dimensions().count() != 1)
      throw std::runtime_error(
          "LinearView supports only 1-dimensional datasets.");

    m_dimensions = {&dataset(Tags{}).m_mutableVariable->mutableDimensions()...};
    m_data = std::make_tuple(
        &dataset(Tags{})
             .m_mutableVariable->template cast<typename Tags::type>()...);
  }

  template <size_t... Is> auto makeView(std::index_sequence<Is...>) {
    return ranges::view::zip(*std::get<Is>(m_data)...);
  }

  auto begin() {
    return makeView(std::make_index_sequence<sizeof...(Tags)>{}).begin();
  }
  auto end() {
    return makeView(std::make_index_sequence<sizeof...(Tags)>{}).end();
  }

  void push_back(const std::tuple<typename Tags::type...> &value) {
    AccessHelper<Tags...>::push_back(m_dimensions, m_data, value);
  }

private:
  std::array<Dimensions *, sizeof...(Tags)> m_dimensions;
  std::tuple<Vector<typename Tags::type> *...> m_data;
};

template <class... Tags>
void swap(typename LinearView<Tags...>::Item &a,
          typename LinearView<Tags...>::Item &b) noexcept {
  a.swap(b);
}

#endif // LINEAR_VIEW_H
