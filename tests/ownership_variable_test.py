# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen
from copy import copy, deepcopy
from typing import Any

import numpy as np

import scipp as sc


def make_variable(data: Any, variances: Any = None, **kwargs: Any) -> sc.Variable:
    """
    Make a Variable with default dimensions from data
    while avoiding copies beyond what sc.Variable does.
    """
    if isinstance(data, list | tuple):
        data = np.array(data)
    if variances is not None and isinstance(variances, list | tuple):
        variances = np.array(variances)
    if isinstance(data, np.ndarray):
        dims = ['x', 'y'][: np.ndim(data)]
        return sc.array(dims=dims, values=data, variances=variances, **kwargs)
    return sc.scalar(data, **kwargs)


def test_own_var_scalar_fundamental() -> None:
    # Python does not allow for any sharing with fundamental types.
    x = 1
    s = sc.scalar(x)
    s.value = 2
    assert s.value == 2
    assert x == 1


def test_own_var_scalar_copy() -> None:
    # Depth of copies of variables can be controlled.
    v = make_variable(1.0, variance=10.0, unit='m')
    v_copy = copy(v)
    v_deepcopy = deepcopy(v)
    v_methcopy = v.copy(deep=False)
    v_methdeepcopy = v.copy(deep=True)

    v.value = 2.0
    v.variance = 20.0
    v.unit = 's'
    assert sc.identical(v, make_variable(2.0, variance=20.0, unit='s'))
    assert sc.identical(v_copy, make_variable(2.0, variance=20.0, unit='s'))
    assert sc.identical(v_deepcopy, make_variable(1.0, variance=10.0, unit='m'))
    assert sc.identical(v_methcopy, make_variable(2.0, variance=20.0, unit='s'))
    assert sc.identical(v_methdeepcopy, make_variable(1.0, variance=10.0, unit='m'))


def test_own_var_scalar_pyobj_set() -> None:
    # Input data is shared.
    x = {'num': 1, 'list': [2, 3]}
    s = make_variable(x)
    x['num'] = -1
    s.value['list'][0] = -2
    assert sc.identical(s, make_variable({'num': -1, 'list': [-2, 3]}))
    assert x == {'num': -1, 'list': [-2, 3]}


def test_own_var_scalar_pyobj_get() -> None:
    # .value getter shares ownership of the object.
    s = make_variable({'num': 1, 'list': [2, 3]})
    x = s.value
    x['num'] = -1
    s.value['list'][0] = -2
    expected = {'num': -1, 'list': [-2, 3]}
    assert sc.identical(s, make_variable(expected))
    assert x == expected


def test_own_var_scalar_pyobj_copy() -> None:
    # Depth of copies of variables can be controlled.
    data = {'num': 1, 'list': [2, 3]}
    s = make_variable(deepcopy(data))
    s_copy = copy(s)
    s_deepcopy = deepcopy(s)
    s_methcopy = s.copy(deep=False)
    s_methdeepcopy = s.copy(deep=True)

    s.value['num'] = -1
    s.value['list'][0] = -2
    modified = {'num': -1, 'list': [-2, 3]}
    assert sc.identical(s, make_variable(modified))
    assert sc.identical(s_copy, make_variable(modified))
    assert sc.identical(s_deepcopy, make_variable(data))
    assert sc.identical(s_methcopy, make_variable(modified))
    assert sc.identical(s_methdeepcopy, make_variable(data))


def test_own_var_scalar_str_get() -> None:
    # Python strings are immutable, no references are shared.
    s = make_variable('abc')
    x = s.value
    s.value = 'def'
    assert sc.identical(s, make_variable('def'))
    assert x == 'abc'


def test_own_var_scalar_str_copy() -> None:
    # Depth of copies of variables can be controlled.
    s = make_variable('abc')
    s_copy = copy(s)
    s_deepcopy = deepcopy(s)
    s_methcopy = s.copy(deep=False)
    s_methdeepcopy = s.copy(deep=True)

    s.value = 'def'
    assert sc.identical(s, make_variable('def'))
    assert sc.identical(s_copy, make_variable('def'))
    assert sc.identical(s_deepcopy, make_variable('abc'))
    assert sc.identical(s_methcopy, make_variable('def'))
    assert sc.identical(s_methdeepcopy, make_variable('abc'))


