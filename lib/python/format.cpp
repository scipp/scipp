#include "format.h"

#include "scipp/core/format.h"
#include "scipp/core/tag_util.h"

using namespace scipp;
using namespace scipp::core;
using namespace scipp::variable;

namespace py = pybind11;
#include <iostream>
namespace {
template <class T> struct FormatElement {
  static std::string apply(const std::any &value,
                           const std::string &nested_spec) {
    const auto &val =
        std::any_cast<std::reference_wrapper<const T>>(value).get();
    [[maybe_unused]] const py::gil_scoped_acquire gil;
    const py::object builtins = py::module_::import("builtins");
    const py::object format = builtins.attr("format");
    return format(val, nested_spec).template cast<std::string>();
  }
};

FormatSpec parse_variable_spec(const std::string &format_string) {
  // TODO proper parsing
  const auto pos = format_string.rfind(':');
  if (pos == std::string::npos) {
    throw std::invalid_argument("Bad format");
  }
  auto nested = format_string.substr(pos + 1);
  return FormatSpec{
      [nested = std::move(nested)](DType dtype, const std::any &value) {
        return callDType<FormatElement>(std::tuple<double, int64_t>{}, dtype,
                                        value, nested);
      }};
}
} // namespace

void bind_format_variable(py::class_<Variable> &variable) {
  variable.def(
      "__format__", [](const Variable &self, const std::string &format_string) {
        if (format_string.empty())
          return core::FormatRegistry::format(self, core::FormatSpec{});
        return core::FormatRegistry::format(self,
                                            parse_variable_spec(format_string));
      });
}
