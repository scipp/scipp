# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
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
    d = sc.Dataset(
        {'sample': sc.array(dims=['y', 'x', 'tof'], values=a, variances=v)},
        coords={
            'tof': sc.array(dims=['tof'], values=x, unit=sc.units.us),
            'x': sc.array(
                dims=['x'], values=np.arange(M).astype(np.float64), unit=sc.units.m
            ),
            'y': sc.array(
                dims=['y'], values=np.arange(L).astype(np.float64), unit=sc.units.m
            ),
        },
    )
    return d


def test_collapse_data_array() -> None:
    d = make_dataset()
    collapsed = sc.collapse(d['sample'], keep='tof')
    assert sc.identical(collapsed['y:0-x:0'], d['sample']['y', 0]['x', 0])
    assert sc.identical(collapsed['y:1-x:0'], d['sample']['y', 1]['x', 0])
    assert sc.identical(collapsed['y:0-x:1'], d['sample']['y', 0]['x', 1])
    assert sc.identical(collapsed['y:1-x:1'], d['sample']['y', 1]['x', 1])


def test_collapse_dataset() -> None:
    d = make_dataset()
    collapsed = sc.collapse(d, keep='tof')
    # In a Dataset, the order of the dims is not fixed
    first_key = next(iter(collapsed.keys()))
    if first_key.startswith('x'):
        dim1 = 'x'
        dim2 = 'y'
    else:
        dim1 = 'y'
        dim2 = 'x'
    assert sc.identical(collapsed[f'{dim1}:0-{dim2}:0'], d[dim2, 0][dim1, 0])
    assert sc.identical(collapsed[f'{dim1}:0-{dim2}:1'], d[dim2, 1][dim1, 0])
    assert sc.identical(collapsed[f'{dim1}:1-{dim2}:0'], d[dim2, 0][dim1, 1])
    assert sc.identical(collapsed[f'{dim1}:1-{dim2}:1'], d[dim2, 1][dim1, 1])


def test_slices_data_array() -> None:
    d = make_dataset()
    collapsed = sc.slices(d['sample'], dim='x')
    assert sc.identical(collapsed['x:0'], d['sample']['x', 0])
    assert sc.identical(collapsed['x:1'], d['sample']['x', 1])


def test_slices_dataset() -> None:
    d = make_dataset()
    collapsed = sc.slices(d, dim='x')
    assert sc.identical(collapsed['x:0'], d['x', 0])
    assert sc.identical(collapsed['x:1'], d['x', 1])
