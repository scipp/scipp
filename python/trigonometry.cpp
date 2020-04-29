// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "bind_free_function.h"
#include "pybind11.h"

#include "scipp/dataset/dataset.h"
#include "scipp/variable/operations.h"

using namespace scipp;
using namespace scipp::variable;
using namespace scipp::dataset;
using namespace scipp::python;

namespace py = pybind11;

// #define BIND_TRIGONOMETRIC_FUNCTION(op, mod, T)                                \

//   head = std::string("Element-wise ") + #op + ".\n" +                          \
//          ":param x: Input Variable, DataArray, or Dataset.\n";                 \
//   tail = std::string(":raises: If the unit is not a plane-angle unit, or if "  \
//                      "the dtype has no ") +                                    \
//          #op +                                                                 \
//          ", e.g., if it is an integer.\n"                                      \
//          ":return: Variable containing the " +                                 \
//          #op + " of the input values.\n";                                      \
//   rtype = ":rtype: Variable, DataArray, or Dataset.";                          \
//   rtype_out = ":rtype: VariableView, DataArrayView, or DatasetView.";          \
//   param_out = std::string(":param out: Output buffer to which the ") + #op +   \
//               " values will be written.\n";                                    \
//   aop = std::string("a") + #op;                                                \
//   mod.def(#op, [](T::const_view_type self) { return op(self); }, py::arg("x"), \
//           py::call_guard<py::gil_scoped_release>(),                            \
//           (head + tail + rtype).c_str());                                        \
//   mod.def(                                                                     \
//       #op,                                                                     \
//       [](T::const_view_type self, T::view_type out) { return op(self, out); }, \
//       py::arg("x"), py::arg("out"), py::call_guard<py::gil_scoped_release>(),  \
//       (head + param_out + tail + rtype_out).c_str());                            \
//   mod.def(aop.c_str(), [](T::const_view_type self) { return a##op(self); },      \
//           py::arg("x"), py::call_guard<py::gil_scoped_release>(),              \
//           (head + tail + rtype).c_str());                                        \
//   mod.def(aop.c_str(),                                                           \
//           [](T::const_view_type self, T::view_type out) {                      \
//             return a##op(self, out);                                           \
//           },                                                                   \
//           py::arg("x"), py::arg("out"),                                        \
//           py::call_guard<py::gil_scoped_release>(),                            \
//           (head + param_out + tail + rtype_out).c_str());


#define TRIG_FUNC_ARGS(op) op, op, a##op, a##op, #op



const Docstring trigonometric_docstring(std::string fname) {
  return {"Element-wise " + fname,
          "If the unit is not a plane-angle unit, or if the dtype has no " + fname + ", e.g., if it is an integer.",
          "",
          "Variable containing the " + fname + " of the input values.",
          "Variable, DataArray, or Dataset.",
          {{"x", "Input Variable, DataArray, or Dataset."}}};
}

namespace {
using VConstView = const typename Variable::const_view_type &;
using VView = typename Variable::view_type;
}

// template <class T>
void bind_trigonometric_function(Variable (*func)(VConstView),
  VView (*func_out)(VConstView, const VView &),
  Variable (*afunc)(VConstView),
  VView (*afunc_out)(VConstView, const VView &),
  std::string fname, py::module &m) {
  Docstring docs = trigonometric_docstring(fname);
  bind_free_function<Variable, VConstView>(func, fname, m, docs);
  bind_free_function<VView, VConstView, const VView &>(func_out, fname, m, docs.with_out_arg());
  const std::string afname = "a" + fname;
  docs = trigonometric_docstring(afname);
  bind_free_function<Variable, VConstView>(afunc, afname, m, docs);
  bind_free_function<VView, VConstView, const VView &>(afunc_out, afname, m, docs.with_out_arg());
}

void init_trigonometry(py::module &m) {

  bind_trigonometric_function(TRIG_FUNC_ARGS(sin), m);
  bind_trigonometric_function(TRIG_FUNC_ARGS(cos), m);
  bind_trigonometric_function(TRIG_FUNC_ARGS(tan), m);

  Docstring docs = trigonometric_docstring("atan2");
  docs.insert_param(0, {"y", "Input Variable, DataArray, or Dataset."});
  bind_free_function<Variable, VConstView, VConstView>(atan2, "atan2", m, docs);
  bind_free_function<VView, VConstView, VConstView, const VView &>(atan2, "atan2", m, docs.with_out_arg());

}
