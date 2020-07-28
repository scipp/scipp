# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

import numpy as np
import scipp as sc


def make_dataset():
    N = 40
    M = 2
    L = 2
    x = np.arange(N).astype(np.float64)
    b = 0.5 * N
    a = 4.0 * np.random.random([L, M, N])
    a[1, 1, :] = np.abs(10.0 * np.cos((x - b) * 2.0 / b))
    v = 0.1 * np.random.random([L, M, N])
    d = sc.Dataset()
    d.coords['tof'] = sc.Variable(['tof'], values=x, unit=sc.units.us)
    d.coords['x'] = sc.Variable(['x'],
                                values=np.arange(M).astype(np.float64),
                                unit=sc.units.m)
    d.coords['y'] = sc.Variable(['y'],
                                values=np.arange(L).astype(np.float64),
                                unit=sc.units.m)
    d['sample'] = sc.Variable(['y', 'x', 'tof'], values=a, variances=v)
    return d


def test_collapse_data_array():
    d = make_dataset()
    collapsed = sc.collapse(d['sample'], keep='tof')
    assert collapsed['y:0-x:0'] == d['sample']['y', 0]['x', 0]
    assert collapsed['y:1-x:0'] == d['sample']['y', 1]['x', 0]
    assert collapsed['y:0-x:1'] == d['sample']['y', 0]['x', 1]
    assert collapsed['y:1-x:1'] == d['sample']['y', 1]['x', 1]


def test_collapse_dataset():
    d = make_dataset()
    collapsed = sc.collapse(d, keep='tof')
    # In a Dataset, the order of the dims is not fixed
    first_key = list(collapsed.keys())[0]
    if first_key.startswith('x'):
        dim1 = 'x'
        dim2 = 'y'
    else:
        dim1 = 'y'
        dim2 = 'x'
    assert collapsed['{}:0-{}:0'.format(dim1, dim2)] == d[dim2, 0][dim1, 0]
    assert collapsed['{}:0-{}:1'.format(dim1, dim2)] == d[dim2, 1][dim1, 0]
    assert collapsed['{}:1-{}:0'.format(dim1, dim2)] == d[dim2, 0][dim1, 1]
    assert collapsed['{}:1-{}:1'.format(dim1, dim2)] == d[dim2, 1][dim1, 1]


def test_slices_data_array():
    d = make_dataset()
    collapsed = sc.slices(d['sample'], dim='x')
    assert collapsed['x:0'] == d['sample']['x', 0]
    assert collapsed['x:1'] == d['sample']['x', 1]


def test_slices_dataset():
    d = make_dataset()
    collapsed = sc.slices(d, dim='x')
    assert collapsed['x:0'] == d['x', 0]
    assert collapsed['x:1'] == d['x', 1]
