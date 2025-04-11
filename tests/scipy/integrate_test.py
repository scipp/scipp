# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
import numpy as np
import pytest

import scipp as sc
from scipp.scipy.integrate import simpson, trapezoid


def make_array():
    x = sc.geomspace(dim='xx', start=0.1, stop=0.4, num=10, unit='rad')
    y = sc.linspace(dim='yy', start=0.5, stop=2.0, num=6, unit='m')
    da = sc.DataArray(sc.sin(x) * y, coords={'xx': x, 'yy': y})
    da.unit = 'K'
    return da


@pytest.mark.parametrize('integ', [trapezoid, simpson])
def test_fail_variances(integ) -> None:
    da = make_array()
    da.variances = da.values
    with pytest.raises(sc.VariancesError):
        integ(da, 'xx')


@pytest.mark.parametrize('integ', [trapezoid, simpson])
def test_fail_bin_edges(integ) -> None:
    tmp = make_array()
    da = tmp['xx', 1:].copy()
    da.coords['xx'] = tmp.coords['xx']
    with pytest.raises(sc.BinEdgeError):
        integ(da, 'xx')


@pytest.mark.parametrize('integ', [trapezoid, simpson])
def test_unit(integ) -> None:
    da = make_array()
    assert integ(da, 'xx').unit == da.unit * da.coords['xx'].unit
    assert integ(da, 'yy').unit == da.unit * da.coords['yy'].unit


def make_periodic():
    x = sc.linspace(dim='xx', start=0.0, stop=4 * np.pi, num=1000, unit='rad')
    y = sc.linspace(dim='yy', start=0.0, stop=4 * np.pi, num=1000, unit='rad')
    return sc.DataArray(sc.sin(x) + sc.sin(y), coords={'xx': x, 'yy': y})


@pytest.mark.parametrize('integ1', [trapezoid, simpson])
@pytest.mark.parametrize('integ2', [trapezoid, simpson])
def test_zero_integral(integ1, integ2) -> None:
    da = make_periodic()
    assert sc.allclose(
        integ1(integ2(da, 'xx'), 'yy').data,
        sc.scalar(0.0, unit='sr'),
        atol=1e-7 * sc.Unit('sr'),
    )


@pytest.mark.parametrize('integ', [trapezoid, simpson])
def test_constant(integ) -> None:
    x = sc.array(dims=['xx'], values=[2.0, 5.0])
    da = sc.DataArray(data=sc.full_like(x, 1.2), coords={'xx': x})
    expected = da['xx', 0].copy()
    expected.data *= 3.0
    del expected.coords['xx']
    assert sc.identical(integ(da, 'xx'), expected)


@pytest.mark.parametrize('split', range(1, 9))
def test_trapzoid_sections(split) -> None:
    da = make_periodic()
    assert sc.allclose(
        trapezoid(da['xx', : split + 1], 'xx').data
        + trapezoid(da['xx', split:], 'xx').data,
        trapezoid(da, 'xx').data,
    )