def test_own_var_scalar_nested_set() -> None:
    # Variables are shared when nested.
    a = np.arange(5)
    inner = make_variable(a, unit='m')
    outer = make_variable(inner)
    outer.value['x', 0] = -1
    outer.value.values[1] = -2
    inner['x', 2] = -3
    inner.unit = 's'
    a[3] = -4
    assert sc.identical(
        outer, make_variable(make_variable([-1, -2, -3, 3, 4], unit='s'))
    )
    assert sc.identical(inner, make_variable([-1, -2, -3, 3, 4], unit='s'))
    np.testing.assert_array_equal(a, [0, 1, 2, -4, 4])

    # Assignment references the RHS and replaces the former content.
    new_inner = make_variable(np.arange(5, 8), unit='kg')
    outer.value = new_inner
    outer.value['x', 0] = -5
    assert sc.identical(outer, make_variable(make_variable([-5, 6, 7], unit='kg')))
    assert sc.identical(new_inner, make_variable([-5, 6, 7], unit='kg'))
    assert sc.identical(inner, make_variable([-1, -2, -3, 3, 4], unit='s'))

    # TODO this causes and infinite loop and segfaults at the moment
    # outer.value = outer
    # assert sc.identical(outer, outer.value)


def test_own_var_scalar_nested_get() -> None:
    # Variables are shared when nested.
    outer = make_variable(make_variable(np.arange(5)))
    inner = outer.value
    a = inner.values
    outer.value['x', 0] = -1
    outer.value.values[1] = -2
    inner['x', 2] = -3
    a[3] = -4
    expected = [-1, -2, -3, -4, 4]
    assert sc.identical(outer, make_variable(make_variable(expected)))
    assert sc.identical(inner, make_variable(expected))
    np.testing.assert_array_equal(a, expected)

    # Assignment replaces the buffer of the inner Variable
    # without touching the original buffer.
    outer.value = make_variable(np.arange(5, 8))
    outer.value['x', 0] = -5
    inner['x', 1] = -10
    assert sc.identical(outer, make_variable(make_variable([-5, -10, 7])))
    assert sc.identical(inner, make_variable([-5, -10, 7]))
    np.testing.assert_array_equal(a, [-1, -2, -3, -4, 4])


def test_own_var_scalar_nested_copy() -> None:
    # Depth of copies of variables can be controlled.
    outer = make_variable(make_variable(np.arange(5)))
    inner = outer.value
    a = inner.values
    outer_copy = copy(outer)
    outer_deepcopy = deepcopy(outer)
    outer_methcopy = outer.copy(deep=False)
    outer_methdeepcopy = outer.copy(deep=True)

    outer.value['x', 0] = -1
    outer.value.values[1] = -2
    inner['x', 2] = -3
    a[3] = -4
    modified = make_variable(make_variable([-1, -2, -3, -4, 4]))
    original = make_variable(make_variable([0, 1, 2, 3, 4]))
    assert sc.identical(outer, modified)
    assert sc.identical(outer_copy, modified)
    assert sc.identical(outer_deepcopy, original)
    assert sc.identical(outer_methcopy, modified)
    assert sc.identical(outer_methdeepcopy, original)


def test_own_var_scalar_nested_str_get() -> None:
    # Variables are shared when nested.
    outer = make_variable(make_variable(np.array(['abc', 'def'])))
    inner = outer.value
    a = inner.values
    # Assignment writes the values into the inner Variable.
    outer.value = make_variable(np.array(['ghi', 'jkl', 'mno']))
    outer.value['x', 0] = sc.scalar('asd')
    inner['x', 1] = sc.scalar('qwe')
    assert sc.identical(
        outer, make_variable(make_variable(np.array(['asd', 'qwe', 'mno'])))
    )
    assert sc.identical(inner, make_variable(np.array(['asd', 'qwe', 'mno'])))
    np.testing.assert_array_equal(a, ['abc', 'def'])


def test_own_var_1d_set() -> None:
    # Input arrays are not shared.
    a = np.arange(5)
    v = make_variable(a)
    v['x', 0] = -1
    v.values[1] = -2
    a[2] = -3
    v['x', 3:] = [-4, -5]
    assert sc.identical(v, make_variable([-1, -2, 2, -4, -5]))
    np.testing.assert_array_equal(a, np.array([0, 1, -3, 3, 4]))


