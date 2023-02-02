#include "format.h"

#include "scipp/core/format.h"

using namespace scipp;
using namespace scipp::core;
using namespace scipp::variable;

namespace py = pybind11;

namespace {
core::FormatRegistry &py_formatters() {
  static core::FormatRegistry formatters = core::FormatRegistry::instance();
  return formatters;
}

template <class T>
std::string format_with_python(const T &value, std::string_view format_spec) {
  const py::gil_scoped_acquire gil;
  const py::object builtins = py::module_::import("builtins");
  const py::object format = builtins.attr("format");
  return format(value, format_spec).template cast<std::string>();
}

template <class T> void register_py_formatter() {
  py_formatters().add(dtype<std::decay_t<T>>, [](const std::any &value,
                                                 const core::FormatSpec &spec,
                                                 const core::FormatRegistry &) {
    if (!spec.has_spec()) {
      // Avoid acquiring the GIL if possible.
      return core::FormatRegistry::instance().format(dtype<std::decay_t<T>>,
                                                     value, spec);
    }
    return format_with_python(
        std::any_cast<std::reference_wrapper<const std::decay_t<T>>>(value),
        spec.full());
  });
}

void register_formatters() {
  register_py_formatter<int64_t>();
  register_py_formatter<int32_t>();
  register_py_formatter<double>();
  register_py_formatter<float>();
  register_py_formatter<std::string>();
}
} // namespace

void bind_format_variable(py::class_<Variable> &variable) {
  register_formatters();

  variable.def(
      "__format__", [](const Variable &self, const std::string &format_string) {
        return py_formatters().format(self, core::FormatSpec{format_string});
      });
}
