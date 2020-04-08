# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import numpy as np
import scipp as sc


def make_dataarray():
    return sc.DataArray(data=sc.Variable(dims=['x'], values=np.ones(4)),
                        coords={
                            'x': sc.Variable(dims=['x'], values=np.ones(4)),
                            'y': sc.Variable(dims=['x'], values=np.arange(4.0))
                        })


def test_realign():
    base = make_dataarray()
    y = sc.Variable(dims=['y'], values=[0.0, 4.0])
    realigned = sc.realign(base, coords={'y': y})
    assert realigned.data is None
    assert realigned.unaligned == base
    expected = sc.DataArray(data=sc.Variable(dims=['y'], values=[4.0]),
                            coords={'y': y})
    assert sc.histogram(realigned) == expected
