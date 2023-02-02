#include "scipp/core/format.h"

#include "scipp/core/eigen.h"
#include "scipp/core/string.h"
#include "scipp/core/time_point.h"

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
template <class T, class F> auto plain_formatter(F &&impl) {
  return [impl = std::forward<F>(impl)](const T &value, const FormatSpec &spec,
                                        const FormatRegistry &) {
    if (spec.has_spec())
      throw std::invalid_argument("Type does not support format specifier");
    return impl(value);
  };
}

template <class T> struct insert_plain : FormatRegistry::insert_global<T> {
  template <class F>
  explicit insert_plain(F &&f)
      : FormatRegistry::insert_global<T>(
            plain_formatter<T>(std::forward<F>(f))) {}
};

[[maybe_unused]] auto format_int64 = insert_plain<int64_t>(
    [](const int64_t &value) { return std::to_string(value); });
[[maybe_unused]] auto format_int32 = insert_plain<int32_t>(
    [](const int32_t &value) { return std::to_string(value); });
[[maybe_unused]] auto format_double = insert_plain<double>(
    [](const double &value) { return std::to_string(value); });
[[maybe_unused]] auto format_float = insert_plain<float>(
    [](const float &value) { return std::to_string(value); });
[[maybe_unused]] auto format_bool = insert_plain<bool>(
    [](const bool &value) { return value ? "True" : "False"; });
[[maybe_unused]] auto format_string = insert_plain<std::string>(
    [](const std::string &value) { return '"' + value + '"'; });

std::string format_vector3d_impl(const Eigen::Vector3d &value) {
  std::ostringstream os;
  os << "(" << value[0] << ", " << value[1] << ", " << value[2] << "), ";
  return os.str();
}
std::string format_matrix3d_impl(const Eigen::Matrix3d &value) {
  return '(' + format_vector3d_impl(value.row(0)) + ", " +
         format_vector3d_impl(value.row(1)) + ", " +
         format_vector3d_impl(value.row(2)) + ')';
}

[[maybe_unused]] auto format_vector3d =
    insert_plain<Eigen::Vector3d>(format_vector3d_impl);
[[maybe_unused]] auto format_matrix3d =
    insert_plain<Eigen::Matrix3d>(format_matrix3d_impl);
[[maybe_unused]] auto format_affine3d =
    insert_plain<Eigen::Affine3d>([](const Eigen::Affine3d &value) {
      std::ostringstream os;
      os << value.matrix();
      return os.str();
    });
[[maybe_unused]] auto format_quaternion =
    insert_plain<Quaternion>([](const Quaternion &value) {
      std::stringstream ss;
      ss << '(' << value.quat().w();
      ss.setf(std::ios::showpos);
      ss << value.quat().x() << 'i' << value.quat().y() << 'j'
         << value.quat().z() << "k), ";
      return ss.str();
    });
[[maybe_unused]] auto format_translation =
    insert_plain<Translation>([](const Translation &value) {
      return format_vector3d_impl(value.vector());
    });

[[maybe_unused]] auto format_datetime =
    FormatRegistry::insert_global<scipp::core::time_point>(
        [](const scipp::core::time_point &value, const FormatSpec &spec,
           const FormatRegistry &) {
          if (spec.has_spec())
            throw std::invalid_argument(
                "Type does not support format specifier");
          if (!spec.unit.has_value())
            throw std::invalid_argument(
                "Cannot format datetime without a unit");
          return to_iso_date(value, *spec.unit);
        });

} // namespace
} // namespace scipp::core
