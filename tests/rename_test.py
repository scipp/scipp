# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import pytest

import scipp as sc


def make_dataarray(dim1, dim2) -> sc.DataArray:
    from numpy.random import default_rng

    rng = default_rng(seed=1234)
    da = sc.DataArray(sc.array(dims=[dim1, dim2], values=rng.random((100, 100))))
    da.coords[dim1] = sc.linspace(dim1, 0.0, 1.0, num=100, unit='mm')
    da.coords[dim2] = sc.linspace(dim2, 0.0, 5.0, num=100, unit='mm')
    return da


def test_rename_dims():
    da = make_dataarray('x', 'y')
    original = da.copy()
    renamed = da.rename_dims({'y': 'z'})
    assert sc.identical(da, original)
    renamed.coords['z'] = renamed.coords['y']
    del renamed.coords['y']
    assert sc.identical(renamed, make_dataarray('x', 'z'))
    renamed = renamed.rename_dims({'x': 'y', 'z': 'x'})
    renamed.coords['y'] = renamed.coords['x']
    renamed.coords['x'] = renamed.coords['z']
    del renamed.coords['z']
    assert sc.identical(renamed, make_dataarray('y', 'x'))


def test_rename():
    da = make_dataarray('x', 'y')
    original = da.copy()
    renamed = da.rename({'y': 'z'})
    assert sc.identical(da, original)
    assert sc.identical(renamed, make_dataarray('x', 'z'))
    renamed = renamed.rename({'x': 'y', 'z': 'x'})
    assert sc.identical(renamed, make_dataarray('y', 'x'))


def test_rename_kwargs():
    da = make_dataarray('x', 'y')
    renamed = da.rename(y='z')
    assert sc.identical(renamed, make_dataarray('x', 'z'))
    renamed = renamed.rename(x='y', z='x')
    assert sc.identical(renamed, make_dataarray('y', 'x'))


def test_rename_with_attr():
    da = make_dataarray('x', 'y')
    da.attrs['y'] = da.coords.pop('y')
    renamed = da.rename({'y': 'z'})
    expected = make_dataarray('x', 'z')
    expected.attrs['z'] = expected.coords.pop('z')
    assert sc.identical(renamed, expected)


def test_rename_fails_when_coord_already_exists():
    da = make_dataarray('x', 'y')
    da.coords['z'] = da.coords['x'].copy()
    with pytest.raises(sc.CoordError):
        da.rename({'x': 'z'})


def test_rename_fails_when_attr_already_exists():
    da = make_dataarray('x', 'y')
    da.attrs['y'] = da.coords.pop('y')
    da.attrs['z'] = da.attrs['y'].copy()
    with pytest.raises(sc.CoordError):
        da.rename({'y': 'z'})


def test_rename_fails_when_attr_with_same_name_already_exists():
    da = make_dataarray('x', 'y')
    da.attrs['meta'] = sc.scalar(5)
    with pytest.raises(sc.CoordError):
        da.rename({'x': 'meta'})


def test_rename_fails_when_coord_with_same_name_already_exists():
    da = make_dataarray('x', 'y')
    da.attrs['aux'] = sc.scalar(5)
    da.attrs['y'] = da.coords.pop('y')
    with pytest.raises(sc.CoordError):
        da.rename({'y': 'aux'})


def test_rename_renames_bins_coords_and_attrs():
    table = sc.data.table_xyz(10)
    table.attrs['y'] = table.coords.pop('y')
    da = table.bin(x=2, y=2)
    renamed = da.rename(x='x2', y='y2')
    assert 'x' not in renamed.bins.coords
    assert 'y' not in renamed.bins.attrs
    assert 'x2' in renamed.bins.coords
    assert 'y2' in renamed.bins.attrs


def test_rename_of_bins_coords_and_attrs_does_not_affect_input():
    table = sc.data.table_xyz(10)
    table.attrs['y'] = table.coords.pop('y')
    da = table.bin(x=2, y=2)
    _ = da.rename(x='x2', y='y2')
    assert 'x' in da.bins.coords
    assert 'y' in da.bins.attrs


def test_rename_raises_DimensionError_if_only_bins_coords_or_attrs():
    table = sc.data.table_xyz(10)
    table.attrs['z'] = table.coords.pop('z')
    da = table.bin(x=2)
    with pytest.raises(sc.DimensionError):
        da.rename(y='y2', z='z2')


def test_rename_dataset_only_data():
    ds = sc.Dataset({'a': make_dataarray('x', 'y').data})
    renamed = ds.rename({'y': 'z'})
    assert set(renamed.keys()) == {'a'}
    expected = make_dataarray('x', 'z')
    expected.coords.clear()
    assert sc.identical(renamed['a'], expected)


def test_rename_dataset_only_coords():
    ds = sc.Dataset(coords=make_dataarray('x', 'y').coords)
    renamed = ds.rename({'y': 'z'})
    assert len(renamed) == 0
    assert renamed.coords == make_dataarray('x', 'z').coords


def test_rename_dataset_data_and_coords():
    ds = sc.Dataset({'a': make_dataarray('x', 'y')})
    renamed = ds.rename({'y': 'z'})
    assert set(renamed.keys()) == {'a'}
    assert sc.identical(renamed['a'], make_dataarray('x', 'z'))
