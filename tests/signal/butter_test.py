# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
import numpy as np
import scipp as sc
from scipp.signal import butter

import pytest


def array1d_linspace(*, coord=None):
    if coord is None:
        dim = 'xx'
        size = 100
        coord = sc.linspace(dim=dim, start=-0.1, stop=4.0, num=size, unit='m')
    else:
        dim = coord.dim
        size = len(coord)
    y = coord.copy()
    y.unit = 'rad'
    y = sc.sin(y)
    rng = np.random.default_rng()
    y.values += 0.1 * rng.normal(size=size)
    return sc.DataArray(y, coords={dim: coord})


@pytest.mark.parametrize("coord", [
    sc.array(dims=['xx'], values=[1.1, 1.2, 1.3, 1.41, 1.5]),
    sc.geomspace(dim='xx', start=1.0, stop=100.0, num=100)
])
def test_raises_CoordError_if_not_linspace(coord):
    da = array1d_linspace(coord=coord)
    with pytest.raises(sc.CoordError):
        butter(da, coord.dim, N=4, Wn=sc.scalar(4.0))


@pytest.mark.parametrize("unit", ['m', 's'])
def test_raises_UnitError_if_Wn_unit_incompatible(unit):
    da = array1d_linspace()
    da.coords[da.dim].unit = unit
    butter(da, da.dim, N=4, Wn=sc.scalar(4.0, unit=sc.units.one / unit))
    with pytest.raises(sc.UnitError):
        butter(da, da.dim, N=4, Wn=sc.scalar(4.0))
    with pytest.raises(sc.UnitError):
        butter(da, da.dim, N=4, Wn=sc.scalar(4.0, unit='kg'))


def test_compatible_Wn_units_are_converted():
    da = array1d_linspace()
    da.coords[da.dim].unit = 'ms'
    sos1 = butter(da, da.dim, N=4, Wn=sc.scalar(4000.0, unit='1/s'))
    sos2 = butter(da, da.dim, N=4, Wn=sc.scalar(4.0, unit='1/ms'))
    sos3 = butter(da, da.dim, N=4, Wn=sc.scalar(0.004, unit='1/us'))
    np.testing.assert_array_almost_equal(sos1.sos, sos2.sos)
    np.testing.assert_array_almost_equal(sos1.sos, sos3.sos)
