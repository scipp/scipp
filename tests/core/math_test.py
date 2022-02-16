# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen
import scipp as sc

from ..common import assert_export


def test_abs():
    assert_export(sc.abs, sc.scalar(0.0))
    assert_export(abs, sc.scalar(0.0))
    assert_export(sc.abs, sc.units.one)


def test_abs_out():
    var = sc.scalar(0.0)
    assert_export(sc.abs, var, out=var)


def test_cross():
    assert_export(sc.cross, sc.vector(value=[0, 0, 1]), sc.vector(value=[0, 0, 1]))


def test_dot():
    assert_export(sc.dot, sc.vector(value=[0, 0, 1]), sc.vector(value=[0, 0, 1]))


def test_erf():
    assert_export(sc.erf, sc.scalar(0.0))


def test_erfc():
    assert_export(sc.erfc, sc.scalar(0.0))


def test_exp():
    assert_export(sc.exp, sc.scalar(0.0))


def test_log():
    assert_export(sc.log, sc.scalar(0.0))


def test_log10():
    assert_export(sc.log10, sc.scalar(0.0))


def test_midpoints():
    assert_export(sc.midpoints, sc.array(dims=['x'], values=[0.0, 1.0]))
    assert_export(sc.midpoints, sc.array(dims=['x'], values=[0.0, 1.0]), dim='x')


def test_norm():
    assert_export(sc.norm, sc.vector(value=[0.0, 0.0, 0.0]))


def test_pow():
    assert_export(sc.pow, sc.scalar(0.0), sc.scalar(0.0))
    assert_export(sc.pow, sc.scalar(0.0), 0.0)
    assert_export(sc.pow, sc.units.one, 0.0)


def test_reciprocal():
    assert_export(sc.reciprocal, sc.scalar(0.0))
    assert_export(sc.reciprocal, sc.units.one)


def test_reciprocal_out():
    var = sc.scalar(0.0)
    assert_export(sc.reciprocal, var, out=var)


def test_sqrt():
    assert_export(sc.sqrt, sc.scalar(0.0))
    assert_export(sc.sqrt, sc.units.one)


def test_sqrt_out():
    var = sc.scalar(0.0)
    assert_export(sc.sqrt, var, out=var)


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
