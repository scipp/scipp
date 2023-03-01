# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
import copy
import operator

import numpy as np
import pytest

import scipp as sc


def test_create_from_kwargs():
    dg = sc.DataGroup(a=4, b=6)

    assert tuple(dg.keys()) == ('a', 'b')


def test_create_from_dict_works_with_mixed_types_and_non_scipp_objects():
    items = {
        'a': 1, 'b': sc.scalar(2), 'c': sc.DataArray(sc.scalar(3)), 'd': np.arange(4)
    }
    dg = sc.DataGroup(items)
    assert tuple(dg.keys()) == ('a', 'b', 'c', 'd')


def test_init_raises_when_keys_are_not_strings():
    d = {1: 0}
    with pytest.raises(ValueError):
        sc.DataGroup(d)


@pytest.mark.parametrize(
    'copy_func', [copy.deepcopy, sc.DataGroup.copy, lambda x: x.copy(deep=True)])
def test_deepcopy(copy_func):
    items = {
        'a': sc.scalar(2), 'b': np.arange(4), 'c': sc.DataGroup(nested=sc.scalar(1))
    }
    dg = sc.DataGroup(items)
    result = copy_func(dg)
    result['a'] += 1
    result['b'] += 1
    result['c']['nested'] += 1
    result['c']['new'] = 1
    result['new'] = 1
    assert sc.identical(dg['a'], sc.scalar(2))
    assert sc.identical(result['a'], sc.scalar(3))
    assert np.array_equal(dg['b'], np.arange(4))
    assert np.array_equal(result['b'], np.arange(4) + 1)
    assert sc.identical(dg['c']['nested'], sc.scalar(1))
    assert sc.identical(result['c']['nested'], sc.scalar(2))
    assert 'new' not in dg
    assert 'new' not in dg['c']


@pytest.mark.parametrize('copy_func', [copy.copy, lambda x: x.copy(deep=False)])
def test_copy(copy_func):
    items = {
        'a': sc.scalar(2), 'b': np.arange(4), 'c': sc.DataGroup(nested=sc.scalar(1))
    }
    dg = sc.DataGroup(items)
    result = copy_func(dg)
    result['a'] += 1
    result['b'] += 1
    result['c']['nested'] += 1
    result['c']['new'] = 1
    result['new'] = 1
    assert sc.identical(dg['a'], sc.scalar(3))
    assert sc.identical(result['a'], sc.scalar(3))
    assert np.array_equal(dg['b'], np.arange(4) + 1)
    assert np.array_equal(result['b'], np.arange(4) + 1)
    assert sc.identical(dg['c']['nested'], sc.scalar(2))
    assert sc.identical(result['c']['nested'], sc.scalar(2))
    assert 'new' not in dg
    assert 'new' in dg['c']


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
    assert sc.identical(dg['a'], sc.scalar(1))


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
    # least for now the implementation makes no attempt at this.
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
    dg = sc.DataGroup({'a': sc.arange('x', 4, dtype='int64')})
    assert sc.identical(dg['x', 2], sc.DataGroup({'a': sc.scalar(2)}))


def test_getitem_positional_indexing_leaves_scalar_items_untouched():
    dg = sc.DataGroup({'x': sc.arange('x', 4), 's': sc.scalar(2)})
    assert sc.identical(dg['x', 2]['s'], dg['s'])


def test_getitem_positional_indexing_leaves_independent_items_untouched():
    dg = sc.DataGroup({'x': sc.arange('x', 4), 'y': sc.arange('y', 2)})
    assert sc.identical(dg['x', 2]['y'], dg['y'])


def test_getitem_positional_indexing_without_dim_label_works_for_1d():
    dg = sc.DataGroup({'a': sc.arange('x', 4, dtype='int64')})
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
    with pytest.raises(sc.DimensionError):
        dg2d[::]


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


def test_getitem_boolean_variable_indexing():
    dg = sc.DataGroup({'a': sc.arange('x', 4), 'b': sc.arange('x', 4)})
    sel = sc.array(dims=['x'], values=[False, True, False, True])
    result = dg[sel]
    assert sc.identical(result['a'], dg['a'][sel])
    assert sc.identical(result['b'], dg['b'][sel])


