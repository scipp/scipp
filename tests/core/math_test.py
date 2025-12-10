# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen
from collections.abc import Callable
from typing import TypeAlias

import numpy as np
import pytest
import scipy

import scipp as sc

UnaryNumpyFunc: TypeAlias = Callable[[float], float]
UnaryScippFunc: TypeAlias = Callable[[sc.Variable], sc.Variable]


@pytest.mark.parametrize(
    'funcs',
    [
        (sc.erf, scipy.special.erf),
        (sc.erfc, scipy.special.erfc),
        (sc.exp, np.exp),
        (sc.log, np.log),
        (sc.log10, np.log10),
        (sc.sqrt, np.sqrt),
        (sc.sinh, np.sinh),
        (sc.cosh, np.cosh),
        (sc.tanh, np.tanh),
        (sc.asinh, np.arcsinh),
        (sc.atanh, np.arctanh),
    ],
)
def test_unary_math_compare_to_numpy_dimensionless(
    funcs: tuple[UnaryScippFunc, UnaryNumpyFunc],
) -> None:
    sc_f, ref = funcs
    assert sc.allclose(sc_f(sc.scalar(0.512)), sc.scalar(ref(0.512)))


def test_unary_math_compare_to_numpy_dimensionless_acosh() -> None:
    # Cannot use the value in the generic function
    assert sc.allclose(sc.acosh(sc.scalar(1.612)), sc.scalar(np.arccosh(1.612)))


@pytest.mark.parametrize('func', [sc.exp, sc.log, sc.log10, sc.sqrt, sc.sinh])
def test_unary_math_out(func: Callable[..., sc.Variable]) -> None:
    out = sc.scalar(np.nan)
    func(sc.scalar(0.932), out=out)
    assert sc.identical(out, func(sc.scalar(0.932)))


@pytest.mark.parametrize(
    'funcs', [(sc.sin, np.sin), (sc.cos, np.cos), (sc.tan, np.tan)]
)
def test_compare_unary_math_to_numpy_trigonometry(
    funcs: tuple[UnaryScippFunc, UnaryNumpyFunc],
) -> None:
    sc_f, ref = funcs
    assert sc.allclose(sc_f(sc.scalar(0.512, unit='rad')), sc.scalar(ref(0.512)))


@pytest.mark.parametrize('func', [sc.sin, sc.cos, sc.tan])
def test_unary_math_trigonometry_out(
    func: Callable[..., sc.Variable],
) -> None:
    out = sc.scalar(np.nan)
    func(sc.scalar(0.932, unit='rad'), out=out)
    assert sc.identical(out, func(sc.scalar(0.932, unit='rad')))


@pytest.mark.parametrize(
    'funcs', [(sc.asin, np.arcsin), (sc.acos, np.arccos), (sc.atan, np.arctan)]
)
def test_compare_unary_math_to_numpy_inv_trigonometry(
    funcs: tuple[UnaryScippFunc, UnaryNumpyFunc],
) -> None:
    sc_f, ref = funcs
    assert sc.allclose(sc_f(sc.scalar(0.512)), sc.scalar(ref(0.512), unit='rad'))


@pytest.mark.parametrize('func', [sc.asin, sc.acos, sc.atan])
def test_unary_math_inv_trigonometry_out(
    func: Callable[..., sc.Variable],
) -> None:
    out = sc.scalar(np.nan, unit='rad')
    func(sc.scalar(0.932), out=out)
    assert sc.identical(out, func(sc.scalar(0.932)))


@pytest.mark.parametrize(
    ('func', 'inp', 'expected'),
    [
        (sc.sqrt, sc.Unit('m^2'), sc.Unit('m')),
    ],
)
def test_unary_math_unit(
    func: Callable[[sc.Unit], sc.Unit], inp: sc.Unit, expected: sc.Unit
) -> None:
    assert func(inp) == expected


def test_abs() -> None:
    assert sc.identical(sc.abs(sc.scalar(-72)), sc.scalar(72))
    assert sc.identical(abs(sc.scalar(-72)), sc.scalar(72))
    assert sc.abs(sc.Unit('m')) == sc.Unit('m')
    assert abs(sc.Unit('m')) == sc.Unit('m')


def test_abs_out() -> None:
    out = sc.scalar(0)
    sc.abs(sc.scalar(-5), out=out)
    assert sc.identical(out, sc.scalar(5))


def test_cross() -> None:
    assert sc.identical(
        sc.cross(sc.vector([0, 0, 1]), sc.vector([0, 1, 0])), sc.vector([-1, 0, 0])
    )


def test_dot() -> None:
    assert sc.identical(
        sc.dot(sc.vector([1, 0, 2]), sc.vector([0, 1, 3])), sc.scalar(6.0)
    )


