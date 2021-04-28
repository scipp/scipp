// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include "scipp/core/element/special_values.h"
#include "scipp/variable/special_values.h"
#include "scipp/variable/transform.h"

using namespace scipp::core;

namespace scipp::variable {

Variable &nan_to_num(const Variable &var, const Variable &replacement,
                     Variable &out) {
  transform_in_place(out, var, replacement, element::nan_to_num_out_arg);
  return out;
}

Variable &positive_inf_to_num(const Variable &var, const Variable &replacement,
                              Variable &out) {
  transform_in_place(out, var, replacement,
                     element::positive_inf_to_num_out_arg);
  return out;
}
Variable &negative_inf_to_num(const Variable &var, const Variable &replacement,
                              Variable &out) {
  transform_in_place(out, var, replacement,
                     element::negative_inf_to_num_out_arg);
  return out;
}

Variable nan_to_num(const Variable &var, const Variable &replacement) {
  return transform(var, replacement, element::nan_to_num);
}

Variable pos_inf_to_num(const Variable &var, const Variable &replacement) {
  return transform(var, replacement, element::positive_inf_to_num);
}

Variable neg_inf_to_num(const Variable &var, const Variable &replacement) {
  return transform(var, replacement, element::negative_inf_to_num);
}

} // namespace scipp::variable
