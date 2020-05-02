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

#define TRIG_FUNC_ARGS(op) op, op, a##op, a##op, #op

const Docstring trigonometric_docstring(std::string fname) {
  return {"Element-wise " + fname,
          "If the unit is not a plane-angle unit, or if the dtype has no " +
              fname + ", e.g., if it is an integer.",
          "",
          "Variable containing the " + fname + " of the input values.",
          // "Variable, DataArray, or Dataset.",
          {{"x", "Input Variable, DataArray, or Dataset."}}};
}

namespace {
using VConstView = const typename Variable::const_view_type &;
using VView = typename Variable::view_type;
} // namespace

void bind_trigonometric_function(Variable (*func)(VConstView),
                                 VView (*func_out)(VConstView, const VView &),
                                 Variable (*afunc)(VConstView),
                                 VView (*afunc_out)(VConstView, const VView &),
                                 std::string fname, py::module &m) {
  Docstring docs = trigonometric_docstring(fname);
  bind_free_function<Variable, VConstView>(func, fname, m, docs);
  bind_free_function<VView, VConstView, const VView &>(func_out, fname, m,
                                                       docs.with_out_arg());
  const std::string afname = "a" + fname;
  docs = trigonometric_docstring(afname);
  bind_free_function<Variable, VConstView>(afunc, afname, m, docs);
  bind_free_function<VView, VConstView, const VView &>(afunc_out, afname, m,
                                                       docs.with_out_arg());
}

void init_trigonometry(py::module &m) {

  bind_trigonometric_function(TRIG_FUNC_ARGS(sin), m);
  bind_trigonometric_function(TRIG_FUNC_ARGS(cos), m);
  bind_trigonometric_function(TRIG_FUNC_ARGS(tan), m);

  Docstring docs = trigonometric_docstring("atan2");
  docs.insert_param(0, {"y", "Input Variable, DataArray, or Dataset."});
  bind_free_function<Variable, VConstView, VConstView>(atan2, "atan2", m, docs);
  bind_free_function<VView, VConstView, VConstView, const VView &>(
      atan2, "atan2", m, docs.with_out_arg());
}
