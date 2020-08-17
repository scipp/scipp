# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import scipp as sc
import numpy as np
import tempfile
import pytest


def check_roundtrip(obj):
    with tempfile.TemporaryDirectory() as path:
        name = f'{path}/test.hdf5'
        obj.to_hdf5(filename=name)
        assert sc.is_equal(sc.io.open_hdf5(filename=name), obj)


x = sc.Variable(dims=['x'], values=np.arange(4.0), unit=sc.units.m)
y = sc.Variable(dims=['y'], values=np.arange(6.0), unit=sc.units.angstrom)
xy = sc.Variable(dims=['y', 'x'],
                 values=np.random.rand(6, 4),
                 variances=np.random.rand(6, 4),
                 unit=sc.units.kg)

array_1d = sc.DataArray(data=x,
                        coords={
                            'x': x,
                            'x2': 2.0 * x
                        },
                        masks={
                            'mask1': sc.less(x, 1.5 * sc.units.m),
                            'mask2': sc.less(x, 2.5 * sc.units.m)
                        },
                        unaligned_coords={
                            'attr1': x,
                            'attr2': 1.2 * sc.units.K
                        })
array_2d = sc.DataArray(data=xy,
                        coords={
                            'x': x,
                            'y': y,
                            'x2': 2.0 * x
                        },
                        masks={
                            'mask1': sc.less(x, 1.5 * sc.units.m),
                            'mask2': sc.less(xy, 0.5 * sc.units.kg)
                        },
                        unaligned_coords={
                            'attr1': xy,
                            'attr2': 1.2 * sc.units.K
                        })


def test_variable_1d():
    check_roundtrip(x)


def test_variable_2d():
    check_roundtrip(xy)


def test_data_array_1d_no_coords():
    a = sc.DataArray(data=x)
    check_roundtrip(a)


def test_data_array_all_units_supported():
    for unit in sc.units.supported_units():
        a = sc.DataArray(data=1.0 * unit)
        check_roundtrip(a)


def test_data_array_1d():
    check_roundtrip(array_1d)


def test_data_array_2d():
    check_roundtrip(array_2d)


def test_data_array_dtype_scipp_container():
    a = sc.DataArray(data=x)
    a.coords['scalar'] = sc.Variable(value=a)
    a.coords['1d'] = sc.Variable(dims=a.dims,
                                 shape=a.shape,
                                 dtype=sc.dtype.DataArray)
    a.coords['1d'].values[0] = sc.DataArray(data=0.0 * sc.units.m)
    a.coords['1d'].values[1] = sc.DataArray(data=1.0 * sc.units.m)
    a.coords['1d'].values[2] = sc.DataArray(data=2.0 * sc.units.m)
    a.coords['1d'].values[3] = sc.DataArray(data=3.0 * sc.units.m)
    a.coords['dataset'] = sc.Variable(value=sc.Dataset({
        'a': array_1d,
        'b': array_2d
    }))
    check_roundtrip(a)


@pytest.mark.parametrize("dtype", [
    sc.dtype.event_list_float64, sc.dtype.event_list_float32,
    sc.dtype.event_list_int64, sc.dtype.event_list_int32
])
def test_data_array_dtype_event_list(dtype):
    events = sc.Variable(dims=['x'], shape=[2], dtype=dtype)
    events['x', 0].values = np.arange(4)
    a = sc.DataArray(data=events)
    check_roundtrip(a)
    check_roundtrip(a['x', 0])


def test_data_array_dtype_string():
    a = sc.DataArray(data=sc.Variable(dims=['x'], values=['abc', 'def']))
    check_roundtrip(a)
    check_roundtrip(a['x', 0])


def test_dataset():
    d = sc.Dataset({'a': array_1d, 'b': array_2d})
    check_roundtrip(d)
