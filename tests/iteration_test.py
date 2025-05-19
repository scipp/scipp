# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
from collections.abc import Generator

import pytest

import scipp as sc


@pytest.fixture(params=['Dataset', 'DatasetView'])
def dataset_abc(request: pytest.FixtureRequest) -> Generator[sc.Dataset, None, None]:
    d = sc.Dataset(
        {
            'a': sc.Variable(dims=['x'], values=[1, 2]),
            'b': sc.Variable(dims=['x'], values=[3, 4]),
            'c': sc.Variable(dims=['x'], values=[5, 6]),
        }
    )
    # Using yield so `d` does not go out of scope when returning slice
    if request.param == 'Dataset':
        yield d
    else:
        yield d['x', 0]


def test_dataset_iter(dataset_abc: sc.Dataset) -> None:
    found = set()
    for key in dataset_abc:
        found.add(key)
    assert found == {'a', 'b', 'c'}


def test_dataset_keys(dataset_abc: sc.Dataset) -> None:
    assert set(dataset_abc.keys()) == {'a', 'b', 'c'}


def test_dataset_values(dataset_abc: sc.Dataset) -> None:
    found = set()
    for value in dataset_abc.values():
        found.add(value.name)
    assert found == {'a', 'b', 'c'}


def test_dataset_items(dataset_abc: sc.Dataset) -> None:
    assert len(dataset_abc.items()) == 3
    found = set()
    for name, value in dataset_abc.items():
        assert name == value.name
        found.add(name)
    assert found == {'a', 'b', 'c'}


def make_coords_xyz() -> sc.Dataset:
    return sc.Dataset(
        coords={
            'x': 1.0 * sc.units.m,
            'y': 2.0 * sc.units.m,
            'z': 3.0 * sc.units.m,
        }
    )


def test_dataset_coords_iter() -> None:
    d = make_coords_xyz()
    found = set()
    for key in d.coords:
        found.add(str(key))
    assert found == {'x', 'y', 'z'}


def test_dataset_coords_keys() -> None:
    d = make_coords_xyz()
    assert set(d.coords.keys()) == {'x', 'y', 'z'}


def test_dataset_coords_values() -> None:
    d = make_coords_xyz()
    found = set()
    for value in d.coords.values():
        found.add(value.value)
    assert found == {1.0, 2.0, 3.0}


def test_dataset_coords_items() -> None:
    d = make_coords_xyz()
    assert len(d.coords.items()) == 3
    found = set()
    for dim in d.coords:
        found.add(str(dim))
    assert found == {'x', 'y', 'z'}
