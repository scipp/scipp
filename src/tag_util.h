#ifndef TAG_UTIL_H
#define TAG_UTIL_H

#include "tags.h"

template <class... Tags> struct Call {
  template <template <class> class Callable, class... Args>
  static auto apply(const Tag tag, Args &&... args) {
    std::array funcs{Callable<Tags>::apply...};
    std::array<Tag, sizeof...(Tags)> tags{Tags{}...};
    for (gsl::index i = 0; i < tags.size(); ++i)
      if (tags[i].value() == tag.value())
        return funcs[i](std::forward<Args>(args)...);
    throw std::runtime_error("Unsupported tag type.");
  }
};

#endif // TAG_UTIL_H
