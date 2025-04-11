# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

import pytest

import scipp as sc


@pytest.mark.parametrize("mapping", ["coords", "masks"])
def test_data_array_mapping_repr_does_not_raise(mapping: str) -> None:
    da = sc.data.table_xyz(10)
    da.masks['m'] = da.coords['x'] > sc.scalar(0.5, unit='m')
    repr(getattr(da, mapping))
    repr(getattr(da, mapping).keys())
    repr(getattr(da, mapping).values())
    repr(getattr(da, mapping).items())


@pytest.mark.parametrize("mapping", ["coords", "masks"])
def test_data_array_empty_mapping_repr_does_not_raise(mapping: str) -> None:
    da = sc.DataArray(data=sc.arange('x', 10))
    repr(getattr(da, mapping))
    repr(getattr(da, mapping).keys())
    repr(getattr(da, mapping).values())
    repr(getattr(da, mapping).items())


def test_dataset_coords_repr_does_not_raise() -> None:
    ds = sc.Dataset({'a': sc.data.table_xyz(10), 'b': sc.data.table_xyz(10).data})  # type: ignore[arg-type]
    repr(ds.coords)
    repr(ds.coords.keys())
    repr(ds.coords.values())
    repr(ds.coords.items())


def test_dataset_iterators_repr_does_not_raise() -> None:
    ds = sc.Dataset({'a': sc.data.table_xyz(10), 'b': sc.data.table_xyz(10).data})  # type: ignore[arg-type]
    repr(ds.keys())
    repr(ds.values())
    repr(ds.items())


def test_data_group_repr_does_not_raise() -> None:
    dg = sc.DataGroup(
        {
            'a': sc.data.table_xyz(10),
            'b': sc.DataGroup(
                {
                    'da': sc.data.binned_x(10, 3),
                    'int': 623,
                }
            ),
            's': 'a string',
            'var': sc.linspace('tt', 5, 8, 13, unit='Å'),
        }
    )
    repr(dg)


def test_data_group_repr_includes_items() -> None:
    dg = sc.DataGroup(
        {
            'item 1': 'a string',
            'another': sc.arange('d', 10, unit='Å'),
        }
    )
    res = repr(dg)
    idx1 = res.index('item 1')
    idx2 = res.index('another')
    assert idx1 < idx2
    idx1 = res.index('a string')
    idx2 = res.index('Variable')
    assert idx1 < idx2


@pytest.mark.parametrize("mapping", ["coords", "masks"])
def test_data_array_mapping_str_does_not_raise(mapping: str) -> None:
    da = sc.data.table_xyz(10)
    da.masks['m'] = da.coords['x'] > sc.scalar(0.5, unit='m')
    str(getattr(da, mapping))
    str(getattr(da, mapping).keys())
    str(getattr(da, mapping).values())
    str(getattr(da, mapping).items())


@pytest.mark.parametrize("mapping", ["coords", "masks"])
def test_data_array_empty_mapping_str_does_not_raise(mapping: str) -> None:
    da = sc.DataArray(data=sc.arange('x', 10))
    str(getattr(da, mapping))
    str(getattr(da, mapping).keys())
    str(getattr(da, mapping).values())
    str(getattr(da, mapping).items())


def test_dataset_coords_str_does_not_raise() -> None:
    ds = sc.Dataset({'a': sc.data.table_xyz(10), 'b': sc.data.table_xyz(10).data})  # type: ignore[arg-type]
    str(ds.coords)
    str(ds.coords.keys())
    str(ds.coords.values())
    str(ds.coords.items())


def test_dataset_iterators_str_does_not_raise() -> None:
    ds = sc.Dataset({'a': sc.data.table_xyz(10), 'b': sc.data.table_xyz(10).data})  # type: ignore[arg-type]
    str(ds.keys())
    str(ds.values())
    str(ds.items())


def test_data_group_str_does_not_raise() -> None:
    dg = sc.DataGroup(
        {
            'a': sc.data.table_xyz(10),
            'b': sc.DataGroup(
                {
                    'da': sc.data.binned_x(10, 3),
                    'int': 623,
                }
            ),
            's': 'a string',
            'var': sc.linspace('tt', 5, 8, 13, unit='Å'),
        }
    )
    str(dg)


def test_data_group_str_includes_items() -> None:
    dg = sc.DataGroup(
        {
            'item 1': 'a string',
            'another': sc.arange('d', 10, unit='Å'),
        }
    )
    res = str(dg)
    idx1 = res.index('item 1')
    idx2 = res.index('another')
    assert idx1 < idx2
