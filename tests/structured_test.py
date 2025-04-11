# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

import numpy as np

import scipp as sc

var = sc.vector(value=np.array([1, 2, 3]), unit=sc.units.m)


def test_structured_fields_dict_like() -> None:
    assert 'x' in var.fields
    assert 'y' in var.fields
    assert 'z' in var.fields
    assert 't' not in var.fields
    assert set(var.fields.keys()) == {'x', 'y', 'z'}
    found = []
    for field in var.fields:
        found.append(field)
        assert sc.identical(var.fields[field], getattr(var.fields, field))
    assert set(found) == {'x', 'y', 'z'}
    var2 = var.copy()
    var2.fields['x'] += 1.0 * sc.units.m
    assert sc.identical(var2, sc.vector(value=np.array([2, 2, 3]), unit=sc.units.m))


def test_structured_fields_keys_values() -> None:
    keys = list(var.fields.keys())
    values = list(var.fields.values())
    for items in [dict(zip(keys, values, strict=True)), dict(var.fields.items())]:
        assert len(keys) == 3
        assert len(values) == 3
        assert sc.identical(items['x'], 1.0 * sc.units.m)
        assert sc.identical(items['y'], 2.0 * sc.units.m)
        assert sc.identical(items['z'], 3.0 * sc.units.m)
