# ~~~
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# ~~~
include(scipp-util)

set(convert_to_rad
    "#include \"scipp/variable/to_unit.h\"
namespace {
decltype(auto) preprocess(const scipp::variable::Variable &var) {
  return to_unit(var, scipp::sc_units::rad)\;
}
}"
)

scipp_unary(math abs OUT)
scipp_unary(math exp OUT)
scipp_unary(math log OUT)
scipp_unary(math log10 OUT)
scipp_unary(math reciprocal OUT)
scipp_unary(math sqrt OUT)
scipp_unary(math norm)
scipp_unary(math floor OUT)
scipp_unary(math ceil OUT)
scipp_unary(math rint OUT)
scipp_unary(math erf)
scipp_unary(math erfc)
scipp_unary(math inv SKIP_VARIABLE)
scipp_binary(math pow SKIP_VARIABLE OUT)
scipp_binary(math dot)
scipp_binary(math cross)
setup_scipp_category(math)

scipp_unary(util values)
scipp_unary(util variances)
scipp_unary(util stddevs)
setup_scipp_category(util)

scipp_unary(trigonometry sin PREPROCESS_VARIABLE "${convert_to_rad}" OUT)
scipp_unary(trigonometry cos PREPROCESS_VARIABLE "${convert_to_rad}" OUT)
scipp_unary(trigonometry tan PREPROCESS_VARIABLE "${convert_to_rad}" OUT)
scipp_unary(trigonometry asin OUT)
scipp_unary(trigonometry acos OUT)
scipp_unary(trigonometry atan OUT)
scipp_binary(trigonometry atan2 OUT)
setup_scipp_category(trigonometry OUT)

scipp_unary(hyperbolic sinh OUT)
scipp_unary(hyperbolic cosh OUT)
scipp_unary(hyperbolic tanh OUT)
scipp_unary(hyperbolic asinh OUT)
scipp_unary(hyperbolic acosh OUT)
scipp_unary(hyperbolic atanh OUT)
setup_scipp_category(hyperbolic OUT)

scipp_unary(special_values isnan)
scipp_unary(special_values isinf)
scipp_unary(special_values isfinite)
scipp_unary(special_values isposinf)
scipp_unary(special_values isneginf)
setup_scipp_category(special_values)

scipp_binary(comparison equal)
scipp_binary(comparison greater)
scipp_binary(comparison greater_equal)
scipp_binary(comparison less)
scipp_binary(comparison less_equal)
scipp_binary(comparison not_equal)
setup_scipp_category(comparison)

scipp_function("unary" arithmetic operator- OP negative)
scipp_function(
  "binary"
  arithmetic
  operator+
  OP
  add
  SKIP_VARIABLE
  BASE_INCLUDE
  variable/arithmetic.h
)
scipp_function(
  "binary"
  arithmetic
  operator-
  OP
  subtract
  SKIP_VARIABLE
  BASE_INCLUDE
  variable/arithmetic.h
)
scipp_function(
  "binary"
  arithmetic
  operator*
  OP
  multiply
  SKIP_VARIABLE
  BASE_INCLUDE
  variable/arithmetic.h
)
scipp_function(
  "binary"
  arithmetic
  operator/
  OP
  divide
  SKIP_VARIABLE
  BASE_INCLUDE
  variable/arithmetic.h
)
scipp_function("binary" arithmetic floor_divide)
scipp_function("binary" arithmetic operator% OP mod)
scipp_function(
  "inplace"
  arithmetic
  operator+=
  OP
  add_equals
  SKIP_VARIABLE
  BASE_INCLUDE
  variable/arithmetic.h
  SKIP_PYTHON
)
scipp_function(
  "inplace"
  arithmetic
  operator-=
  OP
  subtract_equals
  SKIP_VARIABLE
  BASE_INCLUDE
  variable/arithmetic.h
  SKIP_PYTHON
)
scipp_function(
  "inplace"
  arithmetic
  operator*=
  OP
  multiply_equals
  SKIP_VARIABLE
  BASE_INCLUDE
  variable/arithmetic.h
  SKIP_PYTHON
)
scipp_function(
  "inplace"
  arithmetic
  operator/=
  OP
  divide_equals
  SKIP_VARIABLE
  BASE_INCLUDE
  variable/arithmetic.h
  SKIP_PYTHON
)
scipp_function(
  "inplace"
  arithmetic
  floor_divide_equals
  SKIP_VARIABLE
  BASE_INCLUDE
  variable/arithmetic.h
  SKIP_PYTHON
)
scipp_function("inplace" arithmetic operator%= OP mod_equals SKIP_PYTHON)
setup_scipp_category(arithmetic)

scipp_function("unary" logical operator~ OP logical_not)
scipp_function("binary" logical operator| OP logical_or)
scipp_function("binary" logical operator& OP logical_and)
scipp_function("binary" logical operator^ OP logical_xor)
scipp_function("inplace" logical operator|= OP logical_or_equals SKIP_PYTHON)
scipp_function("inplace" logical operator&= OP logical_and_equals SKIP_PYTHON)
scipp_function("inplace" logical operator^= OP logical_xor_equals SKIP_PYTHON)
setup_scipp_category(logical)

scipp_function(
  "unary" bins bin_sizes SKIP_VARIABLE BASE_INCLUDE variable/bins.h
)
scipp_function(
  "unary" bins bins_mean SKIP_VARIABLE BASE_INCLUDE variable/reduction.h
)
scipp_function(
  "unary" bins bins_nanmean SKIP_VARIABLE BASE_INCLUDE variable/reduction.h
)
scipp_function(
  "unary" bins bins_sum SKIP_VARIABLE BASE_INCLUDE variable/reduction.h
)
scipp_function(
  "unary" bins bins_nansum SKIP_VARIABLE BASE_INCLUDE variable/reduction.h
)
scipp_function(
  "unary" bins bins_max SKIP_VARIABLE BASE_INCLUDE variable/reduction.h
)
scipp_function(
  "unary" bins bins_nanmax SKIP_VARIABLE BASE_INCLUDE variable/reduction.h
)
scipp_function(
  "unary" bins bins_min SKIP_VARIABLE BASE_INCLUDE variable/reduction.h
)
scipp_function(
  "unary" bins bins_nanmin SKIP_VARIABLE BASE_INCLUDE variable/reduction.h
)
scipp_function(
  "unary" bins bins_all SKIP_VARIABLE BASE_INCLUDE variable/reduction.h
)
scipp_function(
  "unary" bins bins_any SKIP_VARIABLE BASE_INCLUDE variable/reduction.h
)
setup_scipp_category(bins)

scipp_function("reduction" reduction sum SKIP_VARIABLE)
scipp_function("reduction" reduction nansum SKIP_VARIABLE)
scipp_function("reduction" reduction max SKIP_VARIABLE)
scipp_function("reduction" reduction nanmax SKIP_VARIABLE)
scipp_function("reduction" reduction min SKIP_VARIABLE)
scipp_function("reduction" reduction nanmin SKIP_VARIABLE)
scipp_function("reduction" reduction all SKIP_VARIABLE)
scipp_function("reduction" reduction any SKIP_VARIABLE)
scipp_function("reduction" reduction mean SKIP_VARIABLE SKIP_DATASET)
scipp_function("reduction" reduction nanmean SKIP_VARIABLE SKIP_DATASET)
setup_scipp_category(reduction)
