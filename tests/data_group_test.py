# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
import scipp as sc


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
