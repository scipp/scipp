# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
import pytest

import scipp as sc
from scipp.compat.wrapping import wrap1d


@wrap1d()
def func1d(da, dim, **kwargs):
    assert kwargs['axis'] == da.dims.index(dim)
    return da[dim, 1:3].copy()


@wrap1d(is_partial=True)
def factory1d(da, dim, **kwargs):
    def func(arg):
        assert kwargs['axis'] == da.dims.index(dim)
        out = da[dim, 1:3].copy()
        out.coords[dim] = arg
        return out

    return func


def make_array():
    x = sc.geomspace(dim='xx', start=0.1, stop=0.4, num=10, unit='rad')
    y = sc.linspace(dim='yy', start=0.5, stop=2.0, num=6, unit='m')
    da = sc.DataArray(sc.sin(x) * y, coords={'xx': x, 'yy': y, 'scalar': y[0]})
    da.unit = 'K'
    mask = y.copy()
    mask.unit = ''
    da.masks['yy'] = mask < mask**2
    return da


def check_metadata(out, da, x):
    assert out.unit == da.unit
    if x is not None:
        assert sc.identical(out.coords['xx'], x)
    assert sc.identical(out.coords['yy'], da.coords['yy'])
    assert sc.identical(out.coords['scalar'], da.coords['scalar'])
    assert sc.identical(out.masks['yy'], da.masks['yy'])
    out.masks['yy'] ^= out.masks['yy']
    assert not sc.identical(out.masks['yy'], da.masks['yy'])


@pytest.mark.parametrize(
    "da", [make_array(), make_array().transpose(), make_array().transpose().copy()]
)
def test_wrap1d_metadata_factory1d(da):
    f = factory1d(da, 'xx')
    x = sc.linspace(dim='xx', start=0.1, stop=0.4, num=2, unit='rad')
    check_metadata(f(x), da, x)


@pytest.mark.parametrize(
    "da", [make_array(), make_array().transpose(), make_array().transpose().copy()]
)
def test_wrap1d_metadata_func1d(da):
    check_metadata(func1d(da, dim='xx'), da, x=None)


@pytest.mark.parametrize('func', [factory1d, func1d])
def test_wrap1d_fail_axis_given(func):
    with pytest.raises(ValueError, match='dim.*axis'):
        func(make_array(), axis=0, dim='xx')


@pytest.mark.parametrize('func', [factory1d, func1d])
def test_wrap1d_fail_variances(func):
    da = make_array()
    da.variances = da.values
    with pytest.raises(sc.VariancesError):
        func(da, 'xx')


@pytest.mark.parametrize('func', [factory1d, func1d])
def test_wrap1d_fail_bin_edges(func):
    tmp = make_array()
    da = tmp['xx', 1:].copy()
    da.coords['xx'] = tmp.coords['xx']
    with pytest.raises(sc.BinEdgeError):
        func(da, 'xx')


@pytest.mark.parametrize('func', [factory1d, func1d])
def test_wrap1d_fail_conflicting_mask(func):
    da = make_array()
    da.masks['xx'] = da.coords['xx'] != da.coords['xx']
    with pytest.raises(sc.DimensionError):
        func(da, 'xx')


@wrap1d(accept_masks=True)
def func1d_with_mask(da, dim, **kwargs):
    assert kwargs['axis'] == da.dims.index(dim)
    assert dim in da.masks
    return da[dim, 1:3].copy()


def test_wrap1d_accept_masks():
    da = make_array()
    da.masks['xx'] = da.coords['xx'] != da.coords['xx']
    result = func1d_with_mask(da, 'xx')
    assert sc.identical(result, da['xx', 1:3])
    check_metadata(result, da, x=None)
