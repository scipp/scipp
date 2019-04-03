/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#ifndef TAG_UTIL_H
#define TAG_UTIL_H

#include "tags.h"

namespace scipp::core {

/**
 * At the time of writing older clang 6 lacks support for template
 * argument deduction of class templates (OSX only?)
 * http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0620r0.html
 * https://clang.llvm.org/cxx_status.html
 *
 * This make helper uses function template argument deduction to solve
 * for std::array declarations.
 */
template <typename... T> constexpr auto make_array(T &&... values) {
  return std::array<std::decay_t<std::common_type_t<T...>>, sizeof...(T)>{
      std::forward<T>(values)...};
}

template <template <class> class Callable, class... Tags, class... Args>
static auto call(const std::tuple<Tags...> &, const Tag tag, Args &&... args) {
  auto funcs = make_array(Callable<Tags>::apply...);
  std::array<Tag, sizeof...(Tags)> tags{Tags{}...};
  for (size_t i = 0; i < tags.size(); ++i)
    if (tags[i].value() == tag.value())
      return funcs[i](std::forward<Args>(args)...);
  throw std::runtime_error("Unsupported tag type.");
}

/// Apply Callable<T> to args, for any tag T in Tags, determined by the runtime
/// tag given by `tag`.
template <class... Tags> struct Call {
  template <template <class> class Callable, class... Args>
  static auto apply(const Tag tag, Args &&... args) {
    return call<Callable>(std::tuple<Tags...>{}, tag,
                          std::forward<Args>(args)...);
  }
};

namespace detail {
template <class... Tags> constexpr auto makeTags(const std::tuple<Tags...> &) {
  return std::make_tuple(detail::TagImpl<Tags>{}...);
}
} // namespace detail

/// Apply Callable<T> to args, for arbitrary tag T, determined by the runtime
/// tag given by `tag`.
template <template <class> class Callable, class... Args>
auto callForAnyTag(const Tag tag, Args &&... args) {
  return call<Callable>(detail::makeTags(detail::Tags{}), tag,
                        std::forward<Args>(args)...);
}

// Same as `call` for tags, but based on dtype.
template <template <class> class Callable, class... Ts, class... Args>
static auto callDType(const std::tuple<Ts...> &, const DType dtype,
                      Args &&... args) {
  auto funcs = make_array(Callable<Ts>::apply...);
  std::array<DType, sizeof...(Ts)> dtypes{scipp::core::dtype<Ts>...};
  for (size_t i = 0; i < dtypes.size(); ++i)
    if (dtypes[i] == dtype)
      return funcs[i](std::forward<Args>(args)...);
  throw std::runtime_error("Unsupported dtype.");
}

/// Apply Callable<T> to args, for any dtype T in Ts, determined by the
/// runtime dtype given by `dtype`.
template <class... Ts> struct CallDType {
  template <template <class> class Callable, class... Args>
  static auto apply(const DType dtype, Args &&... args) {
    return callDType<Callable>(std::tuple<Ts...>{}, dtype,
                               std::forward<Args>(args)...);
  }
};

} // namespace scipp::core

#endif // TAG_UTIL_H
