# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import os
import tempfile

import numpy as np
import pytest

import scipp as sc
from scipp.io.hdf5 import collection_element_name

h5py = pytest.importorskip('h5py')


def roundtrip(obj):
    with tempfile.TemporaryDirectory() as path:
        name = f'{path}/test.hdf5'
        obj.save_hdf5(filename=name)
        return sc.io.load_hdf5(filename=name)


def check_roundtrip(obj):
    result = roundtrip(obj)
    assert sc.identical(result, obj)
    return result  # for optional addition tests


x = sc.Variable(dims=['x'], values=np.arange(4.0), unit=sc.units.m)
y = sc.Variable(dims=['y'], values=np.arange(6.0), unit=sc.units.angstrom)
xy = sc.Variable(
    dims=['y', 'x'],
    values=np.random.rand(6, 4),
    variances=np.random.rand(6, 4),
    unit=sc.units.kg,
)
vector = sc.vectors(dims=['x'], values=np.random.rand(4, 3))

matrix = sc.spatial.linear_transforms(dims=['x'], values=np.random.rand(4, 3, 3))

rotation = sc.spatial.rotations(dims=['x'], values=[[1, 2, 3, 4]])
translation = sc.spatial.translations(dims=['x'], values=[[5, 6, 7]], unit=sc.units.m)
affine = sc.spatial.affine_transforms(
    dims=['x'],
    values=[[[0, 1, 2, 4], [5, 6, 7, 8], [9, 10, 11, 12], [13, 14, 15, 16]]],
    unit=sc.units.m,
)

datetime64ms_1d = sc.Variable(
    dims=['x'], dtype=sc.DType.datetime64, unit='ms', values=np.arange(10)
)

datetime64us_1d = sc.Variable(
    dims=['x'], dtype=sc.DType.datetime64, unit='us', values=np.arange(10)
)

array_1d = sc.DataArray(
    data=x,
    coords={'x': x, 'λ': 2.0 * x},
    masks={
        'mask.1': sc.less(x, 1.5 * sc.units.m),
        'mask.2': sc.less(x, 2.5 * sc.units.m),
    },
    attrs={'attr1': x, 'attr2': 1.2 * sc.units.K},
)
array_2d = sc.DataArray(
    data=xy,
    coords={'x': x, 'y': y, 'x2': 2.0 * x},
    masks={
        'mask1': sc.less(x, 1.5 * sc.units.m),
        'mask2': sc.less(xy, 0.5 * sc.units.kg),
    },
    attrs={'attr1': xy, 'attr2': 1.2 * sc.units.K},
)


def test_variable_1d():
    check_roundtrip(x)


def test_variable_2d():
    check_roundtrip(xy)


def test_variable_vector():
    check_roundtrip(vector)
    check_roundtrip(vector['x', 0])


def test_variable_matrix():
    check_roundtrip(matrix)
    check_roundtrip(matrix['x', 0])


def test_variable_rotation():
    check_roundtrip(rotation)
    check_roundtrip(rotation['x', 0])


def test_variable_translation():
    check_roundtrip(translation)
    check_roundtrip(translation['x', 0])


def test_variable_affine():
    check_roundtrip(affine)
    check_roundtrip(affine['x', 0])


def test_variable_datetime64():
    check_roundtrip(datetime64ms_1d)
    check_roundtrip(datetime64ms_1d['x', 0])
    check_roundtrip(datetime64us_1d)
    check_roundtrip(datetime64us_1d['x', 0])


def test_variable_binned_variable():
    begin = sc.Variable(dims=['y'], values=[0, 3], dtype=sc.DType.int64, unit=None)
    end = sc.Variable(dims=['y'], values=[3, 4], dtype=sc.DType.int64, unit=None)
    binned = sc.bins(begin=begin, end=end, dim='x', data=x)
    check_roundtrip(binned)


def test_variable_binned_variable_slice():
    begin = sc.Variable(dims=['y'], values=[0, 3], dtype=sc.DType.int64, unit=None)
    end = sc.Variable(dims=['y'], values=[3, 4], dtype=sc.DType.int64, unit=None)
    binned = sc.bins(begin=begin, end=end, dim='x', data=x)
    # Note the current arbitrary limit is to avoid writing the buffer if it is
    # more than 50% too large. These cutoffs or the entire mechanism may
    # change in the future, so this test should be adapted. This test does not
    # document a strict requirement.
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


def test_variable_legacy_str_unit():
    var = x.copy()
    var.unit = 'm/s*kg^2'
    with tempfile.TemporaryDirectory() as path:
        name = f'{path}/test.hdf5'
        var.save_hdf5(filename=name)
        with h5py.File(name, 'a') as f:
            f['values'].attrs['unit'] = str(var.unit)
        loaded = sc.io.load_hdf5(filename=name)
    assert sc.identical(loaded, var)


def test_data_array_1d_no_coords():
    a = sc.DataArray(data=x)
    check_roundtrip(a)


def test_data_array_all_units_supported():
    for unit in [
        sc.units.one,
        sc.units.us,
        sc.units.angstrom,
        sc.units.counts,
        sc.units.counts / sc.units.us,
        sc.units.meV,
    ]:
        a = sc.DataArray(data=1.0 * unit)
        check_roundtrip(a)