def test_getitem_boolean_variable_indexing_raises_on_dim_mismatch():
    dg = sc.DataGroup({'a': sc.arange('x', 4), 'b': sc.arange('x', 5)})
    sel = sc.array(dims=['x'], values=[False, True, False, True])
    with pytest.raises(sc.DimensionError):
        dg[sel]


def test_getitem_integer_array_indexing_single_dim_var_length():
    dg = sc.DataGroup({'a': sc.arange('x', 4), 'b': sc.arange('x', 5)})
    result = dg[[1, 3]]
    assert sc.identical(result['a'], dg['a'][[1, 3]])
    assert sc.identical(result['b'], dg['b'][[1, 3]])


def test_getitem_integer_array_indexing_multi_dim_raises_without_dim():
    dg = sc.DataGroup({'a': sc.arange('x', 4), 'b': sc.arange('y', 5)})
    with pytest.raises(sc.DimensionError):
        dg[[1, 3]]


def test_getitem_integer_array_indexing_multi_dim():
    dg = sc.DataGroup({'x': sc.arange('x', 4), 'y': sc.arange('y', 5)})
    result = dg['x', [1, 3]]
    assert sc.identical(result['x'], dg['x'][[1, 3]])
    assert sc.identical(result['y'], dg['y'])


def test_getitem_integer_array_indexing_with_numpy_array():
    dg = sc.DataGroup({'x': sc.arange('x', 4), 'y': sc.arange('y', 5)})
    assert sc.identical(dg['x', np.array([1, 3])], dg['x', [1, 3]])


BINARY_NUMBER_OPERATIONS = (
    lambda a,
    b: a + b,
    lambda a,
    b: a - b,
    lambda a,
    b: a * b,
    lambda a,
    b: a / b,
    lambda a,
    b: a // b,
    lambda a,
    b: a % b,
    lambda a,
    b: a == b,
    lambda a,
    b: a != b,
    lambda a,
    b: a > b,
    lambda a,
    b: a >= b,
    lambda a,
    b: a < b,
    lambda a,
    b: a <= b,
)
BINARY_NUMBER_OPERATION_NAMES = ('+',
                                 '-',
                                 '*',
                                 '/',
                                 '//',
                                 '%',
                                 '==',
                                 '!=',
                                 '>',
                                 '>=',
                                 '<',
                                 '<=')


def reverse_op(op, reverse: bool):
    if not reverse:
        return op
    return lambda a, b: op(b, a)


@pytest.mark.parametrize('op',
                         BINARY_NUMBER_OPERATIONS,
                         ids=BINARY_NUMBER_OPERATION_NAMES)
def test_binary_data_group_with_data_group(op):
    x = sc.arange('x', 1, 5, unit='m')
    dg1 = sc.DataGroup({'a': x})
    dg2 = sc.DataGroup({'a': 2 * x, 'b': x})
    result = op(dg1, dg2)
    assert 'a' in result
    assert 'b' not in result
    assert sc.identical(result['a'], op(x, 2 * x))


@pytest.mark.parametrize('op',
                         BINARY_NUMBER_OPERATIONS,
                         ids=BINARY_NUMBER_OPERATION_NAMES)
def test_binary_data_group_with_nested_data_group(op):
    x = sc.arange('x', 1, 5, unit='m')
    dg1 = sc.DataGroup({'a': x})
    inner = sc.DataGroup({'b': 2 * x, 'c': -x})
    dg2 = sc.DataGroup({'a': inner})
    result = op(dg1, dg2)
    assert 'a' in result
    assert 'b' not in result
    assert 'c' not in result
    assert sc.identical(result['a'], op(x, inner))


@pytest.mark.parametrize('op',
                         BINARY_NUMBER_OPERATIONS,
                         ids=BINARY_NUMBER_OPERATION_NAMES)
