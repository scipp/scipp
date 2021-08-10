# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import scipp as sc
import numpy as np
import tempfile


def roundtrip(obj):
    with tempfile.TemporaryDirectory() as path:
        name = f'{path}/test.hdf5'
        obj.to_hdf5(filename=name)
        return sc.io.open_hdf5(filename=name)


def check_roundtrip(obj):
    result = roundtrip(obj)
    assert sc.identical(result, obj)
    return result  # for optional addition tests


x = sc.Variable(dims=['x'], values=np.arange(4.0), unit=sc.units.m)
y = sc.Variable(dims=['y'], values=np.arange(6.0), unit=sc.units.angstrom)
xy = sc.Variable(dims=['y', 'x'],
                 values=np.random.rand(6, 4),
                 variances=np.random.rand(6, 4),
                 unit=sc.units.kg)
eigen_1d = sc.vectors(dims=['x'], values=np.random.rand(4, 3))

eigen_2d = sc.matrices(dims=['x'], values=np.random.rand(4, 3, 3))

datetime64ms_1d = sc.Variable(dims=['x'],
                              dtype=sc.dtype.datetime64,
                              unit='ms',
                              values=np.arange(10))

datetime64us_1d = sc.Variable(dims=['x'],
                              dtype=sc.dtype.datetime64,
                              unit='us',
                              values=np.arange(10))

array_1d = sc.DataArray(data=x,
                        coords={
                            'x': x,
                            'x2': 2.0 * x
                        },
                        masks={
                            'mask1': sc.less(x, 1.5 * sc.units.m),
                            'mask2': sc.less(x, 2.5 * sc.units.m)
                        },
                        attrs={
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
                        attrs={
                            'attr1': xy,
                            'attr2': 1.2 * sc.units.K
                        })


def test_variable_1d():
    check_roundtrip(x)


def test_variable_2d():
    check_roundtrip(xy)


def test_variable_eigen():
    check_roundtrip(eigen_1d)
    check_roundtrip(eigen_1d['x', 0])
    check_roundtrip(eigen_2d)
    check_roundtrip(eigen_2d['x', 0])


def test_variable_datetime64():
    check_roundtrip(datetime64ms_1d)
    check_roundtrip(datetime64ms_1d['x', 0])
    check_roundtrip(datetime64us_1d)
    check_roundtrip(datetime64us_1d['x', 0])


def test_variable_binned_variable():
    begin = sc.Variable(dims=['y'], values=[0, 3], dtype=sc.dtype.int64)
    end = sc.Variable(dims=['y'], values=[3, 4], dtype=sc.dtype.int64)
    binned = sc.bins(begin=begin, end=end, dim='x', data=x)
    check_roundtrip(binned)


def test_variable_binned_variable_slice():
    begin = sc.Variable(dims=['y'], values=[0, 3], dtype=sc.dtype.int64)
    end = sc.Variable(dims=['y'], values=[3, 4], dtype=sc.dtype.int64)
    binned = sc.bins(begin=begin, end=end, dim='x', data=x)
    # Note the current arbitrary limit is to avoid writing the buffer if it is
    # more than 50% too large. These cutoffs or the entiry mechanism may
    # change in the future, so this test should be adapted. This test does not
    # documented a strict requirement.
    result = check_roundtrip(binned['y', 0])
    assert result.bins.constituents['data'].shape[0] == 4
    result = check_roundtrip(binned['y', 1])
    assert result.bins.constituents['data'].shape[0] == 1
    result = check_roundtrip(binned['y', 1:2])
    assert result.bins.constituents['data'].shape[0] == 1


def test_variable_binned_data_array():
    binned = sc.bins(dim='x', data=array_1d)
    check_roundtrip(binned)


def test_variable_binned_dataset():
    d = sc.Dataset(data={'a': array_1d, 'b': array_1d})
    binned = sc.bins(dim='x', data=d)
    check_roundtrip(binned)


def test_data_array_1d_no_coords():
    a = sc.DataArray(data=x)
    check_roundtrip(a)


def test_data_array_all_units_supported():
    for unit in [
            sc.units.one, sc.units.us, sc.units.angstrom, sc.units.counts,
            sc.units.counts / sc.units.us, sc.units.meV
    ]:
        a = sc.DataArray(data=1.0 * unit)
        check_roundtrip(a)


def test_data_array_1d():
    check_roundtrip(array_1d)


def test_data_array_2d():
    check_roundtrip(array_2d)


def test_data_array_dtype_scipp_container():
    a = sc.DataArray(data=x)
    a.coords['variable'] = sc.scalar(x)
    a.coords['scalar'] = sc.scalar(a)
    a.coords['1d'] = sc.empty(dims=x.dims, shape=x.shape, dtype=sc.dtype.DataArray)
    for i in range(4):
        a.coords['1d'].values[i] = sc.DataArray(float(i) * sc.units.m)
    a.coords['dataset'] = sc.scalar(sc.Dataset(data={'a': array_1d, 'b': array_2d}))
    check_roundtrip(a)


def test_data_array_dtype_string():
    a = sc.DataArray(data=sc.Variable(dims=['x'], values=['abc', 'def']))
    check_roundtrip(a)
    check_roundtrip(a['x', 0])


def test_data_array_unsupported_PyObject_coord():
    obj = sc.scalar(dict())
    a = sc.DataArray(data=x, coords={'obj': obj})
    b = roundtrip(a)
    assert not sc.identical(a, b)
    del a.coords['obj']
    assert sc.identical(a, b)
    a.attrs['obj'] = obj
    assert not sc.identical(a, b)
    del a.attrs['obj']
    assert sc.identical(a, b)


def test_dataset():
    d = sc.Dataset(data={'a': array_1d, 'b': array_2d})
    check_roundtrip(d)