def test_own_var_1d_get() -> None:
    # .values getter shares ownership of the array.
    v = make_variable(np.arange(5))
    a = v.values
    v['x', 0] = -1
    v.values[1] = -2
    a[2] = -3
    v['x', 3:] = [-4, -5]
    expected = [-1, -2, -3, -4, -5]
    assert sc.identical(v, make_variable(expected))
    np.testing.assert_array_equal(a, expected)


def test_own_var_1d_copy() -> None:
    # Depth of copies of variables can be controlled.
    v = make_variable(np.arange(5.0), unit='m')
    v_copy = copy(v)
    v_deepcopy = deepcopy(v)
    v_methcopy = v.copy(deep=False)
    v_methdeepcopy = v.copy(deep=True)

    v.values = np.arange(5.0, 10)
    v['x', 0] = -1.0
    v.unit = 's'
    modified = make_variable([-1.0, 6.0, 7.0, 8.0, 9.0], unit='s')
    original = make_variable([0.0, 1.0, 2.0, 3.0, 4.0], unit='m')
    assert sc.identical(v, modified)
    assert sc.identical(v_copy, modified)
    assert sc.identical(v_deepcopy, original)
    assert sc.identical(v_methcopy, modified)
    assert sc.identical(v_methdeepcopy, original)

    v.variances = np.arange(10.0, 15.0)
    np.testing.assert_array_equal(v_copy.variances, v.variances)
    assert v_deepcopy.variances is None
    np.testing.assert_array_equal(v_methcopy.variances, v.variances)
    assert v_methdeepcopy.variances is None

    v_copy = copy(v)
    v_deepcopy = deepcopy(v)
    v_methcopy = v.copy(deep=False)
    v_methdeepcopy = v.copy(deep=True)
    v.values[1] = -2.0
    v.variances[1] = -12.0
    v.unit = 'kg'
    original = make_variable(
        [-1.0, 6.0, 7.0, 8.0, 9.0], variances=[10.0, 11.0, 12.0, 13.0, 14.0], unit='s'
    )
    modified = make_variable(
        [-1.0, -2.0, 7.0, 8.0, 9.0],
        variances=[10.0, -12.0, 12.0, 13.0, 14.0],
        unit='kg',
    )
    assert sc.identical(v, modified)
    assert sc.identical(v_copy, modified)
    assert sc.identical(v_deepcopy, original)
    assert sc.identical(v_methcopy, modified)
    assert sc.identical(v_methdeepcopy, original)


def test_own_var_1d_pyobj_set() -> None:
    # Input data is deep-copied.
    x = {'num': 1, 'list': [2, 3]}
    y = {'num': 4, 'list': [5, 6]}
    v = sc.concat([make_variable(x), make_variable(y)], dim='x')
    v['x', 0].value['num'] = -1
    v.values[0]['list'][0] = -2
    y['num'] = -4
    assert sc.identical(
        v,
        sc.concat(
            [
                make_variable({'num': -1, 'list': [-2, 3]}),
                make_variable({'num': 4, 'list': [5, 6]}),
            ],
            dim='x',
        ),
    )
    assert x == {'num': 1, 'list': [2, 3]}
    assert y == {'num': -4, 'list': [5, 6]}


def test_own_var_1d_pyobj_get() -> None:
    # .values getter shares ownership of the array.
    v = sc.concat(
        [
            make_variable({'num': 1, 'list': [2, 3]}),
            make_variable({'num': 4, 'list': [5, 6]}),
        ],
        dim='x',
    )
    x = v['x', 0].value
    y = v['x', 1].value
    x['num'] = -1
    v['x', 0].value['list'][0] = -2
    v.values[1]['num'] = -5
    v.values[1]['list'][0] = -6
    x_expected = {'num': -1, 'list': [-2, 3]}
    y_expected = {'num': -5, 'list': [-6, 6]}
    assert sc.identical(
        v, sc.concat([make_variable(x_expected), make_variable(y_expected)], dim='x')
    )
    assert x == x_expected
    assert y == y_expected


