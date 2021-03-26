# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

import numpy as np
import scipp as sc
from plot_helper import close


def test_plot_variable():
    v = sc.Variable(['x'], values=np.arange(10.0), unit=sc.units.m)
    close(v.plot().fig)


def test_plot_data_array():
    da = sc.DataArray(data=sc.Variable(['x'], values=np.random.random(10)),
                      coords={
                          'x':
                          sc.Variable(['x'],
                                      values=np.arange(10.0),
                                      unit=sc.units.m)
                      })
    close(da.plot())


def test_plot_dataset():
    N = 100
    ds = sc.Dataset()
    ds.coords['x'] = sc.Variable(['x'], values=np.arange(N), unit=sc.units.m)
    ds['a'] = sc.Variable(['x'], values=np.random.random(N), unit=sc.units.K)
    ds['b'] = sc.Variable(['x'], values=np.random.random(N), unit=sc.units.K)
    close(ds.plot())


def test_plot_data_array_with_kwargs():
    da = sc.DataArray(data=sc.Variable(['y', 'x'],
                                       values=np.random.random([10, 5])),
                      coords={
                          'x':
                          sc.Variable(['x'],
                                      values=np.arange(5.0),
                                      unit=sc.units.m),
                          'y':
                          sc.Variable(['y'],
                                      values=np.arange(10.0),
                                      unit=sc.units.m)
                      })
    close(da.plot(cmap="magma", norm="log"))
