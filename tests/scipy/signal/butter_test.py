# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
import numpy as np
import pytest

import scipp as sc
from scipp.scipy.signal import butter


@pytest.mark.parametrize(
    "coord",
    [
        sc.array(dims=['xx'], values=[1.1, 1.2, 1.3, 1.41, 1.5]),
        sc.geomspace(dim='xx', start=1.0, stop=100.0, num=100),
    ],
)
def test_raises_CoordError_if_not_linspace(coord) -> None:
    with pytest.raises(sc.CoordError):
        butter(coord, N=4, Wn=sc.scalar(4.0))


@pytest.mark.parametrize("unit", ['m', 's'])
def test_raises_UnitError_if_Wn_unit_incompatible(unit) -> None:
    coord = sc.linspace(dim='xx', start=-0.1, stop=4.0, num=100, unit=unit)
    butter(coord, N=4, Wn=sc.scalar(4.0, unit=sc.units.one / unit))
    with pytest.raises(sc.UnitError):
        butter(coord, N=4, Wn=sc.scalar(4.0))
    with pytest.raises(sc.UnitError):
        butter(coord, N=4, Wn=sc.scalar(4.0, unit='kg'))


def test_compatible_Wn_units_are_converted() -> None:
    coord = sc.linspace(dim='xx', start=-0.1, stop=4.0, num=100, unit='ms')
    sos1 = butter(coord, N=4, Wn=sc.scalar(4000.0, unit='1/s'))
    sos2 = butter(coord, N=4, Wn=sc.scalar(4.0, unit='1/ms'))
    sos3 = butter(coord, N=4, Wn=sc.scalar(0.004, unit='1/us'))
    np.testing.assert_array_almost_equal(sos1.sos, sos2.sos)
    np.testing.assert_array_almost_equal(sos1.sos, sos3.sos)