def test_midpoints() -> None:
    assert sc.allclose(
        sc.midpoints(sc.array(dims=['xy'], values=[0.0, 1.0])),
        sc.array(dims=['xy'], values=[0.5]),
    )
    assert sc.allclose(
        sc.midpoints(sc.array(dims=['xy'], values=[0.0, 1.0]), dim='xy'),
        sc.array(dims=['xy'], values=[0.5]),
    )


def test_midpoints_vector() -> None:
    assert sc.allclose(
        sc.midpoints(sc.vectors(dims=['xy'], values=[[2, 2, 3], [5, 6, 7]])),
        sc.vectors(dims=['xy'], values=[[3.5, 4, 5]]),
    )


def test_norm() -> None:
    assert sc.allclose(sc.norm(sc.vector([1.0, 2.0, 0.0])), sc.scalar(np.sqrt(1 + 4)))


def test_pow() -> None:
    assert sc.allclose(sc.pow(sc.scalar(2), sc.scalar(2)), sc.scalar(4))
    assert sc.allclose(sc.pow(sc.scalar(2), 3), sc.scalar(8))
    assert sc.pow(sc.Unit('m'), 2.0) == sc.Unit('m^2')
    assert sc.pow(sc.Unit('m'), 2) == sc.Unit('m^2')


def test_atan2() -> None:
    assert sc.allclose(
        sc.atan2(y=sc.scalar(0.5), x=sc.scalar(1.2)),
        sc.scalar(np.arctan2(0.5, 1.2), unit='rad'),
    )


def test_atan2_out() -> None:
    out = sc.scalar(np.nan)
    sc.atan2(y=sc.scalar(0.5), x=sc.scalar(1.2), out=out)
    assert sc.allclose(out, sc.scalar(np.arctan2(0.5, 1.2), unit='rad'))


def test_reciprocal() -> None:
    assert sc.identical(sc.reciprocal(sc.scalar(2.0)), sc.scalar(0.5))
    assert sc.reciprocal(sc.units.m) == sc.units.one / sc.units.m


def test_reciprocal_out() -> None:
    out = sc.scalar(np.nan)
    sc.reciprocal(sc.scalar(2.0), out=out)
    assert sc.identical(out, sc.scalar(0.5))


def test_round() -> None:
    x = sc.array(dims=['x'], values=(1.1, 1.5, 2.5, 4.7))
    expected = sc.array(dims=['x'], values=(1.0, 2.0, 2.0, 5.0))
    assert sc.identical(sc.round(x), expected)

    x_out = sc.zeros_like(expected)
    sc.round(x, out=x_out)
    assert sc.identical(x_out, expected)


def test_round_decimals() -> None:
    # Test positive decimals
    x = sc.array(dims=['x'], values=(1.567, 2.345, 3.899))
    expected_2 = sc.array(dims=['x'], values=(1.57, 2.35, 3.90))
    assert sc.identical(sc.round(x, decimals=2), expected_2)

    expected_1 = sc.array(dims=['x'], values=(1.6, 2.3, 3.9))
    assert sc.identical(sc.round(x, decimals=1), expected_1)

    # Test decimals=0 is equivalent to default
    expected_0 = sc.array(dims=['x'], values=(2.0, 2.0, 4.0))
    assert sc.identical(sc.round(x, decimals=0), expected_0)

    # Test negative decimals
    y = sc.array(dims=['x'], values=(12345.0, 67890.0))
    expected_neg2 = sc.array(dims=['x'], values=(12300.0, 67900.0))
    assert sc.identical(sc.round(y, decimals=-2), expected_neg2)

    # Test scalar
    s = sc.scalar(1.567)
    assert sc.identical(sc.round(s, decimals=2), sc.scalar(1.57))


def test_round_decimals_out() -> None:
    x = sc.array(dims=['x'], values=(1.567, 2.345, 3.899))
    expected = sc.array(dims=['x'], values=(1.57, 2.35, 3.90))

    x_out = sc.zeros_like(x)
    sc.round(x, decimals=2, out=x_out)
    assert sc.identical(x_out, expected)


def test_ceil() -> None:
    x = sc.array(dims=['x'], values=(1.1, 1.5, 2.5, 4.7))
    expected = sc.array(dims=['x'], values=(2.0, 2.0, 3.0, 5.0))
    assert sc.identical(sc.ceil(x), expected)

    x_out = sc.zeros_like(expected)
    sc.ceil(x, out=x_out)
    assert sc.identical(x_out, expected)


def test_floor() -> None:
    x = sc.array(dims=['x'], values=(1.1, 1.5, 2.5, 4.7))
    expected = sc.array(dims=['x'], values=(1.0, 1.0, 2.0, 4.0))
    assert sc.identical(sc.floor(x), expected)

    x_out = sc.zeros_like(expected)
    sc.floor(x, out=x_out)
    assert sc.identical(x_out, expected)