def test_own_var_1d_pyobj_copy() -> None:
    # Depth of copies of variables can be controlled.
    x = make_variable({'num': 1, 'list': [2, 3]})
    y = make_variable({'num': 4, 'list': [5, 6]})
    v = sc.concat([x, y], dim='x')
    v_copy = copy(v)
    v_deepcopy = deepcopy(v)
    v_methcopy = v.copy(deep=False)
    v_methdeepcopy = v.copy(deep=True)

    v.values[0]['num'] = -1
    v.values[0]['list'][0] = -2
    modified = sc.concat([make_variable({'num': -1, 'list': [-2, 3]}), y], dim='x')
    original = sc.concat([make_variable({'num': 1, 'list': [2, 3]}), y], dim='x')
    assert sc.identical(v, modified)
    assert sc.identical(v_copy, modified)
    assert sc.identical(v_deepcopy, original)
    assert sc.identical(v_methcopy, modified)
    assert sc.identical(v_methdeepcopy, original)


def test_own_var_1d_str_set() -> None:
    # Input arrays are not shared.
    a = np.array(['abc', 'def'])
    v = make_variable(a)
    v['x', 0] = sc.scalar('xyz')
    assert sc.identical(v, make_variable(np.array(['xyz', 'def'])))
    np.testing.assert_array_equal(a, np.array(['abc', 'def']))


def test_own_var_1d_str_get() -> None:
    # .values getter shares ownership of the array.
    v = make_variable(np.array(['abc', 'def']))
    a = v.values
    v['x', 0] = sc.scalar('xyz')
    a[1] = 'ghi'
    assert sc.identical(v, make_variable(np.array(['xyz', 'ghi'])))
    np.testing.assert_array_equal(a, ['xyz', 'ghi'])

    # Assignment writes to existing memory.
    v.values = np.array(['asd', 'qwe'])
    assert sc.identical(v, make_variable(np.array(['asd', 'qwe'])))
    np.testing.assert_array_equal(a, ['asd', 'qwe'])


def test_own_var_1d_str_copy() -> None:
    # Depth of copies of variables can be controlled.
    v = make_variable(np.array(['abc', 'def']))
    v_copy = copy(v)
    v_deepcopy = deepcopy(v)
    v_methcopy = v.copy(deep=False)
    v_methdeepcopy = v.copy(deep=True)

    v['x', 0] = sc.scalar('asd')
    v.values[1] = 'qwe'
    assert sc.identical(v, make_variable(np.array(['asd', 'qwe'])))
    assert sc.identical(v_copy, make_variable(np.array(['asd', 'qwe'])))
    assert sc.identical(v_deepcopy, make_variable(np.array(['abc', 'def'])))
    assert sc.identical(v_methcopy, make_variable(np.array(['asd', 'qwe'])))
    assert sc.identical(v_methdeepcopy, make_variable(np.array(['abc', 'def'])))


def test_own_var_1d_bin_set() -> None:
    # The buffer is shared.
    a_buffer = np.arange(5)
    a_indices = np.array([0, 2, 5], dtype=np.int64)
    buffer = make_variable(a_buffer, unit='m')
    indices = make_variable(a_indices, dtype=sc.DType.int64, unit=None)
    binned = sc.bins(
        data=buffer, begin=indices['x', :-1], end=indices['x', 1:], dim='x'
    )
    binned['x', 0].value['x', 0] = -1
    binned.values[0]['x', 1] = -2
    binned.bins.constituents['data']['x', 2] = -3
    binned.bins.constituents['data'].unit = 's'
    buffer['x', 3] = -4
    a_buffer[4] = -5
    assert sc.identical(
        binned,
        sc.bins(
            data=make_variable([-1, -2, -3, -4, 4], unit='s'),
            begin=indices['x', :-1],
            end=indices['x', 1:],
            dim='x',
        ),
    )
    assert sc.identical(buffer, make_variable([-1, -2, -3, -4, 4], unit='s'))
    np.testing.assert_array_equal(a_buffer, [0, 1, 2, 3, -5])

    # Assigning new Variables to constituents succeeds but does nothing.
    binned.bins.constituents['data'] = make_variable([6, 7, 8, 9])
    binned.bins.constituents['begin'] = make_variable([1, 3])
    binned.bins.constituents['end'] = make_variable([3, 5])
    assert sc.identical(
        binned,
        sc.bins(
            data=make_variable([-1, -2, -3, -4, 4], unit='s'),
            begin=indices['x', :-1],
            end=indices['x', 1:],
            dim='x',
        ),
    )

    # Bin begin/end indices can be changed, there is no safety check
    binned.bins.constituents['begin']['x', 0] = 1
    binned.bins.constituents['end']['x', -1] = 4
    indices['x', 1] = 1
    assert sc.identical(
        binned,
        sc.bins(
            data=make_variable([-1, -2, -3, -4, 4], unit='s'),
            begin=make_variable([1, 2], dtype=sc.DType.int64, unit=None),
            end=make_variable([2, 4], dtype=sc.DType.int64, unit=None),
            dim='x',
        ),
    )


