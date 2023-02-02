#include "scipp/core/format.h"

namespace scipp::core {
FormatRegistry &FormatRegistry::instance() noexcept {
  static FormatRegistry formatters;
  return formatters;
}

void FormatRegistry::add(const DType dtype, const FormatImpl &formatter) {
  m_formatters.insert_or_assign(dtype, formatter);
}

std::string FormatRegistry::format(const DType dtype, const std::any &value,
                                   const FormatSpec &spec) const {
  return m_formatters.at(dtype)(value, spec, *this);
}

namespace {
std::string::size_type first_colon(const std::string &s) {
  const auto pos = s.find_first_of(':');
  return pos == std::string::npos ? s.size() : pos;
}
} // namespace

[[nodiscard]] std::string_view FormatSpec::full() const { return spec; }

[[nodiscard]] std::string_view FormatSpec::current() const {
  return {spec.data(), first_colon(spec)};
}

[[nodiscard]] FormatSpec FormatSpec::nested() const {
  const auto start = first_colon(spec) + 1;
  if (start >= spec.size()) {
    return {};
  }
  return {spec.substr(start)};
}

namespace {
auto format_int64 = FormatRegistry::insert_global<int64_t>(
    [](const int64_t &value, const FormatSpec &, const FormatRegistry &) {
      return std::to_string(value);
    });
auto format_double = FormatRegistry::insert_global<double>(
    [](const double &value, const FormatSpec &, const FormatRegistry &) {
      return std::to_string(value);
    });
} // namespace
} // namespace scipp::core
