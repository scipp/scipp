# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen
import numpy as np
import numpy.typing as npt

import scipp as sc


def make_variables() -> tuple[
    sc.Variable, sc.Variable, sc.Variable, sc.Variable, npt.NDArray[np.float64]
]:
    data = np.arange(1, 4, dtype=float)
    a = sc.Variable(dims=['x'], values=data)
    b = sc.Variable(dims=['x'], values=data)
    a_slice = a['x', :]
    b_slice = b['x', :]
    return a, b, a_slice, b_slice, data


# This check is important: It can happen that an implementation of,
# e.g., __iadd__ does an in-place modification, updating `b`, but then the
# return value is assigned to `a`, which could break the connection unless
# the correct Python object is returned.
def test_iadd_returns_original_object() -> None:
    a = sc.scalar(1.2)
    b = a
    a += 1.0
    assert sc.identical(a, b)


def test_isub_returns_original_object() -> None:
    a = sc.scalar(1.2)
    b = a
    a -= 1.0
    assert sc.identical(a, b)


def test_imul_returns_original_object() -> None:
    a = sc.scalar(1.2)
    b = a
    a *= 1.0
    assert sc.identical(a, b)


def test_itruediv_returns_original_object() -> None:
    a = sc.scalar(1.2)
    b = a
    a /= 1.0
    assert sc.identical(a, b)


def test_ifloordiv_returns_original_object() -> None:
    a = sc.scalar(1.2)
    b = a
    a //= 1.0
    assert sc.identical(a, b)


def test_add_variable() -> None:
    a, b, a_slice, b_slice, data = make_variables()
    c = a + b
    assert np.array_equal(c.values, data + data)
    c = a + 2.0
    assert np.array_equal(c.values, data + 2.0)
    c = a + b_slice
    assert np.array_equal(c.values, data + data)
    c += b
    assert np.array_equal(c.values, data + data + data)
    c += b_slice
    assert np.array_equal(c.values, data + data + data + data)
    c = 3.5 + c
    assert np.array_equal(c.values, data + data + data + data + 3.5)


def test_sub_variable() -> None:
    a, b, a_slice, b_slice, data = make_variables()
    c = a - b
    assert np.array_equal(c.values, data - data)
    c = a - 2.0
    assert np.array_equal(c.values, data - 2.0)
    c = a - b_slice
    assert np.array_equal(c.values, data - data)
    c -= b
    assert np.array_equal(c.values, data - data - data)
    c -= b_slice
    assert np.array_equal(c.values, data - data - data - data)
    c = 3.5 - c
    assert np.array_equal(c.values, 3.5 - data + data + data + data)


def test_mul_variable() -> None:
    a, b, a_slice, b_slice, data = make_variables()
    c = a * b
    assert np.array_equal(c.values, data * data)
    c = a * 2.0
    assert np.array_equal(c.values, data * 2.0)
    c = a * b_slice
    assert np.array_equal(c.values, data * data)
    c *= b
    assert np.array_equal(c.values, data * data * data)
    c *= b_slice
    assert np.array_equal(c.values, data * data * data * data)
    c = 3.5 * c
    assert np.array_equal(c.values, data * data * data * data * 3.5)


def test_truediv_variable() -> None:
    a, b, a_slice, b_slice, data = make_variables()
    c = a / b
    assert np.array_equal(c.values, data / data)
    c = a / 2.0
    assert np.array_equal(c.values, data / 2.0)
    c = a / b_slice
    assert np.array_equal(c.values, data / data)
    c /= b
    assert np.array_equal(c.values, data / data / data)
    c /= b_slice
    assert np.array_equal(c.values, data / data / data / data)
    c = 2.0 / a
    assert np.array_equal(c.values, 2.0 / data)


def test_pow_variable() -> None:
    a, b, a_slice, b_slice, data = make_variables()
    c = a**b
    assert np.array_equal(c.values, data**data)
    c **= b
    assert np.array_equal(c.values, (data**data) ** data)
    c = a**3
    assert np.array_equal(c.values, data**3)
    c **= 3
    assert np.array_equal(c.values, (data**3) ** 3)
    c = a**3.0
    assert np.array_equal(c.values, data**3.0)
    c **= 3.0
    assert np.array_equal(c.values, (data**3.0) ** 3.0)
    c = a**b_slice
    assert np.array_equal(c.values, data**data)
    c **= b_slice
    assert np.array_equal(c.values, (data**data) ** data)
    c = 2**b
    assert np.array_equal(c.values, 2**data)
    c = 2.0**b
    assert np.array_equal(c.values, 2.0**data)


def test_iadd_variable_with_scalar() -> None:
    v = sc.Variable(dims=['x'], values=[10.0])
    expected = sc.Variable(dims=['x'], values=[12.0])
    v += 2
    assert sc.identical(v, expected)


