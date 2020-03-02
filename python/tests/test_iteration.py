# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import pytest

import scipp as sc
from scipp import Dim


@pytest.fixture(params=['Dataset', 'DatasetView'])
def dataset_abc(request):
    d = sc.Dataset()
    d['a'] = sc.Variable(['x'], values=[1, 2])
    d['b'] = sc.Variable(['x'], values=[3, 4])
    d['c'] = sc.Variable(['x'], values=[5, 6])
    # Using yield so `d` does not go out of scope when returning slice
    if request.param == 'Dataset':
        yield d
    else:
        yield d['x', 0]


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
    d.coords['x'] = 1.0 * sc.units.m
    d.coords['y'] = 2.0 * sc.units.m
    d.coords['z'] = 3.0 * sc.units.m
    return d


def test_dataset_coords_iter():
    d = make_coords_xyz()
    found = set()
    for key in d.coords:
        found.add(str(key))
    assert found == set(['x', 'y', 'z'])


def test_dataset_coords_keys():
    d = make_coords_xyz()
    assert set(d.coords.keys()) == set([Dim('x'), Dim('y'), Dim('z')])


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
        found.add(str(dim))
    assert found == set(['x', 'y', 'z'])
