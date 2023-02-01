#include "scipp/core/format.h"

namespace scipp::core {
FormatRegistry &FormatRegistry::instance() noexcept {
  static FormatRegistry instance;
  return instance;
}

void FormatRegistry::add(const DType dtype, const FormatImpl &formatter) {
  m_formatters.insert_or_assign(dtype, formatter);
}

std::string FormatRegistry::format(const DType dtype,
                                   const std::any &value) const {
  return m_formatters.at(dtype)(value);
}

namespace {
auto format_int64 = FormatRegistry::insert<int64_t>(
    [](const int64_t &value) { return std::to_string(value); });
auto format_double = FormatRegistry::insert<double>(
    [](const double &value) { return std::to_string(value); });
} // namespace
} // namespace scipp::core
