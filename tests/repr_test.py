# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

import pytest

import scipp as sc


@pytest.mark.parametrize("mapping", ["coords", "attrs", "meta", "masks"])
def test_data_array_mapping_repr_does_not_raise(mapping):
    da = sc.data.table_xyz(10)
    da.attrs['a'] = da.coords['x']
    da.masks['m'] = da.coords['x'] > sc.scalar(0.5, unit='m')
    repr(getattr(da, mapping))
    repr(getattr(da, mapping).keys())
    repr(getattr(da, mapping).values())
    repr(getattr(da, mapping).items())


@pytest.mark.parametrize("mapping", ["coords", "attrs", "meta", "masks"])
def test_data_array_empty_mapping_repr_does_not_raise(mapping):
    da = sc.DataArray(data=sc.arange('x', 10))
    repr(getattr(da, mapping))
    repr(getattr(da, mapping).keys())
    repr(getattr(da, mapping).values())
    repr(getattr(da, mapping).items())


def test_dataset_coords_repr_does_not_raise():
    ds = sc.Dataset({'a': sc.data.table_xyz(10), 'b': sc.data.table_xyz(10).data})
    repr(ds.coords)
    repr(ds.coords.keys())
    repr(ds.coords.values())
    repr(ds.coords.items())


def test_dataset_iterators_repr_does_not_raise():
    ds = sc.Dataset({'a': sc.data.table_xyz(10), 'b': sc.data.table_xyz(10).data})
    repr(ds.keys())
    repr(ds.values())
    repr(ds.items())


@pytest.mark.parametrize("mapping", ["coords", "attrs", "meta", "masks"])
def test_data_array_mapping_str_does_not_raise(mapping):
    da = sc.data.table_xyz(10)
    da.attrs['a'] = da.coords['x']
    da.masks['m'] = da.coords['x'] > sc.scalar(0.5, unit='m')
    str(getattr(da, mapping))
    str(getattr(da, mapping).keys())
    str(getattr(da, mapping).values())
    str(getattr(da, mapping).items())


@pytest.mark.parametrize("mapping", ["coords", "attrs", "meta", "masks"])
def test_data_array_empty_mapping_str_does_not_raise(mapping):
    da = sc.DataArray(data=sc.arange('x', 10))
    str(getattr(da, mapping))
    str(getattr(da, mapping).keys())
    str(getattr(da, mapping).values())
    str(getattr(da, mapping).items())


def test_dataset_coords_str_does_not_raise():
    ds = sc.Dataset({'a': sc.data.table_xyz(10), 'b': sc.data.table_xyz(10).data})
    str(ds.coords)
    str(ds.coords.keys())
    str(ds.coords.values())
    str(ds.coords.items())


def test_dataset_iterators_str_does_not_raise():
    ds = sc.Dataset({'a': sc.data.table_xyz(10), 'b': sc.data.table_xyz(10).data})
    str(ds.keys())
    str(ds.values())
    str(ds.items())
