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


def test_getitem_str_key():
    dg = sc.DataGroup({'a': sc.scalar(1), 'b': sc.scalar(2)})
    assert sc.identical(dg['a'], sc.scalar(1))
    assert sc.identical(dg['b'], sc.scalar(2))


def test_setitem_str_key_adds_new():
    dg = sc.DataGroup()
    dg['a'] = sc.scalar(1)
    assert 'a' in dg


def test_setitem_str_key_replace():
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


def test_dims_with_scipp_objects_combines_dims_in_insertion_order():
    assert sc.DataGroup({'a': sc.scalar(1)}).dims == ()
    assert sc.DataGroup({'a': sc.ones(dims=('x', ), shape=(2, ))}).dims == ('x', )
    assert sc.DataGroup({
        'a': sc.ones(dims=('x', ), shape=(2, )),
        'b': sc.ones(dims=('y', ), shape=(2, ))
    }).dims == ('x', 'y')
    assert sc.DataGroup({
        'b': sc.ones(dims=('y', ), shape=(2, )),
        'a': sc.ones(dims=('x', ), shape=(2, ))
    }).dims == ('y', 'x')
    # In this case there would be a better order, but in general there is not, so at
    # leasr for now the implementation makes no attempt for this.
    assert sc.DataGroup({
        'a': sc.ones(dims=('x', 'z'), shape=(2, 2)),
        'b': sc.ones(dims=('y', 'z'), shape=(2, 2))
    }).dims == ('x', 'z', 'y')


def test_ndim():
    assert sc.DataGroup({'a': sc.scalar(1)}).ndim == 0
    assert sc.DataGroup({'a': sc.ones(dims=('x', ), shape=(1, ))}).ndim == 1
    assert sc.DataGroup({'a': sc.ones(dims=('x', 'y'), shape=(1, 1))}).ndim == 2
    assert sc.DataGroup({
        'a': sc.ones(dims=('x', ), shape=(2, )),
        'b': sc.ones(dims=('y', ), shape=(2, ))
    }).ndim == 2


def test_non_scipp_objects_are_considered_to_have_0_dims():
    assert sc.DataGroup({'a': np.arange(4)}).dims == ()
    assert sc.DataGroup({'a': np.arange(4)}).ndim == 0


def test_shape_and_sizes():
    dg = sc.DataGroup({
        'a': sc.ones(dims=('x', 'z'), shape=(2, 4)),
        'b': sc.ones(dims=('y', 'z'), shape=(3, 4))
    })
    assert dg.shape == (2, 4, 3)
    assert dg.sizes == {'x': 2, 'z': 4, 'y': 3}


def test_inconsistent_shapes_are_reported_as_None():
    dg = sc.DataGroup({'x1': sc.arange('x', 4)})
    assert dg.shape == (4, )
    dg['x2'] = sc.arange('x', 5)
    assert dg.shape == (None, )
    dg['y1'] = sc.arange('y', 5)
    assert dg.shape == (None, 5)
    dg['y2'] = sc.arange('y', 6)
    assert dg.shape == (None, None)
    del dg['x1']
    assert dg.shape == (5, None)
    del dg['y1']
    assert dg.shape == (5, 6)


def test_numpy_arrays_are_not_considered_for_shape():
    assert sc.DataGroup({'a': np.arange(4)}).shape == ()


def test_getitem_positional_indexing():
    dg = sc.DataGroup({'a': sc.arange('x', 4)})
    assert sc.identical(dg['x', 2], sc.DataGroup({'a': sc.scalar(2)}))


def test_getitem_positional_indexing_leaves_scalar_items_untouched():
    dg = sc.DataGroup({'x': sc.arange('x', 4), 's': sc.scalar(2)})
    assert sc.identical(dg['x', 2]['s'], dg['s'])


def test_getitem_positional_indexing_leaves_independent_items_untouched():
    dg = sc.DataGroup({'x': sc.arange('x', 4), 'y': sc.arange('y', 2)})
    assert sc.identical(dg['x', 2]['y'], dg['y'])


def test_getitem_positional_indexing_without_dim_label_works_for_1d():
    dg = sc.DataGroup({'a': sc.arange('x', 4)})
    assert sc.identical(dg[2], sc.DataGroup({'a': sc.scalar(2)}))


def test_getitem_positional_indexing_without_dim_label_raises_unless_1d():
    dg0d = sc.DataGroup({'a': sc.scalar(4)})
    with pytest.raises(sc.DimensionError):
        dg0d[0]
    dg2d = sc.DataGroup({'a': sc.ones(dims=('x', 'y'), shape=(1, 2))})
    with pytest.raises(sc.DimensionError):
        dg2d[0]
    with pytest.raises(sc.DimensionError):
        dg2d[np.int32(0)]
    with pytest.raises(sc.DimensionError):
        dg2d[:0]
    with pytest.raises(sc.DimensionError):
        dg2d[1:]
    with pytest.raises(sc.DimensionError):
        dg2d[:]
    with pytest.raises(sc.DimensionError):
        dg2d[::2]


def test_getitem_positional_indexing_raises_when_length_is_not_unique():
    dg = sc.DataGroup({'a': sc.arange('x', 4), 'b': sc.arange('x', 5)})
    with pytest.raises(sc.DimensionError):
        dg['x', 2]
    with pytest.raises(sc.DimensionError):
        dg['x', 2:3]


def test_getitem_positional_indexing_treats_numpy_arrays_as_scalars():
    dg = sc.DataGroup({'a': sc.arange('x', 4), 'b': np.arange(4)})
    assert np.array_equal(dg[1]['b'], dg['b'])
    assert np.array_equal(dg['x', 1]['b'], dg['b'])


def test_getitem_label_indexing_based_works_when_length_is_not_unique():
    da1 = sc.DataArray(sc.arange('x', 4), coords={'x': sc.linspace('x', 0.0, 1.0, 4)})
    da2 = sc.DataArray(sc.arange('x', 5), coords={'x': sc.linspace('x', 0.0, 1.0, 5)})
    dg = sc.DataGroup({'a': da1, 'b': da2})
    result = dg['x', :sc.scalar(0.5)]
    assert sc.identical(result['a'], da1['x', :sc.scalar(0.5)])
    assert sc.identical(result['b'], da2['x', :sc.scalar(0.5)])


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
