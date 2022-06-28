# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

import numpy as np
import scipp as sc


def test_plot_variable():
    v = sc.arange('x', 10.0, unit='m')
    v.plot()


def test_plot_data_array():
    da = sc.DataArray(data=sc.Variable(dims=['x'], values=np.random.random(10)),
                      coords={'x': sc.arange('x', 10.0, unit='m')})
    da.plot()


def test_plot_dataset():
    N = 100
    ds = sc.Dataset()
    ds['a'] = sc.Variable(dims=['x'], values=np.random.random(N), unit=sc.units.K)
    ds['b'] = sc.Variable(dims=['x'], values=np.random.random(N), unit=sc.units.K)
    ds.coords['x'] = sc.arange('x', float(N), unit='m')
    ds.plot()


def test_plot_data_array_with_kwargs():
    da = sc.DataArray(data=sc.Variable(dims=['y', 'x'],
                                       values=np.random.random([10, 5])),
                      coords={
                          'x': sc.arange('x', 5.0, unit='m'),
                          'y': sc.arange('y', 10.0, unit='m')
                      })
    da.plot(cmap="magma", norm="log")
