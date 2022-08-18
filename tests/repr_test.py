# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

import scipp as sc
import pytest


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


def test_dataset_mapping_repr_does_not_raise():
    ds = sc.Dataset({'a': sc.data.table_xyz(10), 'b': sc.data.table_xyz(10).data})
    repr(ds.coords)
    repr(ds.coords.keys())
    repr(ds.coords.values())
    repr(ds.coords.items())


def test_dataset_empty_mapping_repr_does_not_raise():
    ds = sc.Dataset()
    repr(ds.coords)
    repr(ds.coords.keys())
    repr(ds.coords.values())
    repr(ds.coords.items())