def test_isub_variable_with_scalar() -> None:
    v = sc.Variable(dims=['x'], values=[10.0])
    expected = sc.Variable(dims=['x'], values=[9.0])
    v -= 1
    assert sc.identical(v, expected)


def test_imul_variable_with_scalar() -> None:
    v = sc.Variable(dims=['x'], values=[10.0])
    expected = sc.Variable(dims=['x'], values=[30.0])
    v *= 3
    assert sc.identical(v, expected)


def test_itruediv_variable_with_scalar() -> None:
    v = sc.Variable(dims=['x'], values=[10.0])
    expected = sc.Variable(dims=['x'], values=[5.0])
    v /= 2
    assert sc.identical(v, expected)


def test_ifloordiv_variable_with_scalar() -> None:
    v = sc.Variable(dims=['x'], values=[10.0, 7.0])
    expected = sc.Variable(dims=['x'], values=[5.0, 3.0])
    v //= 2
    assert sc.identical(v, expected)


def test_add_dataarray_with_dataarray() -> None:
    da = sc.DataArray(
        sc.arange('x', 1.0, 10.0), coords={'x': sc.arange('x', 10.0, 20.0)}
    )
    expected = sc.zeros_like(da)
    expected.data = sc.arange('x', 2.0, 20.0, 2.0)
    assert sc.identical(da + da, expected)


def test_sub_dataarray_with_dataarray() -> None:
    da = sc.DataArray(
        sc.arange('x', 1.0, 10.0), coords={'x': sc.arange('x', 10.0, 20.0)}
    )
    expected = sc.zeros_like(da)
    assert sc.identical(da - da, expected)


def test_mul_dataarray_with_dataarray() -> None:
    da = sc.DataArray(
        sc.arange('x', 1.0, 10.0), coords={'x': sc.arange('x', 10.0, 20.0)}
    )
    expected = sc.zeros_like(da)
    expected.data = sc.arange('x', 1.0, 10.0) ** 2
    assert sc.identical(da * da, expected)


def test_truediv_dataarray_with_dataarray() -> None:
    da = sc.DataArray(
        sc.arange('x', 1.0, 10.0), coords={'x': sc.arange('x', 10.0, 20.0)}
    )
    expected = sc.ones_like(da)
    assert sc.identical(da / da, expected)


def test_add_dataarray_with_variable() -> None:
    da = sc.DataArray(
        sc.arange('x', 1.0, 10.0), coords={'x': sc.arange('x', 10.0, 20.0)}
    )
    expected = sc.zeros_like(da)
    expected.data = sc.arange('x', 2.0, 20.0, 2.0)
    assert sc.identical(da + da.data, expected)
    assert sc.identical(da.data + da, expected)


def test_sub_dataarray_with_variable() -> None:
    da = sc.DataArray(
        sc.arange('x', 1.0, 10.0), coords={'x': sc.arange('x', 10.0, 20.0)}
    )
    expected = sc.zeros_like(da)
    assert sc.identical(da - da.data, expected)
    assert sc.identical(da.data - da, expected)


def test_mul_dataarray_with_variable() -> None:
    da = sc.DataArray(
        sc.arange('x', 1.0, 10.0), coords={'x': sc.arange('x', 10.0, 20.0)}
    )
    expected = sc.zeros_like(da)
    expected.data = sc.arange('x', 1.0, 10.0) ** 2
    assert sc.identical(da * da.data, expected)
    assert sc.identical(da.data * da, expected)


def test_truediv_dataarray_with_variable() -> None:
    da = sc.DataArray(
        sc.arange('x', 1.0, 10.0), coords={'x': sc.arange('x', 10.0, 20.0)}
    )
    expected = sc.ones_like(da)
    assert sc.identical(da / da.data, expected)
    assert sc.identical(da.data / da, expected)


def test_add_dataarray_with_scalar() -> None:
    da = sc.DataArray(
        sc.arange('x', 1.0, 10.0), coords={'x': sc.arange('x', 10.0, 20.0)}
    )
    expected = sc.zeros_like(da)
    expected.data = sc.arange('x', 3.0, 12.0)
    assert sc.identical(da + 2.0, expected)
    assert sc.identical(2.0 + da, expected)


def test_sub_dataarray_with_scalar() -> None:
    da = sc.DataArray(
        sc.arange('x', 1.0, 10.0), coords={'x': sc.arange('x', 10.0, 20.0)}
    )
    expected = sc.zeros_like(da)
    expected.data = sc.arange('x', 1.0, 10.0) - 2.0
    assert sc.identical(da - 2.0, expected)
    expected.data = 2.0 - sc.arange('x', 1.0, 10.0)
    assert sc.identical(2.0 - da, expected)


def test_mul_dataarray_with_scalar() -> None:
    da = sc.DataArray(
        sc.arange('x', 1.0, 10.0), coords={'x': sc.arange('x', 10.0, 20.0)}
    )
    expected = sc.zeros_like(da)
    expected.data = sc.arange('x', 2.0, 20.0, 2.0)
    assert sc.identical(da * 2.0, expected)
    assert sc.identical(2.0 * da, expected)


