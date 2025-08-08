// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <units/units.hpp>

#include "scipp/variable/variable.h"

#include "scipp/core/dtype.h"
#include "scipp/core/eigen.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/except.h"
#include "scipp/variable/variable_concept.h"
#include "scipp/variable/variable_factory.h"

namespace scipp::variable {

namespace {
/// Types that default to unit=units::dimensionless. Everything else is
/// units::none.
const std::tuple<double, float, int64_t, int32_t, core::time_point,
                 Eigen::Vector3d, Eigen::Matrix3d, Eigen::Affine3d,
                 core::Quaternion, core::Translation>
    default_dimensionless_dtypes;

template <class... Ts>
bool is_default_dimensionless(DType type, std::tuple<Ts...>) {
  return ((type == dtype<Ts>) || ...);
}
} // namespace

sc_units::Unit default_unit_for(const DType type) {
  return is_default_dimensionless(type, default_dimensionless_dtypes)
             ? sc_units::dimensionless
             : sc_units::none;
}

/// Construct from parent with same dtype, unit, and has_variances but new dims.
///
/// In the case of bucket variables the buffer size is set to zero.
Variable::Variable(const Variable &parent, const Dimensions &dims)
    : m_dims(dims), m_strides(dims),
      m_object(parent.data().makeDefaultFromParent(dims.volume())) {}

// TODO there is no size check here!
Variable::Variable(const Dimensions &dims, VariableConceptHandle data)
    : m_dims(dims), m_strides(dims), m_object(std::move(data)) {}

Variable::Variable(const units::precise_measurement &m)
    : Variable(m.value() * sc_units::Unit(m.units())) {}

namespace {
void check_nested_in_assign(const Variable &lhs, const Variable &rhs) {
  if (!rhs.is_valid() || rhs.dtype() != dtype<Variable>) {
    return;
  }
  // In principle we should also check when the RHS contains DataArrays or
  // Datasets. But those are copied when stored in Variables,
  // so no check needed here.
  for (const auto &nested : rhs.values<Variable>()) {
    if (&lhs == &nested) {
      throw std::invalid_argument("Cannot assign Variable, the right hand side "
                                  "contains a reference to the left hand side. "
                                  "Reference cycles are not allowed.");
    }
    check_nested_in_assign(lhs, nested);
  }
}
} // namespace

Variable &Variable::operator=(const Variable &other) {
  return *this = Variable(other);
}

Variable &Variable::operator=(Variable &&other) {
  check_nested_in_assign(*this, other);
  m_dims = other.m_dims;
  m_strides = other.m_strides;
  m_offset = other.m_offset;
  m_object = std::move(other.m_object);
  m_readonly = other.m_readonly;
  m_aligned = other.m_aligned;
  return *this;
}

void Variable::setDataHandle(VariableConceptHandle object) {
  if (object->size() != m_object->size())
    throw except::DimensionError("Cannot replace by model of different size.");
  m_object = object;
}

const Dimensions &Variable::dims() const {
  if (!is_valid())
    throw std::runtime_error("invalid variable");
  return m_dims;
}

scipp::index Variable::ndim() const {
  if (!is_valid())
    throw std::runtime_error("invalid variable");
  return m_dims.ndim();
}

DType Variable::dtype() const { return data().dtype(); }

bool Variable::has_variances() const { return data().has_variances(); }

void Variable::expect_can_set_unit(const sc_units::Unit &unit) const {
  if (this->unit() != unit && is_slice())
    throw except::UnitError("Partial view on data of variable cannot be used "
                            "to change the unit.");
}

const sc_units::Unit &Variable::unit() const { return m_object->unit(); }

void Variable::setUnit(const sc_units::Unit &unit) {
  expect_writable();
  expect_can_set_unit(unit);
  m_object->setUnit(unit);
}

Dim Variable::dim() const {
  core::expect::ndim_is(dims(), 1);
  return dims().inner();
}

namespace {
bool compare(const Variable &a, const Variable &b, bool equal_nan) {
  if (equal_nan && a.is_same(b))
    return true;
  if (!a.is_valid() || !b.is_valid())
    return a.is_valid() == b.is_valid();
  // Note: Not comparing strides
  if (a.unit() != b.unit())
    return false;
  if (a.dims() != b.dims())
    return false;
  if (a.dtype() != b.dtype())
    return false;
  if (a.has_variances() != b.has_variances())
    return false;
  if (a.dims().volume() == 0 && a.dims() == b.dims())
    return true;
  return a.dims() == b.dims() &&
         (equal_nan ? a.data().equals_nan(a, b) : a.data().equals(a, b));
}
} // namespace

bool Variable::operator==(const Variable &other) const {
  return compare(*this, other, false);
}

bool equals_nan(const Variable &a, const Variable &b) {
  return compare(a, b, true);
}

bool Variable::operator!=(const Variable &other) const {
  return !(*this == other);
}

const VariableConcept &Variable::data() const & { return *m_object; }

VariableConcept &Variable::data() & {
  expect_writable();
  return *m_object;
}

const VariableConceptHandle &Variable::data_handle() const { return m_object; }

std::span<const scipp::index> Variable::strides() const {
  return std::span<const scipp::index>(&*m_strides.begin(),
                                       &*m_strides.begin() + dims().ndim());
}

scipp::index Variable::stride(const Dim dim) const {
  return m_strides[dims().index(dim)];
}

scipp::index Variable::offset() const { return m_offset; }

core::ElementArrayViewParams Variable::array_params() const {
  return {m_offset, dims(), m_strides, {}};
}

Variable Variable::slice(const Slice params) const {
  core::expect::validSlice(dims(), params);
  Variable out(*this);
  if (params == Slice{})
    return out;
  const auto dim = params.dim();
  const auto begin = params.begin();
  const auto end = params.end();
  const auto stride = params.stride();
  const auto index = out.m_dims.index(dim);
  out.m_offset += begin * m_strides[index];
  if (end == -1) {
    out.m_strides.erase(index);
    out.m_dims.erase(dim);
  } else {
    static_cast<Sizes &>(out.m_dims) = out.m_dims.slice(params);
    out.m_strides[index] *= stride;
  }
  return out;
}

void Variable::validateSlice(const Slice &s, const Variable &data) const {
  core::expect::validSlice(this->dims(), s);
  if (variableFactory().has_variances(data) !=
      variableFactory().has_variances(*this)) {
    auto variances_message = [](const auto &variable) {
      return "does" +
             std::string(variableFactory().has_variances(variable) ? ""
                                                                   : " NOT") +
             " have variances.";
    };
    throw except::VariancesError("Invalid slice operation. Slice " +
                                 variances_message(data) + "Variable " +
                                 variances_message(*this));
  }
  if (variableFactory().elem_unit(data) != variableFactory().elem_unit(*this))
    throw except::UnitError(
        "Invalid slice operation. Slice has unit: " +
        to_string(variableFactory().elem_unit(data)) +
        " Variable has unit: " + to_string(variableFactory().elem_unit(*this)));
  if (variableFactory().elem_dtype(data) != variableFactory().elem_dtype(*this))
    throw except::TypeError("Invalid slice operation. Slice has dtype " +
                            to_string(variableFactory().elem_dtype(data)) +
                            ". Variable has dtype " +
                            to_string(variableFactory().elem_dtype(*this)));
}

Variable &Variable::setSlice(const Slice params, const Variable &data) {
  validateSlice(params, data);
  copy(data, slice(params));
  return *this;
}

Variable Variable::broadcast(const Dimensions &target) const {
  expect::includes(target, dims());
  auto out = target.volume() == dims().volume() ? *this : as_const();
  out.m_dims = target;
  out.m_strides.clear();
  for (const auto &d : target.labels())
    out.m_strides.push_back(dims().contains(d) ? m_strides[dims().index(d)]
                                               : 0);
  return out;
}

Variable Variable::fold(const Dim dim, const Dimensions &target) const {
  auto out(*this);
  out.m_dims = core::fold(dims(), dim, target);
  out.m_strides.clear();
  const Strides substrides(target);
  for (scipp::index i_in = 0; i_in < dims().ndim(); ++i_in) {
    if (dims().label(i_in) == dim)
      for (scipp::index i_target = 0; i_target < target.ndim(); ++i_target)
        out.m_strides.push_back(m_strides[i_in] * substrides[i_target]);
    else
      out.m_strides.push_back(m_strides[i_in]);
  }
  return out;
}

Variable Variable::transpose(const std::span<const Dim> order) const {
  auto transposed(*this);
  transposed.m_strides = core::transpose(m_strides, dims(), order);
  transposed.m_dims = core::transpose(dims(), order);
  return transposed;
}

/// Return new variable with renamed dimensions.
///
/// The `fail_on_unknown` option is used internally for implementing rename_dims
/// in DataArray and related classes.
Variable Variable::rename_dims(const std::vector<std::pair<Dim, Dim>> &names,
                               const bool fail_on_unknown) const {
  auto out(*this);
  // names is a vector for predictable order
  out.m_dims = out.m_dims.rename_dims(names, fail_on_unknown);
  return out;
}

bool Variable::is_valid() const noexcept { return m_object.operator bool(); }

bool Variable::is_slice() const {
  // TODO Is this condition sufficient?
  return m_offset != 0 || m_dims.volume() != data().size();
}

bool Variable::is_readonly() const noexcept { return m_readonly; }

bool Variable::is_same(const Variable &other) const noexcept {
  return std::tie(m_dims, m_strides, m_offset, m_object) ==
         std::tie(other.m_dims, other.m_strides, other.m_offset,
                  other.m_object);
}

bool Variable::is_aligned() const noexcept { return m_aligned; }

void Variable::set_aligned(bool aligned) noexcept { m_aligned = aligned; }

void Variable::setVariances(const Variable &v) {
  expect_writable();
  if (is_slice())
    throw except::VariancesError(
        "Cannot add variances via sliced view of Variable.");
  if (v.is_valid()) {
    core::expect::equals(unit(), v.unit());
    core::expect::equals(dims(), v.dims());
  }
  data().setVariances(v);
}

namespace detail {
void throw_keyword_arg_constructor_bad_dtype(const DType dtype) {
  throw except::TypeError("Cannot create the Variable with type " +
                          to_string(dtype) +
                          " with such values and/or variances.");
}
} // namespace detail

Variable Variable::bin_indices() const {
  auto out{*this};
  out.m_object = data().bin_indices();
  return out;
}

Variable Variable::as_const() const {
  Variable out(*this);
  out.m_readonly = true;
  return out;
}

void Variable::expect_writable() const {
  if (m_readonly)
    throw except::VariableError("Read-only flag is set, cannot mutate data.");
}
} // namespace scipp::variable
