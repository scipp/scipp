# ~~~
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# ~~~
include(scipp-util)

set(convert_to_rad
    "#include \"scipp/variable/to_unit.h\"
namespace {
decltype(auto) preprocess(const scipp::variable::Variable &var) {
  return to_unit(var, scipp::units::rad)\;
}
}"
)

scipp_unary(math abs)
scipp_unary(math exp)
scipp_unary(math log)
scipp_unary(math log10)
scipp_unary(math reciprocal)
scipp_unary(math sqrt)
scipp_unary(math norm NO_OUT)
scipp_binary(math pow SKIP_VARIABLE NO_OUT)
scipp_binary(math dot NO_OUT)
setup_scipp_category(math)

scipp_unary(util values NO_OUT)
scipp_unary(util variances NO_OUT)
scipp_unary(util stddevs NO_OUT)
setup_scipp_category(util)

scipp_unary(trigonometry sin PREPROCESS_VARIABLE "${convert_to_rad}")
scipp_unary(trigonometry cos PREPROCESS_VARIABLE "${convert_to_rad}")
scipp_unary(trigonometry tan PREPROCESS_VARIABLE "${convert_to_rad}")
scipp_unary(trigonometry asin)
scipp_unary(trigonometry acos)
scipp_unary(trigonometry atan)
scipp_binary(trigonometry atan2)
setup_scipp_category(trigonometry)

scipp_unary(special_values isnan NO_OUT)
scipp_unary(special_values isinf NO_OUT)
scipp_unary(special_values isfinite NO_OUT)
scipp_unary(special_values isposinf NO_OUT)
scipp_unary(special_values isneginf NO_OUT)
setup_scipp_category(special_values)

scipp_binary(comparison equal NO_OUT)
scipp_binary(comparison greater NO_OUT)
scipp_binary(comparison greater_equal NO_OUT)
scipp_binary(comparison less NO_OUT)
scipp_binary(comparison less_equal NO_OUT)
scipp_binary(comparison not_equal NO_OUT)
setup_scipp_category(comparison)

scipp_function("unary" arithmetic operator- OP unary_minus NO_OUT)
scipp_function("binary" arithmetic operator+ OP add NO_OUT)
scipp_function("binary" arithmetic operator- OP subtract NO_OUT)
scipp_function("binary" arithmetic operator* OP multiply NO_OUT)
scipp_function("binary" arithmetic operator/ OP divide NO_OUT)
scipp_function("binary" arithmetic floor_divide NO_OUT)
scipp_function("binary" arithmetic operator% OP mod NO_OUT)
scipp_function("inplace" arithmetic operator+= OP add_equals)
scipp_function("inplace" arithmetic operator-= OP subtract_equals)
scipp_function("inplace" arithmetic operator*= OP multiply_equals)
scipp_function("inplace" arithmetic operator/= OP divide_equals)
scipp_function("inplace" arithmetic operator%= OP mod_equals)
setup_scipp_category(arithmetic)

scipp_function("unary" logical operator~ OP logical_not NO_OUT)
scipp_function("binary" logical operator| OP logical_or NO_OUT)
scipp_function("binary" logical operator& OP logical_and NO_OUT)
scipp_function("binary" logical operator^ OP logical_xor NO_OUT)
scipp_function("inplace" logical operator|= OP logical_or_equals)
scipp_function("inplace" logical operator&= OP logical_and_equals)
scipp_function("inplace" logical operator^= OP logical_xor_equals)
setup_scipp_category(logical)
