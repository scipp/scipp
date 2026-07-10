// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "nanobind.h"

namespace nb = nanobind;

void init_buckets(nb::module_ &);
void init_comparison(nb::module_ &);
void init_counts(nb::module_ &);
void init_creation(nb::module_ &);
void init_cumulative(nb::module_ &);
void init_dataset(nb::module_ &);
void init_dtype(nb::module_ &);
void init_element_array_view(nb::module_ &);
void init_exceptions(nb::module_ &);
void init_groupby(nb::module_ &);
void init_geometry(nb::module_ &);
void init_histogram(nb::module_ &);
void init_operations(nb::module_ &);
void init_shape(nb::module_ &);
void init_trigonometry(nb::module_ &);
void init_unary(nb::module_ &);
void init_units(nb::module_ &);
void init_variable(nb::module_ &);
void init_transform(nb::module_ &);

void init_generated_arithmetic(nb::module_ &);
void init_generated_bins(nb::module_ &);
void init_generated_comparison(nb::module_ &);
void init_generated_hyperbolic(nb::module_ &);
void init_generated_logical(nb::module_ &);
void init_generated_math(nb::module_ &);
void init_generated_reduction(nb::module_ &);
void init_generated_trigonometry(nb::module_ &);
void init_generated_util(nb::module_ &);
void init_generated_special_values(nb::module_ &);

void init_core(nb::module_ &m) {
  auto core = m.def_submodule("core");
  // Bind classes before any functions that use them to make sure that
  // nanobind puts proper type annotations into the docstrings.
  init_units(core);
  init_exceptions(core);
  init_dtype(core);
  init_variable(core);
  init_dataset(core);

  init_counts(core);
  init_creation(core);
  init_cumulative(core);
  init_buckets(core);
  init_groupby(core);
  init_comparison(core);
  init_operations(core);
  init_shape(core);
  init_geometry(core);
  init_histogram(core);
  init_trigonometry(core);
  init_unary(core);
  init_element_array_view(core);
  init_transform(core);

  init_generated_arithmetic(core);
  init_generated_bins(core);
  init_generated_comparison(core);
  init_generated_hyperbolic(core);
  init_generated_logical(core);
  init_generated_math(core);
  init_generated_reduction(core);
  init_generated_trigonometry(core);
  init_generated_util(core);
  init_generated_special_values(core);
}

NB_MODULE(_scipp, m) {
  // scipp's Python layer stores instances of bound types (DType, Unit, ...)
  // in module-level globals which are only released at interpreter shutdown,
  // after nanobind's leak check runs. Disable the resulting false-positive
  // warnings.
  nb::set_leak_warnings(false);
#ifdef SCIPP_VERSION
  m.attr("__version__") = nb::str(SCIPP_VERSION);
#else
  m.attr("__version__") = nb::str("unknown version");
#endif
#ifdef NDEBUG
  m.attr("_debug_") = nb::cast(false);
#else
  m.attr("_debug_") = nb::cast(true);
#endif
  init_core(m);
}
