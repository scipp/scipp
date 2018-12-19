#ifndef TAG_UTIL_H
#define TAG_UTIL_H

#include "tags.h"

template <template <class> class Callable, class... Tags, class... Args>
static auto call(const std::tuple<Tags...> &, const Tag tag, Args &&... args) {
  std::array funcs{Callable<Tags>::apply...};
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

#endif // TAG_UTIL_H