def test_binary_nested_data_group_with_data_group(op):
    x = sc.arange('x', 1, 5, unit='m')
    inner = sc.DataGroup({'b': 2 * x, 'c': -x})
    dg1 = sc.DataGroup({'a': inner})
    dg2 = sc.DataGroup({'a': x})
    result = op(dg1, dg2)
    assert 'a' in result
    assert 'b' not in result
    assert 'c' not in result
    assert sc.identical(result['a'], op(inner, x))


@pytest.mark.parametrize('op',
                         BINARY_NUMBER_OPERATIONS,
                         ids=BINARY_NUMBER_OPERATION_NAMES)
@pytest.mark.parametrize('typ', (int, float))
@pytest.mark.parametrize('reverse', (False, True))
def test_arithmetic_data_group_with_builtin(op, typ, reverse):
    op = reverse_op(op, reverse)
    x = sc.arange('x', 1, 5)
    dg = sc.DataGroup({'a': x})
    other = typ(31)
    result = op(dg, other)
    assert 'a' in result
    assert sc.identical(result['a'], op(x, other))


@pytest.mark.parametrize('op',
                         BINARY_NUMBER_OPERATIONS,
                         ids=BINARY_NUMBER_OPERATION_NAMES)
@pytest.mark.parametrize('reverse', (False, True))
def test_arithmetic_data_group_with_variable(op, reverse):
    op = reverse_op(op, reverse)
    x = sc.arange('x', 1, 5, unit='m')
    dg = sc.DataGroup({'a': x})
    other = 2 * x
    result = op(dg, other)
    assert 'a' in result
    assert sc.identical(result['a'], op(x, other))


@pytest.mark.parametrize('op',
                         BINARY_NUMBER_OPERATIONS,
                         ids=BINARY_NUMBER_OPERATION_NAMES)
@pytest.mark.parametrize('reverse', (False, True))
def test_arithmetic_data_group_with_data_array(op, reverse):
    op = reverse_op(op, reverse)
    x = sc.DataArray(sc.arange('x', 1, 5, unit='m'), coords={'x': sc.arange('x', 5)})
    dg = sc.DataGroup({'a': x})
    other = 2 * x
    result = op(dg, other)
    assert 'a' in result
    assert sc.identical(result['a'], op(x, other))


@pytest.mark.parametrize('op',
                         BINARY_NUMBER_OPERATIONS,
                         ids=BINARY_NUMBER_OPERATION_NAMES)
def test_arithmetic_data_group_with_data_array_coord_mismatch(op):
    x = sc.DataArray(sc.arange('x', 1, 5, unit='m'), coords={'x': sc.arange('x', 5)})
    dg = sc.DataGroup({'a': x.copy()})
    other = 2 * x
    other.coords['x'][0] = -1
    with pytest.raises(sc.DatasetError):
        op(dg, other)


def test_pow_data_group_with_data_group():
    x = sc.arange('x', 1, 5, dtype='int64')
    dg1 = sc.DataGroup({'a': x})
    dg2 = sc.DataGroup({'a': 2 * x, 'b': x})
    result = dg1**dg2
    assert 'a' in result
    assert 'b' not in result
    assert sc.identical(result['a'], x**(2 * x))


@pytest.mark.parametrize('other',
                         (3,
                          sc.arange('x', 0, 4, dtype='int64'),
                          sc.DataArray(sc.arange('x', 0, 4, dtype='int64'),
                                       coords={'x': sc.arange('x', 5, dtype='int64')})),
                         ids=('int', 'Variable', 'DataArray'))
@pytest.mark.parametrize('reverse', (False, True))
def test_pow_data_group_with_other(other, reverse):
    op = reverse_op(lambda a, b: a**b, reverse)
    x = sc.arange('x', 1, 5, dtype='int64')
    dg = sc.DataGroup({'a': x})
    result = op(dg, other)
    assert 'a' in result
    assert sc.identical(result['a'], op(x, other))


BINARY_LOGIC_OPERATIONS = (
    lambda a, b: a & b,
    lambda a, b: a | b,
    lambda a, b: a ^ b,
)
BINARY_LOGIC_OPERATION_NAMES = ('&', '|', '^')


@pytest.mark.parametrize('op',
                         BINARY_LOGIC_OPERATIONS,
                         ids=BINARY_LOGIC_OPERATION_NAMES)
