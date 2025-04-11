# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import os
import tempfile
from pathlib import Path

import numpy as np
import pytest

import scipp as sc
import scipp.testing
from scipp.io.hdf5 import _collection_element_name

h5py = pytest.importorskip('h5py')


def roundtrip(obj):
    with tempfile.TemporaryDirectory() as path:
        name = Path(path, 'test.hdf5')
        obj.save_hdf5(filename=name)
        return sc.io.load_hdf5(filename=name)


def check_roundtrip(obj):
    result = roundtrip(obj)
    assert sc.identical(result, obj)
    return result  # for optional addition tests


x = sc.array(dims=['x'], values=np.arange(4.0), unit=sc.units.m)
y = sc.array(dims=['y'], values=np.arange(6.0), unit=sc.units.angstrom)
xy = sc.array(
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

strings = sc.array(
    dims=['s'], values=['a', '', 'long string which, longer than small string opt']
)

datetime64ms_1d = sc.array(
    dims=['x'], dtype=sc.DType.datetime64, unit='ms', values=np.arange(10)
)

datetime64us_1d = sc.array(
    dims=['x'], dtype=sc.DType.datetime64, unit='us', values=np.arange(10)
)

array_1d = sc.DataArray(
    data=x,
    coords={'x': x, 'λ': 2.0 * x},
    masks={
        'mask.1': sc.less(x, 1.5 * sc.units.m),
        'mask.2': sc.less(x, 2.5 * sc.units.m),
    },
)
array_2d = sc.DataArray(
    data=xy,
    coords={'x': x, 'y': y, 'x2': 2.0 * x},
    masks={
        'mask1': sc.less(x, 1.5 * sc.units.m),
        'mask2': sc.less(xy, 0.5 * sc.units.kg),
    },
)


def test_variable_1d() -> None:
    check_roundtrip(x)


def test_variable_2d() -> None:
    check_roundtrip(xy)


def test_variable_vector() -> None:
    check_roundtrip(vector)
    check_roundtrip(vector['x', 0])


def test_variable_matrix() -> None:
    check_roundtrip(matrix)
    check_roundtrip(matrix['x', 0])


def test_variable_rotation() -> None:
    check_roundtrip(rotation)
    check_roundtrip(rotation['x', 0])


def test_variable_translation() -> None:
    check_roundtrip(translation)
    check_roundtrip(translation['x', 0])


def test_variable_affine() -> None:
    check_roundtrip(affine)
    check_roundtrip(affine['x', 0])


def test_variable_strings() -> None:
    check_roundtrip(strings)


def test_variable_datetime64() -> None:
    check_roundtrip(datetime64ms_1d)
    check_roundtrip(datetime64ms_1d['x', 0])
    check_roundtrip(datetime64us_1d)
    check_roundtrip(datetime64us_1d['x', 0])


def test_variable_binned_variable() -> None:
    begin = sc.Variable(dims=['y'], values=[0, 3], dtype=sc.DType.int64, unit=None)
    end = sc.Variable(dims=['y'], values=[3, 4], dtype=sc.DType.int64, unit=None)
    binned = sc.bins(begin=begin, end=end, dim='x', data=x)
    check_roundtrip(binned)


def test_variable_binned_variable_slice() -> None:
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


def test_variable_binned_data_array() -> None:
    binned = sc.bins(dim='x', data=array_1d)
    check_roundtrip(binned)


def test_variable_binned_dataset() -> None:
    d = sc.Dataset(data={'a': array_1d, 'b': array_1d})
    binned = sc.bins(dim='x', data=d)
    check_roundtrip(binned)


def test_variable_legacy_str_unit() -> None:
    var = x.copy()
    var.unit = 'm/s*kg^2'
    with tempfile.TemporaryDirectory() as path:
        name = Path(path, 'test.hdf5')
        var.save_hdf5(filename=name)
        with h5py.File(name, 'a') as f:
            f['values'].attrs['unit'] = str(var.unit)
        loaded = sc.io.load_hdf5(filename=name)
    assert sc.identical(loaded, var)


def test_data_array_1d_no_coords() -> None:
    a = sc.DataArray(data=x)
    check_roundtrip(a)


def test_data_array_all_units_supported() -> None:
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


def test_data_array_1d() -> None:
    check_roundtrip(array_1d)


def test_data_array_1d_name_is_stored_correctly() -> None:
    da = array_1d.copy()
    da.name = 'some name'
    result = roundtrip(da)
    assert result.name == 'some name'


def test_data_array_2d() -> None:
    check_roundtrip(array_2d)


def test_data_array_dtype_scipp_container() -> None:
    a = sc.DataArray(data=x)
    a.coords['variable'] = sc.scalar(x)
    a.coords['scalar'] = sc.scalar(a)
    a.coords['1d'] = sc.empty(dims=x.dims, shape=x.shape, dtype=sc.DType.DataArray)
    for i in range(4):
        a.coords['1d'].values[i] = sc.DataArray(float(i) * sc.units.m)
    a.coords['dataset'] = sc.scalar(sc.Dataset(data={'a': array_1d}))
    check_roundtrip(a)


def test_data_array_dtype_string() -> None:
    a = sc.DataArray(data=sc.Variable(dims=['x'], values=['abc', 'def']))
    check_roundtrip(a)
    check_roundtrip(a['x', 0])


def test_data_array_unsupported_PyObject_coord() -> None:
    obj = sc.scalar({})
    a = sc.DataArray(data=x, coords={'obj': obj})
    b = roundtrip(a)
    assert not sc.identical(a, b)
    del a.coords['obj']
    assert sc.identical(a, b)


def test_data_array_coord_alignment() -> None:
    a = array_2d.copy()
    a.coords.set_aligned('y', False)
    b = roundtrip(a)
    assert sc.identical(a, b)


# Attributes where removed in https://github.com/scipp/scipp/pull/3626
# But the loader can still load attrs from files and assigns them as coords
# as per https://github.com/scipp/scipp/pull/3626#discussion_r1906966229
def test_data_array_loads_legacy_attributes() -> None:
    a = sc.ones(sizes=array_1d.sizes, dtype='float64')
    µ = sc.array(dims=array_1d.dims, values=[f'a{i}' for i in range(len(array_1d))])
    with_attrs_as_coords = array_1d.assign_coords({'a': a, 'µ': µ})

    with tempfile.TemporaryDirectory() as path:
        name = Path(path, 'test.hdf5')
        with_attrs_as_coords.save_hdf5(filename=name)
        with h5py.File(name, 'r+') as f:
            # Move new coords to attrs
            f.move(source='coords/elem_002_a', dest='attrs/elem_000_a')
            f.move(source='coords/elem_003_&#181;', dest='attrs/elem_001_&#181;')

        loaded = sc.io.load_hdf5(filename=name)
    with_attrs_as_coords.coords.set_aligned('a', False)
    with_attrs_as_coords.coords.set_aligned('µ', False)
    sc.testing.assert_identical(with_attrs_as_coords, loaded)


def test_data_array_raises_with_clashing_attr_and_coord() -> None:
    initial = array_1d

    with tempfile.TemporaryDirectory() as path:
        name = Path(path, 'test.hdf5')
        initial.save_hdf5(filename=name)
        with h5py.File(name, 'r+') as f:
            # Copy coord into attr to create clash
            f.copy(source='coords/elem_000_x', dest='attrs/elem_000_x')

        with pytest.raises(ValueError, match="attributes {'x'}"):
            sc.io.load_hdf5(filename=name)


def test_variable_binned_data_array_coord_alignment() -> None:
    binned = sc.bins(dim='x', data=array_1d)
    binned.bins.coords.set_aligned('x', False)
    check_roundtrip(binned)


def test_dataset() -> None:
    d = sc.Dataset(data={'a': array_2d, 'b': array_2d})
    check_roundtrip(d)


def test_dataset_item_can_be_read_as_data_array() -> None:
    ds = sc.Dataset(data={'a': array_1d})
    with tempfile.TemporaryDirectory() as path:
        name = Path(path, 'test.hdf5')
        ds.save_hdf5(filename=name)
        loaded = {}
        with h5py.File(name, 'r') as f:
            for entry in f['entries'].values():
                da = sc.io.load_hdf5(entry)
                loaded[da.name] = da
        loaded = sc.Dataset(loaded)
        assert sc.identical(loaded, ds)


def test_dataset_with_many_coords() -> None:
    rows = 10000
    a = sc.ones(dims=['row'], shape=[rows], dtype='float64', unit=None)
    b = sc.ones(dims=['row'], shape=[rows], dtype='float64', unit=None)
    coords = {
        k: sc.ones(dims=['row'], shape=[rows], dtype='float64', unit=None)
        for k in 'abcdefghijklmnopqrstuvwxyz'
    }
    ds1 = sc.Dataset({'a': a}, coords=coords)
    ds2 = sc.Dataset({'a': a, 'b': b}, coords=coords)
    with tempfile.TemporaryDirectory() as path:
        name1 = Path(path, 'test1.hdf5')
        name2 = Path(path, 'test2.hdf5')
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


def test_data_group() -> None:
    dg = sc.DataGroup(
        {
            'variable': vector,
            'data_array': array_2d,
            'dataset': sc.Dataset({'a': array_1d}),
            'data group': sc.DataGroup({'v.same': vector, 'm/copy': matrix.copy()}),
            'integer': 513,
            'numpy int64': np.int64(9),
            'numpy int32': np.int32(-23),
            'numpy uint64': np.uint64(9312),
            'numpy uint32': np.uint32(66134),
            'float': 7.81,
            'numpy float64': np.float64(5235.1),
            'numpy float32': np.float32(0.031),
            'boolean': True,
            'numpy bool_': np.bool_(False),
            'string': 'a string for testing',
            'bytes': b'a bytes array',
            'numpy array': x.values,
        }
    )
    check_roundtrip(dg)


def test_data_group_empty() -> None:
    dg = sc.DataGroup()
    check_roundtrip(dg)


def test_data_group_unsupported_PyObject() -> None:
    dg = sc.DataGroup({'a': x, 'b': sc.scalar([1, 2], dtype=object)})
    res = roundtrip(dg)
    assert sc.identical(res['a'], dg['a'])
    assert 'b' not in res


def test_variable_with_zero_length_dimension() -> None:
    v = sc.Variable(dims=["x"], values=[])
    check_roundtrip(v)


def test_variable_with_zero_length_dimension_with_variances() -> None:
    v = sc.Variable(dims=["x"], values=[], variances=[])
    check_roundtrip(v)


def test_None_unit_is_preserved_even_if_dtype_does_not_default_to_None_unit() -> None:
    v = sc.scalar(1.2, unit=None)
    result = check_roundtrip(v)
    assert result.unit is None


def assert_is_valid_hdf5_name(name: str):
    assert '.' not in name
    assert '/' not in name
    name.encode('ascii', 'strict')  # raise exception if not ASCII


def test_periods_are_escaped_in_names() -> None:
    assert_is_valid_hdf5_name(_collection_element_name('a.b', 0))
    assert_is_valid_hdf5_name(_collection_element_name('a..b', 1))
    assert_is_valid_hdf5_name(_collection_element_name('a.', 2))
    assert_is_valid_hdf5_name(_collection_element_name('.a', 3))


def test_slashes_are_escaped_in_names() -> None:
    assert_is_valid_hdf5_name(_collection_element_name('a/b', 0))
    assert_is_valid_hdf5_name(_collection_element_name('a//b', 1))
    assert_is_valid_hdf5_name(_collection_element_name('a/', 2))
    assert_is_valid_hdf5_name(_collection_element_name('/a', 3))


def test_unicode_is_escaped_in_names() -> None:
    assert_is_valid_hdf5_name(_collection_element_name('µm', 0))
    assert_is_valid_hdf5_name(_collection_element_name('λ', 1))
    assert_is_valid_hdf5_name(_collection_element_name('Å/travel_time', 2))
    assert_is_valid_hdf5_name(_collection_element_name('λ in Å', 3))
