#pragma once

#include <any>
#include <functional>
#include <string>
#include <unordered_map>

#include "scipp/units/unit.h"

#include "scipp-core_export.h"
#include "scipp/core/dtype.h"

namespace scipp::core {
struct FormatSpec;
class FormatRegistry;

// The any is expected to contain a std::reference_wrapper<T>
using FormatImpl = std::function<std::string(
    const std::any &, const FormatSpec &, const FormatRegistry &)>;

class SCIPP_CORE_EXPORT FormatRegistry {
public:
  static FormatRegistry &instance() noexcept;

  void add(DType dtype, const FormatImpl &formatter);
  std::string format(DType dtype, const std::any &value,
                     const FormatSpec &spec) const;

  template <class T>
  std::string format(const T &value, const FormatSpec &spec) const {
    return format(dtype<std::decay_t<T>>, std::cref(value), spec);
  }

  template <class T> struct insert_global {
    template <class F> explicit insert_global(F &&formatter) {
      FormatRegistry::instance().add(
          dtype<T>, [f = std::forward<F>(formatter)](
                        const std::any &value, const FormatSpec &spec,
                        const FormatRegistry &formatters) {
            return f(
                std::any_cast<std::reference_wrapper<const T>>(value).get(),
                spec, formatters);
          });
    }
  };

private:
  std::unordered_map<DType, FormatImpl> m_formatters = {};
};

struct SCIPP_CORE_EXPORT FormatSpec {
  bool has_spec() const noexcept { return !spec.empty(); }

  [[nodiscard]] std::string_view full() const;

  [[nodiscard]] std::string_view current() const;

  [[nodiscard]] FormatSpec nested() const;

  std::string spec;
  std::optional<units::Unit> unit = std::nullopt;
};
} // namespace scipp::core
