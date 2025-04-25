# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
import pytest

import scipp as sc

_ = pytest.importorskip('numba')


def test_unary() -> None:
    f = sc.elemwise_func(lambda x: x + x)
    var = sc.array(dims=['x'], values=[1.0, 2.0])
    assert sc.identical(f(var), var + var)


def test_binary() -> None:
    f = sc.elemwise_func(lambda x, y: x + y)
    a = sc.array(dims=['x'], values=[1.0, 2.0])
    b = sc.array(dims=['x'], values=[2.0, 3.0])
    assert sc.identical(f(a, b), a + b)


def test_ternary() -> None:
    f = sc.elemwise_func(lambda x, y, z: x + y + z)
    a = sc.array(dims=['x'], values=[1.0, 2.0])
    b = sc.array(dims=['x'], values=[2.0, 3.0])
    c = sc.array(dims=['x'], values=[3.0, 4.0])
    assert sc.identical(f(a, b, c), a + b + c)


def test_4_inputs() -> None:
    f = sc.elemwise_func(lambda x, y, z, t: x + y + z + t)
    a = sc.array(dims=['x'], values=[1.0, 2.0])
    b = sc.array(dims=['x'], values=[2.0, 3.0])
    c = sc.array(dims=['x'], values=[3.0, 4.0])
    d = sc.array(dims=['x'], values=[4.0, 5.0])
    assert sc.identical(f(a, b, c, d), a + b + c + d)


def fmadd(a: sc.Variable, b: sc.Variable, c: sc.Variable) -> sc.Variable:
    return a * b + c


def test_handles_unit_using_same_kernel() -> None:
    f = sc.elemwise_func(fmadd)
    a = 2.0 * sc.Unit('m')
    b = 3.0 * sc.Unit('s')
    c = 4.0 * sc.Unit('m*s')
    assert f(a, b, c).unit == 'm*s'


def test_custom_unit_func_is_used() -> None:
    def unit_func(a: sc.Unit, b: sc.Unit, c: sc.Unit) -> str:
        return 'K'

    f = sc.elemwise_func(fmadd, unit_func=unit_func)
    a = 2.0 * sc.Unit('m')
    b = 3.0 * sc.Unit('s')
    c = 4.0 * sc.Unit('m*s')
    assert f(a, b, c).unit == 'K'


def test_raises_on_dtype_mismatch() -> None:
    a = sc.ones(dims=['x'], shape=[10])
    b = sc.ones(dims=['x'], shape=[10])
    c = sc.ones(dims=['x'], shape=[10])
    f = sc.elemwise_func(fmadd)
    with pytest.raises(TypeError):
        f(a, b, c.to(dtype='int64'))


def test_auto_convert_dtype_handles_dtype_mismatch() -> None:
    a = 2.0 * sc.Unit('m')
    b = 3.0 * sc.Unit('s')
    c = 4.0 * sc.Unit('m*s')
    f = sc.elemwise_func(fmadd, auto_convert_dtypes=True)
    assert sc.identical(f(a, b, c.to(dtype='int64')), a * b + c)


def test_usable_as_decorator() -> None:
    @sc.elemwise_func(unit_func=lambda u: u)
    def add1(a: sc.Variable) -> sc.Variable:
        return a + 1

    assert sc.identical(add1(sc.scalar(1.0)), sc.scalar(2.0))
