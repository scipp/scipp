#pragma once

#include <any>
#include <functional>
#include <string>
#include <unordered_map>

#include "scipp-core_export.h"
#include "scipp/core/dtype.h"

namespace scipp::core {
// The any is expected to contain a std::reference_wrapper<T>
using FormatImpl = std::function<std::string(const std::any &)>;

class SCIPP_CORE_EXPORT FormatRegistry {
public:
  static FormatRegistry &instance() noexcept;

  void add(DType dtype, const FormatImpl &formatter);
  std::string format(DType dtype, const std::any &value) const;

  template <class T> static std::string format(const T &value) {
    return FormatRegistry::instance().format(dtype<std::decay_t<T>>,
                                             std::cref(value));
  }

  template <class T> struct insert {
    template <class F> explicit insert(F &&formatter) {
      FormatRegistry::instance().add(dtype<T>, [f = std::forward<F>(formatter)](
                                                   const std::any &value) {
        return f(std::any_cast<std::reference_wrapper<const T>>(value).get());
      });
    }
  };

private:
  std::unordered_map<DType, FormatImpl> m_formatters = {};
};
} // namespace scipp::core