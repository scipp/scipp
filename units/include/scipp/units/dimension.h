// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_UNITS_DIMENSION_H
#define SCIPP_UNITS_DIMENSION_H

#include <cstdint>

namespace scipp::units {

#define SCIPP_UNITS_DECLARE_DIMENSIONS(...)                                    \
  enum class Dim : uint16_t { __VA_ARGS__, Invalid };                          \
                                                                               \
  namespace detail {                                                           \
  constexpr const char *names = #__VA_ARGS__;                                  \
  constexpr auto ndim = static_cast<size_t>(Dim::Invalid);                     \
  }                                                                            \
                                                                               \
  std::string to_string(const Dim dim);

#define SCIPP_UNITS_DEFINE_DIMENSIONS(MODULE)                                  \
  namespace MODULE {                                                           \
  namespace {                                                                  \
  constexpr std::string_view names(detail::names);                             \
                                                                               \
  constexpr auto get_pos(const size_t index) {                                 \
    constexpr const char *sep = ", ";                                          \
    size_t pos = 0;                                                            \
    for (size_t i = 0; i < index; ++i)                                         \
      pos = names.find(sep, pos) + 2;                                          \
    return pos;                                                                \
  }                                                                            \
                                                                               \
  constexpr auto make_dim_name(const size_t index) {                           \
    const auto pos = get_pos(index);                                           \
    const auto count = get_pos(index + 1) - pos - 2;                           \
    return names.substr(pos, count);                                           \
  }                                                                            \
  template <size_t... Is>                                                      \
  constexpr auto make_dim_names(std::index_sequence<Is...>) {                  \
    return std::array{make_dim_name(Is)...};                                   \
  }                                                                            \
                                                                               \
  constexpr auto dim_names =                                                   \
      make_dim_names(std::make_index_sequence<detail::ndim>());                \
  }                                                                            \
                                                                               \
  std::string to_string(const Dim dim) {                                       \
    if (dim == Dim::Invalid)                                                   \
      return std::string("Dim::Invalid");                                      \
    return "Dim::" + std::string(dim_names[static_cast<uint16_t>(dim)]);       \
  }                                                                            \
  }

} // namespace scipp::units

#endif // SCIPP_UNITS_DIMENSION_H
