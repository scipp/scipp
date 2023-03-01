# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen
import numpy as np
import pytest
import scipy

import scipp as sc


@pytest.mark.parametrize('funcs',
                         ((sc.erf, scipy.special.erf), (sc.erfc, scipy.special.erfc),
                          (sc.exp, np.exp), (sc.log, np.log), (sc.log10, np.log10),
                          (sc.sqrt, np.sqrt)))
def test_unary_math_compare_to_numpy_dimensionless(funcs):
    sc_f, ref = funcs
    assert sc.allclose(sc_f(sc.scalar(0.512)), sc.scalar(ref(0.512)))


@pytest.mark.parametrize('func', (sc.exp, sc.log, sc.log10, sc.sqrt))
def test_unary_math_out(func):
    out = sc.scalar(np.nan)
    func(sc.scalar(0.932), out=out)
    assert sc.identical(out, func(sc.scalar(0.932)))


@pytest.mark.parametrize('funcs',
                         ((sc.sin, np.sin), (sc.cos, np.cos), (sc.tan, np.tan)))
def test_compare_unary_math_to_numpy_trigonometry(funcs):
    sc_f, ref = funcs
    assert sc.allclose(sc_f(sc.scalar(0.512, unit='rad')), sc.scalar(ref(0.512)))


@pytest.mark.parametrize('func', (sc.sin, sc.cos, sc.tan))
def test_unary_math_trigonometry_out(func):
    out = sc.scalar(np.nan)
    func(sc.scalar(0.932, unit='rad'), out=out)
    assert sc.identical(out, func(sc.scalar(0.932, unit='rad')))


@pytest.mark.parametrize('funcs',
                         ((sc.asin, np.arcsin), (sc.acos, np.arccos),
                          (sc.atan, np.arctan)))
def test_compare_unary_math_to_numpy_inv_trigonometry(funcs):
    sc_f, ref = funcs
    assert sc.allclose(sc_f(sc.scalar(0.512)), sc.scalar(ref(0.512), unit='rad'))


@pytest.mark.parametrize('func', (sc.asin, sc.acos, sc.atan))
def test_unary_math_inv_trigonometry_out(func):
    out = sc.scalar(np.nan, unit='rad')
    func(sc.scalar(0.932), out=out)
    assert sc.identical(out, func(sc.scalar(0.932)))


@pytest.mark.parametrize('args', ((sc.sqrt, sc.Unit('m^2'), sc.Unit('m')), ))
def test_unary_math_unit(args):
    func, inp, expected = args
    assert func(inp) == expected


def test_abs():
    assert sc.identical(sc.abs(sc.scalar(-72)), sc.scalar(72))
    assert sc.identical(abs(sc.scalar(-72)), sc.scalar(72))
    assert sc.abs(sc.Unit('m')) == sc.Unit('m')
    assert abs(sc.Unit('m')) == sc.Unit('m')


def test_abs_out():
    out = sc.scalar(0)
    sc.abs(sc.scalar(-5), out=out)
    assert sc.identical(out, sc.scalar(5))


def test_cross():
    assert sc.identical(sc.cross(sc.vector([0, 0, 1]), sc.vector([0, 1, 0])),
                        sc.vector([-1, 0, 0]))


def test_dot():
    assert sc.identical(sc.dot(sc.vector([1, 0, 2]), sc.vector([0, 1, 3])),
                        sc.scalar(6.0))


def test_midpoints():
    assert sc.allclose(sc.midpoints(sc.array(dims=['xy'], values=[0.0, 1.0])),
                       sc.array(dims=['xy'], values=[0.5]))
    assert sc.allclose(sc.midpoints(sc.array(dims=['xy'], values=[0.0, 1.0]), dim='xy'),
                       sc.array(dims=['xy'], values=[0.5]))


def test_norm():
    assert sc.allclose(sc.norm(sc.vector([1.0, 2.0, 0.0])), sc.scalar(np.sqrt(1 + 4)))


def test_pow():
    assert sc.allclose(sc.pow(sc.scalar(2), sc.scalar(2)), sc.scalar(4))
    assert sc.allclose(sc.pow(sc.scalar(2), 3), sc.scalar(8))
    assert sc.pow(sc.Unit('m'), 2.0) == sc.Unit('m^2')
    assert sc.pow(sc.Unit('m'), 2) == sc.Unit('m^2')


def test_atan2():
    assert sc.allclose(sc.atan2(y=sc.scalar(0.5), x=sc.scalar(1.2)),
                       sc.scalar(np.arctan2(0.5, 1.2), unit='rad'))


def test_atan2_out():
    out = sc.scalar(np.nan)
    sc.atan2(y=sc.scalar(0.5), x=sc.scalar(1.2), out=out)
    assert sc.allclose(out, sc.scalar(np.arctan2(0.5, 1.2), unit='rad'))


def test_reciprocal():
    assert sc.identical(sc.reciprocal(sc.scalar(2.0)), sc.scalar(0.5))
    assert sc.reciprocal(sc.units.m) == sc.units.one / sc.units.m


def test_reciprocal_out():
    out = sc.scalar(np.nan)
    sc.reciprocal(sc.scalar(2.0), out=out)
    assert sc.identical(out, sc.scalar(0.5))


def test_round():
    x = sc.array(dims=['x'], values=(1.1, 1.5, 2.5, 4.7))
    expected = sc.array(dims=['x'], values=(1., 2., 2., 5.))
    assert sc.identical(sc.round(x), expected)

    x_out = sc.zeros_like(expected)
    sc.round(x, out=x_out)
    assert sc.identical(x_out, expected)


def test_ceil():
    x = sc.array(dims=['x'], values=(1.1, 1.5, 2.5, 4.7))
    expected = sc.array(dims=['x'], values=(2., 2., 3., 5.))
    assert sc.identical(sc.ceil(x), expected)

    x_out = sc.zeros_like(expected)
    sc.ceil(x, out=x_out)
    assert sc.identical(x_out, expected)


def test_floor():
    x = sc.array(dims=['x'], values=(1.1, 1.5, 2.5, 4.7))
    expected = sc.array(dims=['x'], values=(1., 1., 2., 4.))
    assert sc.identical(sc.floor(x), expected)

    x_out = sc.zeros_like(expected)
    sc.floor(x, out=x_out)
    assert sc.identical(x_out, expected)
