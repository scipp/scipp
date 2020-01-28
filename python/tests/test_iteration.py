# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import pytest

import scipp as sc
from scipp import Dim


@pytest.fixture(params=['Dataset', 'DatasetProxy'])
def dataset_abc(request):
    d = sc.Dataset()
    d['a'] = sc.Variable([Dim.X], values=[1, 2])
    d['b'] = sc.Variable([Dim.X], values=[3, 4])
    d['c'] = sc.Variable([Dim.X], values=[5, 6])
    # Using yield so `d` does not go out of scope when returning slice
    if request.param == 'Dataset':
        yield d
    else:
        yield d[Dim.X, 0]


def test_dataset_iter(dataset_abc):
    found = set()
    for key in dataset_abc:
        found.add(key)
    assert found == set(['a', 'b', 'c'])


def test_dataset_keys(dataset_abc):
    assert set(dataset_abc.keys()) == set(['a', 'b', 'c'])


def test_dataset_values(dataset_abc):
    found = set()
    for value in dataset_abc.values():
        found.add(value.name)
    assert found == set(['a', 'b', 'c'])


def test_dataset_items(dataset_abc):
    assert len(dataset_abc.items()) == 3
    found = set()
    for name, value in dataset_abc.items():
        assert name == value.name
        found.add(name)
    assert found == set(['a', 'b', 'c'])


def make_coords_xyz():
    d = sc.Dataset()
    d.coords[Dim.X] = 1.0 * sc.units.m
    d.coords[Dim.Y] = 2.0 * sc.units.m
    d.coords[Dim.Z] = 3.0 * sc.units.m
    return d


def test_dataset_coords_iter():
    d = make_coords_xyz()
    found = set()
    for key in d.coords:
        found.add(key)
    assert found == set([Dim.X, Dim.Y, Dim.Z])


def test_dataset_coords_keys():
    d = make_coords_xyz()
    assert set(d.coords.keys()) == set([Dim.X, Dim.Y, Dim.Z])


def test_dataset_coords_values():
    d = make_coords_xyz()
    found = set()
    for value in d.coords.values():
        found.add(value.value)
    assert found == set([1.0, 2.0, 3.0])


def test_dataset_coords_items():
    d = make_coords_xyz()
    assert len(d.coords.items()) == 3
    found = set()
    for dim, value in d.coords.items():
        found.add(dim)
    assert found == set([Dim.X, Dim.Y, Dim.Z])