def test_own_var_1d_bin_get() -> None:
    # The buffer is shared.
    indices = make_variable(np.array([0, 2, 5]), dtype=sc.DType.int64, unit=None)
    binned = sc.bins(
        data=make_variable(np.arange(5), unit='m'),
        begin=indices['x', :-1],
        end=indices['x', 1:],
        dim='x',
    )
    buffer = binned.bins.constituents['data']
    binned['x', 0].value['x', 0] = -1
    binned.values[0]['x', 1] = -2
    binned.bins.constituents['data']['x', 2] = -3
    binned.bins.constituents['data'].unit = 's'
    assert sc.identical(buffer, make_variable([-1, -2, -3, 3, 4], unit='s'))


def test_own_var_1d_bin_copy() -> None:
    # Depth of copies of variables can be controlled.
    indices = make_variable(np.array([0, 2, 5]), dtype=sc.DType.int64, unit=None)
    binned = sc.bins(
        data=make_variable(np.arange(5), unit='m'),
        begin=indices['x', :-1],
        end=indices['x', 1:],
        dim='x',
    )
    binned_copy = copy(binned)
    binned_deepcopy = deepcopy(binned)
    binned_methcopy = binned.copy(deep=False)
    binned_methdeepcopy = binned.copy(deep=True)

    binned['x', 0].value['x', 0] = -1
    binned.values[0]['x', 1] = -2
    binned.bins.constituents['data']['x', 2] = -3
    binned.bins.constituents['data'].unit = 's'

    modified = sc.bins(
        data=make_variable([-1, -2, -3, 3, 4], unit='s'),
        begin=indices['x', :-1],
        end=indices['x', 1:],
        dim='x',
    )
    original = sc.bins(
        data=make_variable(np.arange(5), unit='m'),
        begin=indices['x', :-1],
        end=indices['x', 1:],
        dim='x',
    )
    assert sc.identical(binned, modified)
    assert sc.identical(binned_copy, modified)
    assert sc.identical(binned_deepcopy, original)
    assert sc.identical(binned_methcopy, modified)
    assert sc.identical(binned_methdeepcopy, original)


def test_own_var_2d_set() -> None:
    # Input arrays are not shared.
    a = np.arange(10).reshape(2, 5)
    v = make_variable(a)
    v['x', 0] = -np.arange(5)
    v.values[1] = -30
    v['y', 3:]['x', 1] = [-40, -50]
    a[0, 0] = -100
    assert sc.identical(
        v, make_variable([[0, -1, -2, -3, -4], [-30, -30, -30, -40, -50]])
    )
    np.testing.assert_array_equal(a, [[-100, 1, 2, 3, 4], [5, 6, 7, 8, 9]])


def test_own_var_2d_get() -> None:
    # .values getter shares ownership of the array.
    v = make_variable(np.arange(10).reshape(2, 5))
    a = v.values
    v['x', 0] = -np.arange(5)
    v.values[1] = -30
    v['y', 3:]['x', 1] = [-40, -50]
    a[0, 0] = -100
    expected = [[-100, -1, -2, -3, -4], [-30, -30, -30, -40, -50]]
    assert sc.identical(v, make_variable(expected))
    np.testing.assert_array_equal(a, expected)


def test_own_var_2d_copy() -> None:
    # Depth of copies of variables can be controlled.
    v = make_variable(np.arange(6).reshape(2, 3), unit='m')
    v_copy = copy(v)
    v_deepcopy = deepcopy(v)
    v_methcopy = v.copy(deep=False)
    v_methdeepcopy = v.copy(deep=True)

    v['x', 0] = -np.arange(3)
    v.values[1] = -30
    v['y', 1:]['x', 1] = [-40, -50]
    v.unit = 's'
    modified = make_variable(np.reshape([0, -1, -2, -30, -40, -50], (2, 3)), unit='s')
    original = make_variable(np.arange(6).reshape(2, 3), unit='m')
    assert sc.identical(v, modified)
    assert sc.identical(v_copy, modified)
    assert sc.identical(v_deepcopy, original)
    assert sc.identical(v_methcopy, modified)
    assert sc.identical(v_methdeepcopy, original)