def test_truediv_dataarray_with_scalar() -> None:
    da = sc.DataArray(
        sc.arange('x', 1.0, 10.0), coords={'x': sc.arange('x', 10.0, 20.0)}
    )
    expected = sc.zeros_like(da)
    expected.data = sc.arange('x', 1.0, 10.0) / 2.0
    assert sc.identical(da / 2.0, expected)
    expected.data = 2.0 / sc.arange('x', 1.0, 10.0)
    assert sc.identical(2.0 / da, expected)


def test_iadd_dataset_with_dataarray() -> None:
    da = sc.DataArray(
        sc.arange('x', 1.0, 10.0), coords={'x': sc.arange('x', 10.0, 20.0)}
    )
    ds = sc.Dataset({'data': da.copy()})
    expected = sc.Dataset({'data': da + da})
    ds += da
    assert sc.identical(ds, expected)


def test_isub_dataset_with_dataarray() -> None:
    da = sc.DataArray(
        sc.arange('x', 1.0, 10.0), coords={'x': sc.arange('x', 10.0, 20.0)}
    )
    ds = sc.Dataset({'data': da.copy()})
    expected = sc.Dataset({'data': da - da})
    ds -= da
    assert sc.identical(ds, expected)


def test_imul_dataset_with_dataarray() -> None:
    da = sc.DataArray(
        sc.arange('x', 1.0, 10.0), coords={'x': sc.arange('x', 10.0, 20.0)}
    )
    ds = sc.Dataset({'data': da.copy()})
    expected = sc.Dataset({'data': da * da})
    ds *= da
    assert sc.identical(ds, expected)


def test_itruediv_dataset_with_dataarray() -> None:
    da = sc.DataArray(
        sc.arange('x', 1.0, 10.0), coords={'x': sc.arange('x', 10.0, 20.0)}
    )
    ds = sc.Dataset({'data': da.copy()})
    expected = sc.Dataset({'data': da / da})
    ds /= da
    assert sc.identical(ds, expected)


def test_iadd_dataset_with_scalar() -> None:
    ds = sc.Dataset(
        data={'data': sc.arange('x', 10.0)}, coords={'x': sc.arange('x', 10.0, 20.0)}
    )
    expected = ds.copy()
    expected['data'] = ds['data'] + 2.0

    ds += 2.0
    assert sc.identical(ds, expected)


def test_isub_dataset_with_scalar() -> None:
    ds = sc.Dataset(
        data={'data': sc.arange('x', 10.0)}, coords={'x': sc.arange('x', 10.0, 20.0)}
    )
    expected = ds.copy()
    expected['data'] = ds['data'] - 3.0

    ds -= 3.0
    assert sc.identical(ds, expected)


def test_imul_dataset_with_scalar() -> None:
    ds = sc.Dataset(
        data={'data': sc.arange('x', 10.0)}, coords={'x': sc.arange('x', 10.0, 20.0)}
    )
    expected = ds.copy()
    expected['data'] = ds['data'] * 1.5

    ds *= 1.5
    assert sc.identical(ds, expected)


def test_itruediv_dataset_with_scalar() -> None:
    ds = sc.Dataset(
        data={'data': sc.arange('x', 10.0)}, coords={'x': sc.arange('x', 10.0, 20.0)}
    )
    expected = ds.copy()
    expected['data'] = ds['data'] / 0.5

    ds /= 0.5
    assert sc.identical(ds, expected)


def test_isub_dataset_with_dataset_broadcast() -> None:
    ds = sc.Dataset(
        data={'data': sc.arange('x', 10.0)}, coords={'x': sc.arange('x', 10.0, 20.0)}
    )
    expected = ds - ds['x', 0]
    ds -= ds['x', 0]
    assert sc.identical(ds, expected)


def test_add_function() -> None:
    assert sc.identical(sc.add(sc.scalar(3), sc.scalar(2)), sc.scalar(5))


def test_divide_function() -> None:
    assert sc.identical(sc.divide(sc.scalar(6), sc.scalar(2)), sc.scalar(3.0))


def test_floor_divide_function() -> None:
    assert sc.identical(sc.floor_divide(sc.scalar(6), sc.scalar(2.5)), sc.scalar(2.0))


def test_mod_function() -> None:
    assert sc.identical(sc.mod(sc.scalar(3), sc.scalar(2)), sc.scalar(1))


def test_multipy_function() -> None:
    assert sc.identical(sc.multiply(sc.scalar(3), sc.scalar(2)), sc.scalar(6))


def test_subtract_function() -> None:
    assert sc.identical(sc.subtract(sc.scalar(3), sc.scalar(2)), sc.scalar(1))


def test_negative_function() -> None:
    assert sc.identical(sc.negative(sc.scalar(3)), sc.scalar(-3))