def test_logical_data_group_with_data_group(op):
    x = sc.array(dims=['l'], values=[True, False, False, True])
    y = sc.array(dims=['l'], values=[True, False, True, False])
    dg1 = sc.DataGroup({'a': x})
    dg2 = sc.DataGroup({'a': y})
    result = op(dg1, dg2)
    assert sc.identical(result['a'], op(x, y))


def test_logical_not():
    x = sc.array(dims=['l'], values=[True, False, False])
    dg = sc.DataGroup({'a': x})
    result = ~dg
    assert sc.identical(result['a'], ~x)


@pytest.mark.parametrize('op',
                         (operator.iadd,
                          operator.isub,
                          operator.imul,
                          operator.imod,
                          operator.itruediv,
                          operator.ifloordiv,
                          operator.ipow))
def test_inplace_is_disabled(op):
    x = sc.arange('x', 1, 5, dtype='int64')
    dg1 = sc.DataGroup({'a': x.copy()})
    dg2 = sc.DataGroup({'a': x.copy()})
    with pytest.raises(TypeError):
        op(dg1, dg2)


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


def test_elemwise_unary():
    dg = sc.DataGroup(a=sc.linspace('x', 0.0, 1.0, num=4, unit='rad'))
    result = sc.sin(dg)
    assert isinstance(result, sc.DataGroup)
    assert sc.identical(result['a'], sc.sin(dg['a']))


def test_elemwise_binary():
    dg1 = sc.DataGroup(a=sc.linspace('x', 0.0, 1.0, num=4, unit='rad'))
    dg2 = sc.DataGroup(a=sc.linspace('x', 0.0, 2.0, num=4, unit='rad'))
    result = sc.add(dg1, dg2)
    assert isinstance(result, sc.DataGroup)
    assert sc.identical(result['a'], sc.add(dg1['a'], dg2['a']))


def test_elemwise_binary_return_intersection_of_keys():
    dg1 = sc.DataGroup(a=sc.scalar(1), b=sc.scalar(2))
    dg2 = sc.DataGroup(a=sc.scalar(3), c=sc.scalar(4))
    result = sc.add(dg1, dg2)
    assert set(result.keys()) == {'a'}
    assert sc.identical(result['a'], sc.add(dg1['a'], dg2['a']))


def test_elemwise_with_kwargs():
    dg1 = sc.DataGroup(a=sc.linspace('x', 0.0, 1.0, num=4))
    dg2 = sc.DataGroup(a=sc.linspace('x', 0.0, 1.0, num=4))
    result = sc.atan2(y=dg1, x=dg2)
    assert isinstance(result, sc.DataGroup)
    assert sc.identical(result['a'], sc.atan2(y=dg1['a'], x=dg2['a']))


def test_elemwise_unary_raises_with_out_arg():
    dg = sc.DataGroup(a=sc.linspace('x', 0.0, 1.0, num=4, unit='rad'))
    out = sc.DataGroup()
    with pytest.raises(ValueError):
        sc.sin(dg, out=out)


def test_construction_from_dataset_creates_dataarray_items():
    ds = sc.Dataset({'a': sc.scalar(1), 'b': sc.scalar(2)}, coords={'x': sc.scalar(3)})
    dg = sc.DataGroup(ds)
    assert len(dg) == 2
    assert sc.identical(dg['a'], ds['a'])
    assert sc.identical(dg['b'], ds['b'])


def test_dataset_can_be_created_from_datagroup_with_variable_or_dataarray_items():
    dg = sc.DataGroup(a=sc.DataArray(sc.arange('x', 4),
                                     coords={'x': sc.linspace('x', 0.0, 1.0, 4)}),
                      b=sc.arange('x', 4))
    ds = sc.Dataset(dg)
    assert len(ds) == 2
    assert sc.identical(dg['a'], ds['a'])
    assert sc.identical(dg['b'], ds['b'].data)


def test_fold_flatten():
    dg = sc.DataGroup(a=sc.arange('x', 4), b=sc.arange('x', 6))
    assert sc.identical(dg.fold('x', sizes={'y': 2, 'z': -1}).flatten(to='x'), dg)