def test_data_array_1d():
    check_roundtrip(array_1d)


def test_data_array_1d_name_is_stored_correctly():
    da = array_1d.copy()
    da.name = 'some name'
    result = roundtrip(da)
    assert result.name == 'some name'


def test_data_array_2d():
    check_roundtrip(array_2d)


def test_data_array_dtype_scipp_container():
    a = sc.DataArray(data=x)
    a.coords['variable'] = sc.scalar(x)
    a.coords['scalar'] = sc.scalar(a)
    a.coords['1d'] = sc.empty(dims=x.dims, shape=x.shape, dtype=sc.DType.DataArray)
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


def test_dataset_item_can_be_read_as_data_array():
    ds = sc.Dataset(data={'a': array_1d, 'b': array_2d})
    with tempfile.TemporaryDirectory() as path:
        name = f'{path}/test.hdf5'
        ds.save_hdf5(filename=name)
        loaded = sc.Dataset()
        with h5py.File(name, 'r') as f:
            for entry in f['entries'].values():
                da = sc.io.hdf5.HDF5IO.read(entry)
                loaded[da.name] = da
        assert sc.identical(loaded, ds)


def test_dataset_with_many_coords():
    rows = 10000
    a = sc.ones(dims=['row'], shape=[rows], dtype='float64', unit=None)
    b = sc.ones(dims=['row'], shape=[rows], dtype='float64', unit=None)
    coords = {
        k: sc.ones(dims=['row'], shape=[rows], dtype='float64', unit=None)
        for k in 'abcdefghijklmnopqrstuvwxyz'
    }
    ds1 = sc.Dataset(coords=coords)
    ds1['a'] = a
    ds2 = sc.Dataset(coords=coords)
    ds2['a'] = a
    ds2['b'] = b
    with tempfile.TemporaryDirectory() as path:
        name1 = f'{path}/test1.hdf5'
        name2 = f'{path}/test2.hdf5'
        ds1.save_hdf5(filename=name1)
        ds2.save_hdf5(filename=name2)
        size1 = os.path.getsize(name1)
        size2 = os.path.getsize(name2)
        # If coords were stored per-entry then we would need a factor close to 2
        # here. 1.1 implies that there is no such full duplication.
        assert size1 * 1.1 > size2
        # Empirically determined extra size. This is likely brittle. We want to
        # somehow ensure that no column is stored more than once. This does not
        # depend on the number of rows.
        extra = 75000
        assert size1 < (len(ds1.coords) + 1) * rows * 8 + extra


def test_data_group():
    dg = sc.DataGroup(
        {
            'variable': vector,
            'data_array': array_2d,
            'dataset': sc.Dataset({'a': array_1d, 'b': array_2d}),
            'data group': sc.DataGroup({'v.same': vector, 'm/copy': matrix.copy()}),
        }
    )
    check_roundtrip(dg)


def test_data_group_empty():
    dg = sc.DataGroup()
    check_roundtrip(dg)


def test_data_group_unsupported_PyObject():
    dg = sc.DataGroup({'a': x, 'b': sc.scalar([1, 2], dtype=object)})
    res = roundtrip(dg)
    assert sc.identical(res['a'], dg['a'])
    assert 'b' not in res


def test_data_group_unsupported_type():
    dg = sc.DataGroup({'a': 2, 'b': sc.scalar(3)})
    res = roundtrip(dg)
    assert 'a' not in res
    assert sc.identical(res['b'], dg['b'])


def test_variable_with_zero_length_dimension():
    v = sc.Variable(dims=["x"], values=[])
    check_roundtrip(v)


def test_variable_with_zero_length_dimension_with_variances():
    v = sc.Variable(dims=["x"], values=[], variances=[])
    check_roundtrip(v)


def test_None_unit_is_preserved_even_if_dtype_does_not_default_to_None_unit():
    v = sc.scalar(1.2, unit=None)
    result = check_roundtrip(v)
    assert result.unit is None


def assert_is_valid_hdf5_name(name: str):
    assert '.' not in name
    assert '/' not in name
    name.encode('ascii', 'strict')  # raise exception if not ASCII


def test_periods_are_escaped_in_names():
    assert_is_valid_hdf5_name(collection_element_name('a.b', 0))
    assert_is_valid_hdf5_name(collection_element_name('a..b', 1))
    assert_is_valid_hdf5_name(collection_element_name('a.', 2))
    assert_is_valid_hdf5_name(collection_element_name('.a', 3))


def test_slashes_are_escaped_in_names():
    assert_is_valid_hdf5_name(collection_element_name('a/b', 0))
    assert_is_valid_hdf5_name(collection_element_name('a//b', 1))
    assert_is_valid_hdf5_name(collection_element_name('a/', 2))
    assert_is_valid_hdf5_name(collection_element_name('/a', 3))


def test_unicode_is_escaped_in_names():
    assert_is_valid_hdf5_name(collection_element_name('µm', 0))
    assert_is_valid_hdf5_name(collection_element_name('λ', 1))
    assert_is_valid_hdf5_name(collection_element_name('Å/travel_time', 2))
    assert_is_valid_hdf5_name(collection_element_name('λ in Å', 3))
