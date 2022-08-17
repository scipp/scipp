# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

import scipp as sc
import pytest

# TODO: For now, we are just checking that creating the repr does not throw.


@pytest.mark.parametrize("mapping", ["coords", "attrs", "meta", "masks"])
def test_dat_array_mapping_repr(mapping):
    da = sc.data.table_xyz(10)
    da.attrs['a'] = da.coords['x']
    da.masks['m'] = da.coords['x'] > sc.scalar(0.5, 'm')
    repr(getattr(da, mapping))
    repr(getattr(da, mapping).keys())
    repr(getattr(da, mapping).values())
    repr(getattr(da, mapping).items())

@pytest.mark.parametrize("mapping", ["coords", "attrs", "meta", "masks"])
def test_data_array_empty mapping_repr(mapping):
    da = sc.DataArray(data=sc.arange('x', 10))
    repr(getattr(da, mapping))
    repr(getattr(da, mapping).keys())
    repr(getattr(da, mapping).values())
    repr(getattr(da, mapping).items())

def test_dataset mapping_repr(mapping):
    ds = sc.Dataset({'a': sc.data.table_xyz(10),
                 'b': sc.data.table_xyz(10).data})
    repr(getattr(ds.coords))
    repr(getattr(ds.coords.keys()))
    repr(getattr(ds.coords.values()))
    repr(getattr(ds.coords.items()))

def test_dataset empty_mapping_repr(mapping):
    ds = sc.Dataset()
    repr(getattr(ds.coords)
    repr(getattr(ds.coords.keys())
    repr(getattr(ds.coords.values())
    repr(getattr(ds.coords.items())
