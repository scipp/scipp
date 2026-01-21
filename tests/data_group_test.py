# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
import copy
import operator

import numpy as np
import pytest

import scipp as sc
from scipp.testing import assert_identical


def test_create_from_kwargs() -> None:
    dg = sc.DataGroup(a=4, b=6)

    assert tuple(dg.keys()) == ('a', 'b')


def test_create_from_dict_works_with_mixed_types_and_non_scipp_objects() -> None:
    items = {
        'a': 1,
        'b': sc.scalar(2),
        'c': sc.DataArray(sc.scalar(3)),
        'd': np.arange(4),
    }

    dg = sc.DataGroup(items)
    assert tuple(dg.keys()) == ('a', 'b', 'c', 'd')


def test_init_raises_when_keys_are_not_strings() -> None:
    d = {1: 0}
    with pytest.raises(ValueError, match='DataGroup keys must be strings'):
        sc.DataGroup(d)


@pytest.mark.parametrize(
    'copy_func', [copy.deepcopy, sc.DataGroup.copy, lambda x: x.copy(deep=True)]
)
def test_deepcopy(copy_func) -> None:
    items = {
        'a': sc.scalar(2),
        'b': np.arange(4),
        'c': sc.DataGroup(nested=sc.scalar(1)),
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
def test_copy(copy_func) -> None:
    items = {
        'a': sc.scalar(2),
        'b': np.arange(4),
        'c': sc.DataGroup(nested=sc.scalar(1)),
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


def test_len() -> None:
    assert len(sc.DataGroup()) == 0
    assert len(sc.DataGroup({'a': 0})) == 1
    assert len(sc.DataGroup({'a': 0, 'b': 1})) == 2


def test_iter_empty() -> None:
    dg = sc.DataGroup()
    it = iter(dg)
    with pytest.raises(StopIteration):
        next(it)


def test_iter_nonempty() -> None:
    dg = sc.DataGroup({'a': sc.scalar(1), 'b': sc.scalar(2)})
    it = iter(dg)
    assert next(it) == 'a'
    assert next(it) == 'b'
    with pytest.raises(StopIteration):
        next(it)


def test_getitem_str_key() -> None:
    dg = sc.DataGroup({'a': sc.scalar(1), 'b': sc.scalar(2)})
    assert sc.identical(dg['a'], sc.scalar(1))
    assert sc.identical(dg['b'], sc.scalar(2))


def test_setitem_str_key_adds_new() -> None:
    dg = sc.DataGroup()
    dg['a'] = sc.scalar(1)
    assert sc.identical(dg['a'], sc.scalar(1))


def test_setitem_str_key_replace() -> None:
    dg = sc.DataGroup({'a': sc.scalar(1)})
    dg['a'] = sc.scalar(2)
    assert sc.identical(dg['a'], sc.scalar(2))


def test_setitem_nonstr_key_fails() -> None:
    dg = sc.DataGroup(a=sc.arange('x', 2))
    with pytest.raises(TypeError):
        dg[0] = sc.scalar(1)


def test_delitem_removes_item() -> None:
    dg = sc.DataGroup({'a': sc.scalar(1)})
    del dg['a']
    assert 'a' not in dg


def test_delitem_raises_KeyError_when_not_found() -> None:
    dg = sc.DataGroup({'a': sc.scalar(1)})
    with pytest.raises(KeyError):
        del dg['b']


def test_dims_with_scipp_objects_combines_dims_in_insertion_order() -> None:
    assert sc.DataGroup({'a': sc.scalar(1)}).dims == ()
    assert sc.DataGroup({'a': sc.ones(dims=('x',), shape=(2,))}).dims == ('x',)
    assert sc.DataGroup(
        {'a': sc.ones(dims=('x',), shape=(2,)), 'b': sc.ones(dims=('y',), shape=(2,))}
    ).dims == ('x', 'y')
    assert sc.DataGroup(
        {'b': sc.ones(dims=('y',), shape=(2,)), 'a': sc.ones(dims=('x',), shape=(2,))}
    ).dims == ('y', 'x')
    # In this case there would be a better order, but in general there is not, so at
    # least for now the implementation makes no attempt at this.
    assert sc.DataGroup(
        {
            'a': sc.ones(dims=('x', 'z'), shape=(2, 2)),
            'b': sc.ones(dims=('y', 'z'), shape=(2, 2)),
        }
    ).dims == ('x', 'z', 'y')


def test_ndim() -> None:
    assert sc.DataGroup({'a': sc.scalar(1)}).ndim == 0
    assert sc.DataGroup({'a': sc.ones(dims=('x',), shape=(1,))}).ndim == 1
    assert sc.DataGroup({'a': sc.ones(dims=('x', 'y'), shape=(1, 1))}).ndim == 2
    assert (
        sc.DataGroup(
            {
                'a': sc.ones(dims=('x',), shape=(2,)),
                'b': sc.ones(dims=('y',), shape=(2,)),
            }
        ).ndim
        == 2
    )


def test_non_scipp_objects_are_considered_to_have_0_dims() -> None:
    assert sc.DataGroup({'a': np.arange(4)}).dims == ()
    assert sc.DataGroup({'a': np.arange(4)}).ndim == 0


def test_shape_and_sizes() -> None:
    dg = sc.DataGroup(
        {
            'a': sc.ones(dims=('x', 'z'), shape=(2, 4)),
            'b': sc.ones(dims=('y', 'z'), shape=(3, 4)),
        }
    )
    assert dg.shape == (2, 4, 3)
    assert dg.sizes == {'x': 2, 'z': 4, 'y': 3}


def test_inconsistent_shapes_are_reported_as_None() -> None:
    dg = sc.DataGroup({'x1': sc.arange('x', 4)})
    assert dg.shape == (4,)
    dg['x2'] = sc.arange('x', 5)
    assert dg.shape == (None,)
    dg['y1'] = sc.arange('y', 5)
    assert dg.shape == (None, 5)
    dg['y2'] = sc.arange('y', 6)
    assert dg.shape == (None, None)
    del dg['x1']
    assert dg.shape == (5, None)
    del dg['y1']
    assert dg.shape == (5, 6)


def test_numpy_arrays_are_not_considered_for_shape() -> None:
    assert sc.DataGroup({'a': np.arange(4)}).shape == ()


def test_getitem_positional_indexing() -> None:
    dg = sc.DataGroup({'a': sc.arange('x', 4, dtype='int64')})
    assert sc.identical(dg['x', 2], sc.DataGroup({'a': sc.scalar(2)}))


def test_getitem_positional_indexing_leaves_scalar_items_untouched() -> None:
    dg = sc.DataGroup({'x': sc.arange('x', 4), 's': sc.scalar(2)})
    assert sc.identical(dg['x', 2]['s'], dg['s'])


def test_getitem_positional_indexing_leaves_independent_items_untouched() -> None:
    dg = sc.DataGroup({'x': sc.arange('x', 4), 'y': sc.arange('y', 2)})
    assert sc.identical(dg['x', 2]['y'], dg['y'])


def test_getitem_positional_indexing_without_dim_label_works_for_1d() -> None:
    dg = sc.DataGroup({'a': sc.arange('x', 4, dtype='int64')})
    assert sc.identical(dg[2], sc.DataGroup({'a': sc.scalar(2)}))


def test_getitem_positional_indexing_without_dim_label_raises_unless_1d() -> None:
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


def test_getitem_positional_slice_does_not_raise_when_length_is_not_unique() -> None:
    dg = sc.DataGroup({'a': sc.arange('x', 4), 'b': sc.arange('x', 5)})
    dg['x', 2:3]
    dg['x', 2:]
    dg['x', 2:10]
    dg['x', 2:4]
    dg['x', -1:]
    dg['x', -10:]


def test_getitem_positional_indexing_raise_when_one_is_empty() -> None:
    dg = sc.DataGroup({'a': sc.array(dims='x', values=[]), 'b': sc.arange('x', 5)})
    with pytest.raises(IndexError):
        dg['x', 2]


def test_getitem_positional_indexing_raise_when_out_of_range() -> None:
    dg = sc.DataGroup({'a': sc.arange('x', 3), 'b': sc.arange('x', 5)})
    with pytest.raises(IndexError):
        dg['x', 3]


def test_getitem_positional_indexing_treats_numpy_arrays_as_scalars() -> None:
    dg = sc.DataGroup({'a': sc.arange('x', 4), 'b': np.arange(4)})
    assert np.array_equal(dg[1]['b'], dg['b'])
    assert np.array_equal(dg['x', 1]['b'], dg['b'])


@pytest.mark.parametrize(
    's',
    [
        slice(*v)
        for v in ((2,), (2, 3), (2, None), (2, 10), (2, 4), (-1, None), (-10, None))
    ],
)
def test_getitem_positional_slicing_works_when_length_is_not_unique(s) -> None:
    da1 = sc.DataArray(sc.arange('x', 4), coords={'x': sc.linspace('x', 0.0, 1.0, 4)})
    da2 = sc.DataArray(sc.arange('x', 5), coords={'x': sc.linspace('x', 0.0, 1.0, 5)})
    dg = sc.DataGroup({'a': da1, 'b': da2})
    result = dg['x', s]
    sc.testing.assert_identical(result['a'], da1['x', s])
    sc.testing.assert_identical(result['b'], da2['x', s])


def test_getitem_label_indexing_based_works_when_length_is_not_unique() -> None:
    da1 = sc.DataArray(sc.arange('x', 4), coords={'x': sc.linspace('x', 0.0, 1.0, 4)})
    da2 = sc.DataArray(sc.arange('x', 5), coords={'x': sc.linspace('x', 0.0, 1.0, 5)})
    dg = sc.DataGroup({'a': da1, 'b': da2})
    result = dg['x', : sc.scalar(0.5)]
    assert sc.identical(result['a'], da1['x', : sc.scalar(0.5)])
    assert sc.identical(result['b'], da2['x', : sc.scalar(0.5)])


def test_getitem_boolean_variable_indexing() -> None:
    dg = sc.DataGroup({'a': sc.arange('x', 4), 'b': sc.arange('x', 4)})
    sel = sc.array(dims=['x'], values=[False, True, False, True])
    result = dg[sel]
    assert sc.identical(result['a'], dg['a'][sel])
    assert sc.identical(result['b'], dg['b'][sel])


def test_getitem_boolean_variable_indexing_raises_on_dim_mismatch() -> None:
    dg = sc.DataGroup({'a': sc.arange('x', 4), 'b': sc.arange('x', 5)})
    sel = sc.array(dims=['x'], values=[False, True, False, True])
    with pytest.raises(sc.DimensionError):
        dg[sel]


def test_getitem_integer_array_indexing_single_dim_var_length() -> None:
    dg = sc.DataGroup({'a': sc.arange('x', 4), 'b': sc.arange('x', 5)})
    result = dg[[1, 3]]
    assert sc.identical(result['a'], dg['a'][[1, 3]])
    assert sc.identical(result['b'], dg['b'][[1, 3]])


def test_getitem_integer_array_indexing_multi_dim_raises_without_dim() -> None:
    dg = sc.DataGroup({'a': sc.arange('x', 4), 'b': sc.arange('y', 5)})
    with pytest.raises(sc.DimensionError):
        dg[[1, 3]]


def test_getitem_integer_array_indexing_multi_dim() -> None:
    dg = sc.DataGroup({'x': sc.arange('x', 4), 'y': sc.arange('y', 5)})
    result = dg['x', [1, 3]]
    assert sc.identical(result['x'], dg['x'][[1, 3]])
    assert sc.identical(result['y'], dg['y'])


def test_getitem_integer_array_indexing_with_numpy_array() -> None:
    dg = sc.DataGroup({'x': sc.arange('x', 4), 'y': sc.arange('y', 5)})
    assert sc.identical(dg['x', np.array([1, 3])], dg['x', [1, 3]])


BINARY_NUMBER_OPERATIONS = (
    lambda a, b: a + b,
    lambda a, b: a - b,
    lambda a, b: a * b,
    lambda a, b: a / b,
    lambda a, b: a // b,
    lambda a, b: a % b,
    lambda a, b: a == b,
    lambda a, b: a != b,
    lambda a, b: a > b,
    lambda a, b: a >= b,
    lambda a, b: a < b,
    lambda a, b: a <= b,
)
BINARY_NUMBER_OPERATION_NAMES = (
    '+',
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
    '<=',
)


def reverse_op(op, reverse: bool):
    if not reverse:
        return op
    return lambda a, b: op(b, a)


@pytest.mark.parametrize(
    'op', BINARY_NUMBER_OPERATIONS, ids=BINARY_NUMBER_OPERATION_NAMES
)
def test_binary_data_group_with_data_group(op) -> None:
    x = sc.arange('x', 1, 5, unit='m')
    dg1 = sc.DataGroup({'a': x})
    dg2 = sc.DataGroup({'a': 2 * x, 'b': x})
    result = op(dg1, dg2)
    assert 'a' in result
    assert 'b' not in result
    assert sc.identical(result['a'], op(x, 2 * x))


@pytest.mark.parametrize(
    'op', BINARY_NUMBER_OPERATIONS, ids=BINARY_NUMBER_OPERATION_NAMES
)
def test_binary_data_group_with_data_group_preserves_key_order(op) -> None:
    x = sc.arange('x', 1, 5, unit='m')
    dg1 = sc.DataGroup({'a': 3 * x, 'x': x, 'b': -x})
    dg2 = sc.DataGroup({'b': x, 'x': 2 * x})

    result = op(dg1, dg2)
    assert list(result.keys()) == ['x', 'b']

    result = op(dg2, dg1)
    assert list(result.keys()) == ['b', 'x']


@pytest.mark.parametrize(
    'op', BINARY_NUMBER_OPERATIONS, ids=BINARY_NUMBER_OPERATION_NAMES
)
def test_binary_data_group_with_nested_data_group(op) -> None:
    x = sc.arange('x', 1, 5, unit='m')
    dg1 = sc.DataGroup({'a': x})
    inner = sc.DataGroup({'b': 2 * x, 'c': -x})
    dg2 = sc.DataGroup({'a': inner})
    result = op(dg1, dg2)
    assert 'a' in result
    assert 'b' not in result
    assert 'c' not in result
    assert sc.identical(result['a'], op(x, inner))


@pytest.mark.parametrize(
    'op', BINARY_NUMBER_OPERATIONS, ids=BINARY_NUMBER_OPERATION_NAMES
)
def test_binary_nested_data_group_with_data_group(op) -> None:
    x = sc.arange('x', 1, 5, unit='m')
    inner = sc.DataGroup({'b': 2 * x, 'c': -x})
    dg1 = sc.DataGroup({'a': inner})
    dg2 = sc.DataGroup({'a': x})
    result = op(dg1, dg2)
    assert 'a' in result
    assert 'b' not in result
    assert 'c' not in result
    assert sc.identical(result['a'], op(inner, x))


@pytest.mark.parametrize(
    'op', BINARY_NUMBER_OPERATIONS, ids=BINARY_NUMBER_OPERATION_NAMES
)
@pytest.mark.parametrize('typ', [int, float])
@pytest.mark.parametrize('reverse', [False, True])
def test_arithmetic_data_group_with_builtin(op, typ, reverse) -> None:
    op = reverse_op(op, reverse)
    x = sc.arange('x', 1, 5)
    dg = sc.DataGroup({'a': x})
    other = typ(31)
    result = op(dg, other)
    assert 'a' in result
    assert sc.identical(result['a'], op(x, other))


@pytest.mark.parametrize(
    'op', BINARY_NUMBER_OPERATIONS, ids=BINARY_NUMBER_OPERATION_NAMES
)
@pytest.mark.parametrize('reverse', [False, True])
def test_arithmetic_data_group_with_variable(op, reverse) -> None:
    op = reverse_op(op, reverse)
    x = sc.arange('x', 1, 5, unit='m')
    dg = sc.DataGroup({'a': x})
    other = 2 * x
    result = op(dg, other)
    assert 'a' in result
    assert sc.identical(result['a'], op(x, other))


@pytest.mark.parametrize(
    'op', BINARY_NUMBER_OPERATIONS, ids=BINARY_NUMBER_OPERATION_NAMES
)
@pytest.mark.parametrize('reverse', [False, True])
def test_arithmetic_data_group_with_data_array(op, reverse) -> None:
    op = reverse_op(op, reverse)
    x = sc.DataArray(sc.arange('x', 1, 5, unit='m'), coords={'x': sc.arange('x', 5)})
    dg = sc.DataGroup({'a': x})
    other = 2 * x
    result = op(dg, other)
    assert 'a' in result
    assert sc.identical(result['a'], op(x, other))


@pytest.mark.parametrize(
    'op', BINARY_NUMBER_OPERATIONS, ids=BINARY_NUMBER_OPERATION_NAMES
)
def test_arithmetic_data_group_with_data_array_coord_mismatch(op) -> None:
    x = sc.DataArray(sc.arange('x', 1, 5, unit='m'), coords={'x': sc.arange('x', 5)})
    dg = sc.DataGroup({'a': x.copy()})
    other = 2 * x
    other.coords['x'][0] = -1
    with pytest.raises(sc.DatasetError):
        op(dg, other)


def test_pow_data_group_with_data_group() -> None:
    x = sc.arange('x', 1, 5, dtype='int64')
    dg1 = sc.DataGroup({'a': x})
    dg2 = sc.DataGroup({'a': 2 * x, 'b': x})
    result = dg1**dg2
    assert 'a' in result
    assert 'b' not in result
    assert sc.identical(result['a'], x ** (2 * x))


@pytest.mark.parametrize(
    'other',
    [
        3,
        sc.arange('x', 0, 4, dtype='int64'),
        sc.DataArray(
            sc.arange('x', 0, 4, dtype='int64'),
            coords={'x': sc.arange('x', 5, dtype='int64')},
        ),
    ],
    ids=('int', 'Variable', 'DataArray'),
)
@pytest.mark.parametrize('reverse', [False, True])
def test_pow_data_group_with_other(other, reverse) -> None:
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


@pytest.mark.parametrize(
    'op', BINARY_LOGIC_OPERATIONS, ids=BINARY_LOGIC_OPERATION_NAMES
)
def test_logical_data_group_with_data_group(op) -> None:
    x = sc.array(dims=['l'], values=[True, False, False, True])
    y = sc.array(dims=['l'], values=[True, False, True, False])
    dg1 = sc.DataGroup({'a': x})
    dg2 = sc.DataGroup({'a': y})
    result = op(dg1, dg2)
    assert sc.identical(result['a'], op(x, y))


def test_logical_not() -> None:
    x = sc.array(dims=['l'], values=[True, False, False])
    dg = sc.DataGroup({'a': x})
    result = ~dg
    assert sc.identical(result['a'], ~x)


@pytest.mark.parametrize(
    'op',
    [
        operator.iadd,
        operator.isub,
        operator.imul,
        operator.imod,
        operator.itruediv,
        operator.ifloordiv,
        operator.ipow,
    ],
)
def test_inplace_is_disabled(op) -> None:
    x = sc.arange('x', 1, 5, dtype='int64')
    dg1 = sc.DataGroup({'a': x.copy()})
    dg2 = sc.DataGroup({'a': x.copy()})
    with pytest.raises(TypeError):
        op(dg1, dg2)


def test_hist() -> None:
    table = sc.data.table_xyz(1000)
    dg = sc.DataGroup()
    dg['a'] = table[:100]
    dg['b'] = table[100:]
    hists = dg.hist(x=10)
    assert sc.identical(hists['a'], table[:100].hist(x=10))
    assert sc.identical(hists['b'], table[100:].hist(x=10))


def test_bins_property() -> None:
    table = sc.data.table_xyz(1000)
    dg = sc.DataGroup()
    dg['a'] = table.bin(x=10)
    dg['b'] = table.bin(x=12)
    result = dg.bins.sum()
    assert sc.identical(result['a'], table.hist(x=10))
    assert sc.identical(result['b'], table.hist(x=12))


def test_groupby() -> None:
    table = sc.data.table_xyz(100)
    dg = sc.DataGroup()
    dg['a'] = table[:60]
    dg['b'] = table[60:]
    result = dg.groupby('x').sum('row')
    assert sc.identical(result['a'], table[:60].groupby('x').sum('row'))
    assert sc.identical(result['b'], table[60:].groupby('x').sum('row'))


def test_elemwise_unary() -> None:
    dg = sc.DataGroup(a=sc.linspace('x', 0.0, 1.0, num=4, unit='rad'))
    result = sc.sin(dg)
    assert isinstance(result, sc.DataGroup)
    assert sc.identical(result['a'], sc.sin(dg['a']))


def test_elemwise_binary() -> None:
    dg1 = sc.DataGroup(a=sc.linspace('x', 0.0, 1.0, num=4, unit='rad'))
    dg2 = sc.DataGroup(a=sc.linspace('x', 0.0, 2.0, num=4, unit='rad'))
    result = sc.add(dg1, dg2)
    assert isinstance(result, sc.DataGroup)
    assert sc.identical(result['a'], sc.add(dg1['a'], dg2['a']))


def test_elemwise_binary_return_intersection_of_keys() -> None:
    dg1 = sc.DataGroup(a=sc.scalar(1), b=sc.scalar(2))
    dg2 = sc.DataGroup(a=sc.scalar(3), c=sc.scalar(4))
    result = sc.add(dg1, dg2)
    assert set(result.keys()) == {'a'}
    assert sc.identical(result['a'], sc.add(dg1['a'], dg2['a']))


def test_elemwise_with_kwargs() -> None:
    dg1 = sc.DataGroup(a=sc.linspace('x', 0.0, 1.0, num=4))
    dg2 = sc.DataGroup(a=sc.linspace('x', 0.0, 1.0, num=4))
    result = sc.atan2(y=dg1, x=dg2)
    assert isinstance(result, sc.DataGroup)
    assert sc.identical(result['a'], sc.atan2(y=dg1['a'], x=dg2['a']))


def test_elemwise_unary_raises_with_out_arg() -> None:
    dg = sc.DataGroup(a=sc.linspace('x', 0.0, 1.0, num=4, unit='rad'))
    out = sc.DataGroup()
    with pytest.raises(ValueError, match='`out` argument is not supported'):
        sc.sin(dg, out=out)


def test_construction_from_dataset_creates_dataarray_items() -> None:
    ds = sc.Dataset({'a': sc.scalar(1), 'b': sc.scalar(2)}, coords={'x': sc.scalar(3)})
    dg = sc.DataGroup(ds)
    assert len(dg) == 2
    assert sc.identical(dg['a'], ds['a'])
    assert sc.identical(dg['b'], ds['b'])


def test_dataset_can_be_created_from_datagroup_with_variable_or_dataarray_items() -> (
    None
):
    dg = sc.DataGroup(
        a=sc.DataArray(sc.arange('x', 4), coords={'x': sc.linspace('x', 0.0, 1.0, 4)}),
        b=sc.arange('x', 4),
    )
    ds = sc.Dataset(dg)
    assert len(ds) == 2
    assert sc.identical(dg['a'], ds['a'])
    assert sc.identical(dg['b'], ds['b'].data)


def test_fold_flatten() -> None:
    dg = sc.DataGroup(a=sc.arange('x', 4), b=sc.arange('x', 6))
    assert sc.identical(dg.fold('x', sizes={'y': 2, 'z': -1}).flatten(to='x'), dg)


def test_fold_ignores_python_objects() -> None:
    dg = sc.DataGroup(a=sc.arange('x', 4, dtype='int64'), b='abc')
    assert sc.identical(
        dg.fold('x', sizes={'y': 2, 'z': -1}),
        sc.DataGroup(
            a=sc.array(dims=['y', 'z'], values=[[0, 1], [2, 3]], dtype='int64'), b='abc'
        ),
    )


def test_fold_skips_items_that_lack_input_dim() -> None:
    dg = sc.DataGroup(
        a=sc.arange('x', 4, dtype='int64'),
        b=sc.arange('other', 2, dtype='int64'),
        c=sc.scalar(1),
        d='abc',
    )
    expected = dg.copy()
    expected['a'] = sc.array(dims=['y', 'z'], values=[[0, 1], [2, 3]], dtype='int64')
    assert_identical(dg.fold('x', sizes={'y': 2, 'z': -1}), expected)


def test_flatten_preserves_items_that_lack_input_dim() -> None:
    dg = sc.DataGroup(
        a=sc.array(dims=['y', 'z'], values=[[0, 1], [2, 3]], dtype='int64'),
        b=sc.arange('other', 2, dtype='int64'),
        c=sc.scalar(1),
        d='abc',
    )
    expected = dg.copy()
    expected['a'] = sc.arange('x', 4, dtype='int64')
    assert_identical(dg.flatten(dims=['y', 'z'], to='x'), expected)


def test_fold_works_with_existing_dim_in_sibling() -> None:
    dg = sc.DataGroup(
        a=sc.arange('x', 4, dtype='int64'), b=sc.arange('y', 2, dtype='int64')
    )
    assert sc.identical(
        dg.fold('x', sizes={'y': 2, 'z': -1}),
        sc.DataGroup(
            a=sc.array(dims=['y', 'z'], values=[[0, 1], [2, 3]], dtype='int64'),
            b=dg['b'],
        ),
    )


def test_squeeze() -> None:
    dg = sc.DataGroup(a=sc.ones(dims=['x', 'y'], shape=(4, 1)))
    assert sc.identical(dg.squeeze(), sc.DataGroup(a=sc.ones(dims=['x'], shape=(4,))))


def test_squeeze_works_with_binned() -> None:
    content = sc.ones(dims=['row'], shape=[6])
    binned = sc.bins(begin=sc.index(0), end=sc.index(6), data=content, dim='row')
    dg = sc.DataGroup(a=binned)
    assert sc.identical(dg.squeeze(), dg)


def test_squeeze_leaves_numpy_array_unchanged() -> None:
    dg = sc.DataGroup(a=np.ones(shape=(4, 1)))
    assert sc.identical(dg.squeeze(), sc.DataGroup(a=np.ones(shape=(4, 1))))


def test_squeeze_ignores_python_objects() -> None:
    dg = sc.DataGroup(a='abc', b=sc.ones(dims=['x', 'y'], shape=(4, 1)))
    assert sc.identical(
        dg.squeeze(), sc.DataGroup(a='abc', b=sc.ones(dims=['x'], shape=(4,)))
    )


def test_transpose() -> None:
    dg = sc.DataGroup(a=sc.ones(dims=['x', 'y'], shape=(2, 3)))
    assert sc.identical(
        dg.transpose(), sc.DataGroup(a=sc.ones(dims=['y', 'x'], shape=(3, 2)))
    )


def test_transpose_leaves_numpy_array_unchanged() -> None:
    dg = sc.DataGroup(a=np.ones(shape=(2, 3)))
    assert sc.identical(dg.transpose(), sc.DataGroup(a=np.ones(shape=(2, 3))))


def test_transpose_ignores_python_objects() -> None:
    dg = sc.DataGroup(a=sc.ones(dims=['x', 'y'], shape=(2, 3)), b='abc')
    assert sc.identical(
        dg.transpose(), sc.DataGroup(a=sc.ones(dims=['y', 'x'], shape=(3, 2)), b='abc')
    )


def test_broadcast() -> None:
    dg = sc.DataGroup(a=sc.scalar(1))
    assert sc.identical(
        dg.broadcast(sizes={'x': 2}),
        sc.DataGroup(a=sc.scalar(1).broadcast(sizes={'x': 2})),
    )


def test_bins_property_indexing() -> None:
    table = sc.data.table_xyz(10)
    dg = sc.DataGroup(a=table)
    dg = dg.bin(x=10)
    result = dg.bins['x', sc.scalar(0.5, unit='m') :]
    assert sc.identical(result['a'], dg['a'].bins['x', sc.scalar(0.5, unit='m') :])


def test_merge() -> None:
    dg1 = sc.DataGroup(a=sc.arange('x', 6), b='a string', d=np.array([1, 2]))
    dg2 = sc.DataGroup(
        c=sc.DataArray(sc.arange('y', 3), coords={'y': -sc.arange('y', 3)}),
        b='a string',
        d=np.array([1, 2]),
    )
    merged = sc.merge(dg1, dg2)
    assert set(merged.keys()) == {'a', 'b', 'c', 'd'}
    assert sc.identical(merged['a'], dg1['a'])
    assert sc.identical(merged['c'], dg2['c'])
    assert merged['b'] == dg1['b']
    np.testing.assert_array_equal(merged['d'], dg1['d'])


def test_merge_conflict_scipp_object() -> None:
    dg1 = sc.DataGroup(a=sc.arange('x', 6), b='a string')
    dg2 = sc.DataGroup(a=-sc.arange('x', 6), b='a string')
    with pytest.raises(sc.DatasetError):
        sc.merge(dg1, dg2)


def test_merge_conflict_numpy_array() -> None:
    dg1 = sc.DataGroup(a=sc.arange('x', 6), d=np.array([1, 2]))
    dg2 = sc.DataGroup(a=sc.arange('x', 6), d=np.array([3, 4, 5]))
    with pytest.raises(sc.DatasetError):
        sc.merge(dg1, dg2)


def test_merge_conflict_python_object() -> None:
    dg1 = sc.DataGroup(a=sc.arange('x', 6), b='a string')
    dg2 = sc.DataGroup(
        c=sc.DataArray(sc.arange('y', 3), coords={'y': -sc.arange('y', 3)}),
        b='another string',
    )
    with pytest.raises(sc.DatasetError):
        sc.merge(dg1, dg2)


@pytest.mark.parametrize(
    'func', [sc.bin, lambda x, /, *args, **kwargs: x.bin(*args, **kwargs)]
)
def test_bin(func) -> None:
    dg = sc.DataGroup(
        {
            'da': sc.DataArray(
                10 * sc.arange('x', 5.0), coords={'x': sc.arange('x', 5.0)}
            ),
            'dg': sc.DataGroup(
                {
                    'inner_da': sc.DataArray(
                        10 * sc.arange('x', 50.0), coords={'x': sc.arange('x', 50.0)}
                    )
                }
            ),
        }
    )
    edges = {'x': 3}
    expected = sc.DataGroup(
        {
            'da': dg['da'].bin(**edges),
            'dg': sc.DataGroup({'inner_da': dg['dg']['inner_da'].bin(**edges)}),
        }
    )
    assert sc.identical(func(dg, **edges), expected)


bin_reduction_ops = (
    'sum',
    'mean',
    'all',
    'any',
    'min',
    'max',
    'nansum',
    'nanmean',
    'nanmin',
    'nanmax',
)

# These reductions are supported by DataGroup but not by Bins / binned data.
reduction_ops = (
    *bin_reduction_ops,
    'median',
    'std',
    'var',
    'nanmedian',
    'nanstd',
    'nanvar',
)


@pytest.mark.parametrize('dim', ['x', None])
@pytest.mark.parametrize('op', reduction_ops)
def test_reduction_op_ignores_python_objects(op, dim) -> None:
    dtype = 'bool' if op in ('all', 'any') else 'float64'
    if 'std' in op or 'var' in op:
        op = operator.methodcaller(op, dim=dim, ddof=0)
    else:
        op = operator.methodcaller(op, dim=dim)
    dg = sc.DataGroup(a=sc.ones(dims=['x', 'y'], shape=(2, 3), dtype=dtype), b='abc')
    result = op(dg)
    assert_identical(result['a'], op(dg['a']))
    assert result['b'] == dg['b']


@pytest.mark.parametrize('dim', ['x', None])
@pytest.mark.parametrize('op', reduction_ops)
def test_reduction_op_ignores_numpy_arrays(op, dim) -> None:
    dtype = 'bool' if op in ('all', 'any') else 'float64'
    if 'std' in op or 'var' in op:
        op = operator.methodcaller(op, dim=dim, ddof=0)
    else:
        op = operator.methodcaller(op, dim=dim)
    dg = sc.DataGroup(
        a=sc.ones(dims=['x', 'y'], shape=(2, 3), dtype=dtype), b=np.ones((4,))
    )
    result = op(dg)
    assert_identical(result['a'], op(dg['a']))
    assert np.array_equal(result['b'], dg['b'])


@pytest.mark.parametrize('op', reduction_ops)
def test_reduction_op_ignores_objects_lacking_given_dim(op) -> None:
    dtype = 'bool' if op in ('all', 'any') else 'float64'
    if 'std' in op or 'var' in op:
        op = operator.methodcaller(op, dim='x', ddof=0)
    else:
        op = operator.methodcaller(op, dim='x')
    dg = sc.DataGroup(
        a=sc.ones(dims=['x', 'y'], shape=(2, 3), dtype=dtype),
        b=sc.ones(dims=['y'], shape=(2,), dtype=dtype),
    )
    result = op(dg)
    assert_identical(result['a'], op(dg['a']))
    assert_identical(result['b'], dg['b'])


@pytest.mark.parametrize('dim', ['x', None])
@pytest.mark.parametrize('opname', bin_reduction_ops)
def test_reduction_op_handles_bin_without_dim(opname, dim) -> None:
    dtype = 'bool' if opname in ('all', 'any') else 'float64'
    op = operator.methodcaller(opname, dim=dim)
    content = sc.ones(dims=['row'], shape=[6], dtype=dtype)
    binned = sc.bins(begin=sc.index(0), end=sc.index(6), data=content, dim='row')
    dg = sc.DataGroup(a=sc.ones(dims=['x', 'y'], shape=(2, 3), dtype=dtype), b=binned)
    result = op(dg)
    assert_identical(result['a'], op(dg['a']))
    assert_identical(result['b'], operator.methodcaller(opname)(content))


@pytest.mark.parametrize('opname', bin_reduction_ops)
def test_reduction_op_handles_bins_reduction(opname) -> None:
    dtype = 'bool' if opname in ('all', 'any') else 'float64'
    op = operator.methodcaller(opname)
    content = sc.ones(dims=['row'], shape=[6], dtype=dtype)
    binned = sc.bins(begin=sc.index(0), end=sc.index(6), data=content, dim='row')
    dg = sc.DataGroup(a=binned)
    result = op(dg.bins)
    assert_identical(result['a'], operator.methodcaller(opname)(content))


@pytest.mark.parametrize('opname', reduction_ops)
def test_reduction_op_handles_dim_with_empty_name(opname) -> None:
    dtype = 'bool' if opname in ('all', 'any') else 'float64'
    if 'std' in opname or 'var' in opname:
        op = operator.methodcaller(opname, dim='', ddof=0)
    else:
        op = operator.methodcaller(opname, dim='')
    dg = sc.DataGroup(
        a=sc.ones(dims=['', 'y'], shape=(3, 2), dtype=dtype),
        b=sc.ones(dims=[''], shape=(2,), dtype=dtype),
    )
    result = op(dg)
    assert '' not in result.dims
    assert_identical(result['a'], op(dg['a']))
    assert_identical(result['b'], op(dg['b']))
