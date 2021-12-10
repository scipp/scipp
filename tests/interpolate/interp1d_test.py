# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
from itertools import product
import numpy as np
import scipp as sc
import scipy.interpolate as theirs
from scipp.interpolate import interp1d

import pytest


def make_array():
    x = sc.geomspace(dim='xx', start=0.1, stop=0.4, num=10, unit='rad')
    y = sc.linspace(dim='yy', start=0.5, stop=2.0, num=6, unit='m')
    da = sc.DataArray(sc.sin(x) * y, coords={'xx': x, 'yy': y, 'scalar': y[0]})
    da.unit = 'K'
    mask = y.copy()
    mask.unit = ''
    da.masks['yy'] = mask < mask**2
    da.attrs['xx'] = x
    da.attrs['yy'] = y
    da.attrs['scalar'] = x[0]
    return da


def check_metadata(out, da, x):
    assert out.unit == da.unit
    assert sc.identical(out.coords['xx'], x)
    assert sc.identical(out.coords['yy'], da.coords['yy'])
    assert sc.identical(out.coords['scalar'], da.coords['scalar'])
    assert sc.identical(out.masks['yy'], da.masks['yy'])
    assert sc.identical(out.attrs['yy'], da.attrs['yy'])
    assert sc.identical(out.attrs['scalar'], da.attrs['scalar'])
    out.masks['yy'] ^= out.masks['yy']
    assert not sc.identical(out.masks['yy'], da.masks['yy'])


@pytest.mark.parametrize(
    "da", [make_array(),
           make_array().transpose(),
           make_array().transpose().copy()])
def test_metadata(da):
    f = interp1d(da, 'xx')
    x = sc.linspace(dim='xx', start=0.1, stop=0.4, num=10, unit='rad')
    check_metadata(f(x), da, x)
    check_metadata(f(x[:5]), da, x[:5])


def test_fail_axis_given():
    with pytest.raises(ValueError):
        interp1d(make_array(), axis=0, dim='xx')


def test_fail_variances():
    da = make_array()
    da.variances = da.values
    with pytest.raises(sc.VariancesError):
        interp1d(da, 'xx')


def test_fail_conflicting_mask():
    da = make_array()
    da.masks['xx'] = da.coords['xx'] != da.coords['xx']
    with pytest.raises(sc.DimensionError):
        interp1d(da, 'xx')


def test_data():
    da = make_array()
    x = sc.linspace(dim='xx', start=0.1, stop=0.4, num=10, unit='rad')
    out = interp1d(da, 'xx')(x)
    assert np.array_equal(
        out.values,
        theirs.interp1d(x=da.coords['xx'].values, y=da.values, axis=0)(x.values))
    da = da.transpose()
    out = interp1d(da, 'xx')(x)
    assert np.array_equal(
        out.values,
        theirs.interp1d(x=da.coords['xx'].values, y=da.values, axis=1)(x.values))
    da = da.copy()
    out = interp1d(da, 'xx')(x)
    assert np.array_equal(
        out.values,
        theirs.interp1d(x=da.coords['xx'].values, y=da.values, axis=1)(x.values))


def test_midpoints():
    da = make_array()
    x = sc.linspace(dim='xx', start=0.1, stop=0.4, num=10, unit='rad')
    out = interp1d(da, 'xx')(x, midpoints=True)
    midpoints = (0.5 * (x[1:] + x[:-1])).values
    assert np.array_equal(
        out.values,
        theirs.interp1d(x=da.coords['xx'].values, y=da.values, axis=0)(midpoints))


@pytest.mark.parametrize("kind,fill_value",
                         list(
                             product(['nearest', 'quadratic', 'cubic'],
                                     [0.0, 'extrapolate'])))
def test_options(kind, fill_value):
    da = make_array()
    x = sc.linspace(dim='xx', start=0.1, stop=0.4, num=10, unit='rad')
    out = interp1d(da, 'xx', kind=kind, fill_value=fill_value)(x)
    assert np.array_equal(
        out.values,
        theirs.interp1d(x=da.coords['xx'].values,
                        y=da.values,
                        axis=0,
                        kind=kind,
                        fill_value=fill_value)(x.values))
