# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import pytest

import scipp as sc
import scipp.testing


def test_scalar_Variable_values_property_float() -> None:
    var = sc.scalar(value=1.0, variance=2.0)
    assert var.dtype == sc.DType.float64
    assert var.values == 1.0
    assert var.variances == 2.0


def test_scalar_Variable_values_property_int() -> None:
    var = sc.scalar(1)
    assert var.dtype == sc.DType.int64
    assert var.values == 1


def test_scalar_Variable_values_property_string() -> None:
    var = sc.scalar('abc')
    assert var.dtype == sc.DType.string
    assert var.values == 'abc'


def test_scalar_Variable_values_property_PyObject() -> None:
    var = sc.scalar([1, 2])
    assert var.dtype == sc.DType.PyObject
    assert var.values == [1, 2]


@pytest.mark.parametrize(
    'var', [sc.scalar(3.1), sc.scalar(-2), sc.scalar('abc'), sc.scalar([1, 2])]
)
def test_scalar_Variable_value_is_same_as_values(var: sc.Variable) -> None:
    assert var.value == var.values


@pytest.mark.parametrize('var', [sc.scalar(4), sc.scalar(4.61), sc.scalar('4')])
def test_scalar_Variable_conversion_to_builtin_int(var: sc.Variable) -> None:
    assert int(var) == 4


def test_scalar_Variable_conversion_to_builtin_int_bad_dtype() -> None:
    var = sc.vector(value=[1, 2, 3])
    with pytest.raises(TypeError):
        int(var)


def test_scalar_Variable_conversion_to_builtin_int_bad_unit() -> None:
    var = sc.scalar(7, unit='m')
    with pytest.raises(sc.UnitError):
        int(var)


def test_scalar_Variable_conversion_to_builtin_int_with_variance() -> None:
    var = sc.scalar(7.0, variance=2.0)
    with pytest.raises(sc.VariancesError):
        int(var)


def test_conversion_to_builtin_int_fails_with_array() -> None:
    var = sc.array(dims=['x'], values=[1])
    with pytest.raises(sc.DimensionError):
        int(var)


@pytest.mark.parametrize('var', [sc.scalar(-3), sc.scalar(-3.0), sc.scalar('-3.0')])
def test_scalar_Variable_conversion_to_builtin_float(var: sc.Variable) -> None:
    assert float(var) == -3.0


def test_scalar_Variable_conversion_to_builtin_float_bad_dtype() -> None:
    var = sc.vector(value=[1.0, 2.0, 3.0])
    with pytest.raises(TypeError):
        float(var)


def test_scalar_Variable_conversion_to_builtin_float_bad_unit() -> None:
    var = sc.scalar(7, unit='m')
    with pytest.raises(sc.UnitError):
        float(var)


def test_scalar_Variable_conversion_to_builtin_float_with_variance() -> None:
    var = sc.scalar(7.0, variance=2.0)
    with pytest.raises(sc.VariancesError):
        float(var)


def test_conversion_to_builtin_float_fails_with_array() -> None:
    var = sc.array(dims=['x'], values=[1.0])
    with pytest.raises(sc.DimensionError):
        float(var)


def test_scalar_Variable_conversion_to_builtin_bool_True() -> None:
    assert sc.scalar(True)


def test_scalar_Variable_conversion_to_builtin_bool_False() -> None:
    assert not sc.scalar(False)


def test_scalar_Variable_conversion_to_builtin_bool_bad_dtype() -> None:
    var = sc.scalar(value=1.0, unit=None)
    with pytest.raises(TypeError):
        bool(var)


def test_scalar_Variable_conversion_to_builtin_bool_bad_unit_dimensionless() -> None:
    var = sc.scalar(True, unit='')
    with pytest.raises(sc.UnitError):
        bool(var)


def test_scalar_Variable_conversion_to_builtin_bool_bad_unit() -> None:
    var = sc.scalar(True, unit='m')
    with pytest.raises(sc.UnitError):
        bool(var)


def test_conversion_to_builtin_bool_fails_with_array() -> None:
    var = sc.array(dims=['x'], values=[True, False])
    with pytest.raises(sc.DimensionError):
        bool(var)


def test_conversion_to_builtin_bool_fails_with_size_1_array() -> None:
    var = sc.array(dims=['x'], values=[True])
    with pytest.raises(sc.DimensionError):
        bool(var)


def test_scalar_Variable_conversion_to_builtin_index() -> None:
    var = sc.scalar(4)
    assert var.__index__() == 4


@pytest.mark.parametrize(
    'var', [sc.vector(value=[1, 2, 3]), sc.scalar(4.61), sc.scalar('4')]
)
def test_scalar_Variable_conversion_to_builtin_index_bad_dtype(
    var: sc.Variable,
) -> None:
    with pytest.raises(TypeError):
        var.__index__()


def test_scalar_Variable_conversion_to_builtin_index_bad_unit() -> None:
    var = sc.scalar(7, unit='m')
    with pytest.raises(sc.UnitError):
        var.__index__()


def test_scalar_Variable_conversion_to_builtin_index_with_variance() -> None:
    var = sc.scalar(7.0, variance=2.0)
    with pytest.raises(sc.VariancesError):
        var.__index__()


def test_conversion_to_builtin_index_fails_with_array() -> None:
    var = sc.array(dims=['x'], values=[1])
    with pytest.raises(sc.DimensionError):
        var.__index__()


# This test is mainly about ensuring that mypy accepts a variable as part of a slice.
def test_can_use_variable_as_index() -> None:
    index = sc.scalar(12.0)
    da = sc.DataArray(sc.arange('x', 4), coords={'x': sc.arange('x', 10, 14)})
    sliced = da['x', :index]  # There should be no type error here
    sc.testing.assert_identical(sliced, da['x', :2])
