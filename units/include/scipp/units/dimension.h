// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_UNITS_DIMENSION_H
#define SCIPP_UNITS_DIMENSION_H

#include <cstdint>

namespace scipp::units {

#define xstr(...) STR(__VA_ARGS__)
#define STR(...) #__VA_ARGS__

#define SCIPP_UNITS_DECLARE_DIMENSIONS(...)                                    \
  enum class Dim : uint16_t { Invalid, __VA_ARGS__ };                          \
                                                                               \
  namespace detail {                                                           \
  constexpr const char *names = #__VA_ARGS__;                                  \
                                                                               \
  enum dim_list { __VA_ARGS__ };                                               \
                                                                               \
  constexpr std::integer_sequence<dim_list, __VA_ARGS__> dim_sequence;         \
  }                                                                            \
                                                                               \
  std::string to_string(const Dim dim);

#define SCIPP_UNITS_DEFINE_DIMENSIONS()                                        \
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
  template <detail::dim_list... Dims> constexpr auto dim_count() {             \
    return sizeof...(Dims);                                                    \
  }                                                                            \
                                                                               \
  constexpr auto make_dim_name(const size_t index) {                           \
    const auto pos = get_pos(index);                                           \
    const auto count = get_pos(index + 1) - pos - 2;                           \
    return names.substr(pos, count);                                           \
  }                                                                            \
  template <detail::dim_list... Dims>                                          \
  constexpr auto                                                               \
  make_dim_names(std::integer_sequence<detail::dim_list, Dims...>) {           \
    return std::array{make_dim_name(Dims)...};                                 \
  }                                                                            \
                                                                               \
  constexpr auto dim_names = make_dim_names(detail::dim_sequence);             \
  }                                                                            \
                                                                               \
  std::string to_string(const Dim dim) {                                       \
    if (dim == Dim::Invalid)                                                   \
      return std::string("Dim::Invalid");                                      \
    return "Dim::" + std::string(dim_names[static_cast<uint16_t>(dim) - 1]);   \
  }

// constexpr bool isContinuous(const Dim dim) {
//  if (dim == Dim::DSpacing || dim == Dim::Energy || dim == Dim::DeltaE ||
//      dim == Dim::Tof || dim == Dim::X || dim == Dim::Y || dim == Dim::Z)
//    return true;
//  return false;
//}

} // namespace scipp::units

#endif // SCIPP_UNITS_DIMENSION_H
