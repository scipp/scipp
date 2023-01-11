# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
import numpy as np
import pytest

import scipp as sc


def test_create_from_dict_works_with_mixed_types_and_non_scipp_objects():
    items = {
        'a': 1,
        'b': sc.scalar(2),
        'c': sc.DataArray(sc.scalar(3)),
        'd': np.arange(4)
    }
    dg = sc.DataGroup(items)
    assert tuple(dg.keys()) == ('a', 'b', 'c', 'd')


def test_len():
    assert len(sc.DataGroup()) == 0
    assert len(sc.DataGroup({'a': 0})) == 1
    assert len(sc.DataGroup({'a': 0, 'b': 1})) == 2


def test_iter_empty():
    dg = sc.DataGroup()
    it = iter(dg)
    with pytest.raises(StopIteration):
        next(it)


def test_iter_nonempty():
    dg = sc.DataGroup({'a': sc.scalar(1), 'b': sc.scalar(2)})
    it = iter(dg)
    assert next(it) == 'a'
    assert next(it) == 'b'
    with pytest.raises(StopIteration):
        next(it)


def test_getitem():
    dg = sc.DataGroup({'a': sc.scalar(1), 'b': sc.scalar(2)})
    assert sc.identical(dg['a'], sc.scalar(1))
    assert sc.identical(dg['b'], sc.scalar(2))


def test_setitem_adds_new():
    dg = sc.DataGroup()
    dg['a'] = sc.scalar(1)
    assert 'a' in dg


def test_setitem_replace():
    dg = sc.DataGroup({'a': sc.scalar(1)})
    dg['a'] = sc.scalar(2)
    assert sc.identical(dg['a'], sc.scalar(2))


def test_delitem_removes_item():
    dg = sc.DataGroup({'a': sc.scalar(1)})
    del dg['a']
    assert 'a' not in dg


def test_delitem_raises_KeyError_when_not_found():
    dg = sc.DataGroup({'a': sc.scalar(1)})
    with pytest.raises(KeyError):
        del dg['b']


def test_add():
    x = sc.arange('x', 4, unit='m')
    dg1 = sc.DataGroup({'a': x})
    dg2 = sc.DataGroup({'a': x, 'b': x})
    result = dg1 + dg2
    assert 'a' in result
    assert 'b' not in result
    assert sc.identical(result['a'], x + x)


def test_eq():
    x = sc.arange('x', 4, unit='m')
    dg1 = sc.DataGroup({'a': x})
    dg2 = sc.DataGroup({'a': x, 'b': x})
    result = dg1 == dg2
    assert 'a' in result
    assert 'b' not in result
    assert sc.identical(result['a'], x == x)


def test_hist():
    table = sc.data.table_xyz(1000)
    dg = sc.DataGroup()
    dg['a'] = table[:100]
    dg['b'] = table[100:]
    hists = dg.hist(x=10)
    assert sc.identical(hists['a'], table[:100].hist(x=10))
    assert sc.identical(hists['b'], table[100:].hist(x=10))


def test_bins_property():
    table = sc.data.table_xyz(1000)
    dg = sc.DataGroup()
    dg['a'] = table.bin(x=10)
    dg['b'] = table.bin(x=12)
    result = dg.bins.sum()
    assert sc.identical(result['a'], table.hist(x=10))
    assert sc.identical(result['b'], table.hist(x=12))


def test_groupby():
    table = sc.data.table_xyz(100)
    dg = sc.DataGroup()
    dg['a'] = table[:60]
    dg['b'] = table[60:]
    result = dg.groupby('x').sum('row')
    assert sc.identical(result['a'], table[:60].groupby('x').sum('row'))
    assert sc.identical(result['b'], table[60:].groupby('x').sum('row'))