def test_squeeze():
    dg = sc.DataGroup(a=sc.ones(dims=['x', 'y'], shape=(4, 1)))
    assert sc.identical(dg.squeeze(), sc.DataGroup(a=sc.ones(dims=['x'], shape=(4, ))))


def test_transpose():
    dg = sc.DataGroup(a=sc.ones(dims=['x', 'y'], shape=(2, 3)))
    assert sc.identical(dg.transpose(),
                        sc.DataGroup(a=sc.ones(dims=['y', 'x'], shape=(3, 2))))


def test_broadcast():
    dg = sc.DataGroup(a=sc.scalar(1))
    assert sc.identical(dg.broadcast(sizes={'x': 2}),
                        sc.DataGroup(a=sc.scalar(1).broadcast(sizes={'x': 2})))


def test_methods_work_with_any_value_type_that_supports_it():
    dg = sc.DataGroup(a=sc.arange('x', 4), b=np.arange(5))
    result = dg.max()
    assert sc.identical(result['a'], sc.arange('x', 4).max())
    assert np.array_equal(result['b'], np.arange(5).max())


def test_bins_property_indexing():
    table = sc.data.table_xyz(10)
    dg = sc.DataGroup(a=table)
    dg = dg.bin(x=10)
    result = dg.bins['x', sc.scalar(0.5, unit='m'):]
    assert sc.identical(result['a'], dg['a'].bins['x', sc.scalar(0.5, unit='m'):])


def test_merge():
    dg1 = sc.DataGroup(a=sc.arange('x', 6), b='a string', d=np.array([1, 2]))
    dg2 = sc.DataGroup(c=sc.DataArray(sc.arange('y', 3),
                                      coords={'y': -sc.arange('y', 3)}),
                       b='a string',
                       d=np.array([1, 2]))
    merged = sc.merge(dg1, dg2)
    assert set(merged.keys()) == {'a', 'b', 'c', 'd'}
    assert sc.identical(merged['a'], dg1['a'])
    assert sc.identical(merged['c'], dg2['c'])
    assert merged['b'] == dg1['b']
    np.testing.assert_array_equal(merged['d'], dg1['d'])


def test_merge_conflict_scipp_object():
    dg1 = sc.DataGroup(a=sc.arange('x', 6), b='a string')
    dg2 = sc.DataGroup(a=-sc.arange('x', 6), b='a string')
    with pytest.raises(sc.DatasetError):
        sc.merge(dg1, dg2)


def test_merge_conflict_numpy_array():
    dg1 = sc.DataGroup(a=sc.arange('x', 6), d=np.array([1, 2]))
    dg2 = sc.DataGroup(a=sc.arange('x', 6), d=np.array([3, 4, 5]))
    with pytest.raises(sc.DatasetError):
        sc.merge(dg1, dg2)


def test_merge_conflict_python_object():
    dg1 = sc.DataGroup(a=sc.arange('x', 6), b='a string')
    dg2 = sc.DataGroup(c=sc.DataArray(sc.arange('y', 3),
                                      coords={'y': -sc.arange('y', 3)}),
                       b='another string')
    with pytest.raises(sc.DatasetError):
        sc.merge(dg1, dg2)


@pytest.mark.parametrize('func',
                         (sc.bin, lambda x, /, *args, **kwargs: x.bin(*args, **kwargs)))
def test_bin(func):
    dg = sc.DataGroup({
        'da': sc.DataArray(10 * sc.arange('x', 5.0), coords={'x': sc.arange('x', 5.0)}),
        'dg': sc.DataGroup({
            'inner_da': sc.DataArray(10 * sc.arange('x', 50.0),
                                     coords={'x': sc.arange('x', 50.0)})
        })
    })
    edges = {'x': 3}
    expected = sc.DataGroup({
        'da': dg['da'].bin(**edges),
        'dg': sc.DataGroup({'inner_da': dg['dg']['inner_da'].bin(**edges)})
    })
    assert sc.identical(func(dg, **edges), expected)
