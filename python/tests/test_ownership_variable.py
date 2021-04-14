# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen
from copy import copy, deepcopy

import numpy as np
import scipp as sc


def make_variable(data, variances=None, **kwargs):
    """
    Make a Variable with default dimensions from data
    while avoiding copies beyond what sc.Variable does.
    """
    if isinstance(data, (list, tuple)):
        data = np.array(data)
    if variances is not None and isinstance(variances, (list, tuple)):
        variances = np.array(variances)
    if isinstance(data, np.ndarray):
        dims = ['x', 'y'][:np.ndim(data)]
        return sc.array(dims=dims, values=data, variances=variances, **kwargs)
    return sc.scalar(data, **kwargs)


def test_own_var_scalar_fundamental():
    # Python does not allow for any sharing with fundamental types.
    x = 1
    s = sc.scalar(x)
    s.value = 2
    assert s.value == 2
    assert x == 1


def test_own_var_scalar_copy():
    # Copies of variables are always deep.
    v = make_variable(1.0, variance=10.0, unit='m')
    v_copy = copy(v)
    v_deepcopy = deepcopy(v)
    v_methcopy = v.copy()

    v.value = 2.0
    v.variance = 20.0
    v.unit = 's'
    assert sc.identical(v, make_variable(2.0, variance=20.0, unit='s'))
    assert sc.identical(v_copy, make_variable(1.0, variance=10.0, unit='m'))
    assert sc.identical(v_deepcopy, make_variable(1.0, variance=10.0,
                                                  unit='m'))
    assert sc.identical(v_methcopy, make_variable(1.0, variance=10.0,
                                                  unit='m'))


def test_own_var_scalar_pyobj_set():
    # Input data is deep-copied.
    x = {'num': 1, 'list': [2, 3]}
    s = make_variable(x)
    x['num'] = -1
    s.value['list'][0] = -2
    assert sc.identical(s, make_variable({'num': 1, 'list': [-2, 3]}))
    assert x == {'num': -1, 'list': [2, 3]}


def test_own_var_scalar_pyobj_get():
    # .value getter shares ownership of the object.
    s = make_variable({'num': 1, 'list': [2, 3]})
    x = s.value
    x['num'] = -1
    s.value['list'][0] = -2
    expected = {'num': -1, 'list': [-2, 3]}
    assert sc.identical(s, make_variable(expected))
    assert x == expected


def test_own_var_scalar_pyobj_copy():
    # Copies of variables are always deep.
    data = {'num': 1, 'list': [2, 3]}
    s = make_variable(data)
    s_copy = copy(s)
    s_deepcopy = deepcopy(s)
    s_methcopy = s.copy()

    s.value['num'] = -1
    s.value['list'][0] = -2
    assert sc.identical(s, make_variable({'num': -1, 'list': [-2, 3]}))
    assert sc.identical(s_copy, make_variable(data))
    assert sc.identical(s_deepcopy, make_variable(data))
    assert sc.identical(s_methcopy, make_variable(data))


def test_own_var_scalar_str_get():
    # Python strings are immutable, no references are shared.
    s = make_variable('abc')
    x = s.value
    s.value = 'def'
    assert sc.identical(s, make_variable('def'))
    assert x == 'abc'


def test_own_var_scalar_str_copy():
    # Copies of variables are always deep.
    s = make_variable('abc')
    s_copy = copy(s)
    s_deepcopy = deepcopy(s)
    s_methcopy = s.copy()

    s.value = 'def'
    assert sc.identical(s, make_variable('def'))
    assert sc.identical(s_copy, make_variable('abc'))
    assert sc.identical(s_deepcopy, make_variable('abc'))
    assert sc.identical(s_methcopy, make_variable('abc'))


def test_own_var_scalar_nested_set():
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
        outer, make_variable(make_variable([-1, -2, -3, 3, 4], unit='s')))
    assert sc.identical(inner, make_variable([-1, -2, -3, 3, 4], unit='s'))
    np.testing.assert_array_equal(a, [0, 1, 2, -4, 4])

    # Assignment creates a new inner Variable.
    outer.value = make_variable(np.arange(5, 8), unit='kg')
    outer.value['x', 0] = -5
    assert sc.identical(outer,
                        make_variable(make_variable([-5, 6, 7], unit='kg')))
    assert sc.identical(inner, make_variable([-1, -2, -3, 3, 4], unit='s'))

    # TODO this causes and infinite loop and segfaults at the moment
    # outer.value = outer
    # assert sc.identical(outer, outer.value)


def test_own_var_scalar_nested_get():
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

    # Assignment writes the values into the inner Variable.
    outer.value = make_variable(np.arange(5, 8))
    outer.value['x', 0] = -5
    inner['x', 1] = -10
    assert sc.identical(outer, make_variable(make_variable([-5, -10, 7])))
    assert sc.identical(inner, make_variable([-5, -10, 7]))
    # TODO `a` seems to be invalidated
    # np.testing.assert_array_equal(a, [-1, -2, -3, -4, 4])


def test_own_var_scalar_nested_copy():
    # Copies of variables never copy nested variables.
    # TODO intentional? Seems to contradict the behaviour with PyObjects.
    outer = make_variable(make_variable(np.arange(5)))
    inner = outer.value
    a = inner.values
    outer_copy = copy(outer)
    outer_deepcopy = deepcopy(outer)
    outer_methcopy = outer.copy()

    outer.value['x', 0] = -1
    outer.value.values[1] = -2
    inner['x', 2] = -3
    a[3] = -4
    expected = [-1, -2, -3, -4, 4]
    assert sc.identical(outer, make_variable(make_variable(expected)))
    assert sc.identical(outer_copy, make_variable(make_variable(expected)))
    assert sc.identical(outer_deepcopy, make_variable(make_variable(expected)))
    assert sc.identical(outer_methcopy, make_variable(make_variable(expected)))


def test_own_var_scalar_nested_str_get():
    # Variables are shared when nested.
    outer = make_variable(make_variable(np.array(['abc', 'def'])))
    inner = outer.value
    a = inner.values
    # Assignment writes the values into the inner Variable.
    outer.value = make_variable(np.array(['ghi', 'jkl', 'mno']))
    outer.value['x', 0] = sc.scalar('asd')
    inner['x', 1] = sc.scalar('qwe')
    assert sc.identical(outer, make_variable(make_variable(np.array(['asd', 'qwe', 'mno']))))
    assert sc.identical(inner, make_variable(np.array(['asd', 'qwe', 'mno'])))
    # TODO `a` seems to be invalidated
    # np.testing.assert_array_equal(a, ['abc', 'def'])


def test_own_var_1d_set():
    # Input arrays are not shared.
    a = np.arange(5)
    v = make_variable(a)
    v['x', 0] = -1
    v.values[1] = -2
    a[2] = -3
    v['x', 3:] = [-4, -5]
    assert sc.identical(v, make_variable([-1, -2, 2, -4, -5]))
    np.testing.assert_array_equal(a, np.array([0, 1, -3, 3, 4]))


def test_own_var_1d_get():
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


def test_own_var_1d_copy():
    # Copies of variables are always deep.
    v = make_variable(np.arange(5.0), unit='m')
    v_copy = copy(v)
    v_deepcopy = deepcopy(v)
    v_methcopy = v.copy()

    v.values = np.arange(5.0, 10)
    v['x', 0] = -1.0
    v.unit = 's'
    assert sc.identical(v, make_variable([-1.0, 6.0, 7.0, 8.0, 9.0], unit='s'))
    assert sc.identical(v_copy,
                        make_variable([0.0, 1.0, 2.0, 3.0, 4.0], unit='m'))
    assert sc.identical(v_deepcopy,
                        make_variable([0.0, 1.0, 2.0, 3.0, 4.0], unit='m'))
    assert sc.identical(v_methcopy,
                        make_variable([0.0, 1.0, 2.0, 3.0, 4.0], unit='m'))

    v.variances = np.arange(10.0, 15.0)
    assert v_copy.variances is None
    assert v_deepcopy.variances is None
    assert v_methcopy.variances is None

    v_copy = copy(v)
    v_deepcopy = deepcopy(v)
    v_methcopy = v.copy()
    v.values[1] = -2.0
    v.variances[1] = -12.0
    v.unit = 'kg'
    assert sc.identical(
        v,
        make_variable([-1.0, -2.0, 7.0, 8.0, 9.0],
                      variances=[10.0, -12.0, 12.0, 13.0, 14.0],
                      unit='kg'))
    assert sc.identical(
        v_copy,
        make_variable([-1.0, 6.0, 7.0, 8.0, 9.0],
                      variances=[10.0, 11.0, 12.0, 13.0, 14.0],
                      unit='s'))
    assert sc.identical(
        v_deepcopy,
        make_variable([-1.0, 6.0, 7.0, 8.0, 9.0],
                      variances=[10.0, 11.0, 12.0, 13.0, 14.0],
                      unit='s'))
    assert sc.identical(
        v_methcopy,
        make_variable([-1.0, 6.0, 7.0, 8.0, 9.0],
                      variances=[10.0, 11.0, 12.0, 13.0, 14.0],
                      unit='s'))


# # TODO needs branch pyobject_arrays
# def test_own_var_1d_pyobj_set():
#     # Input data is deep-copied.
#     x = {'num': 1, 'list': [2, 3]}
#     y = {'num': 4, 'list': [5, 6]}
#     s = sc.concatenate(make_variable(x), make_variable(y), dim='x')
#     s['x', 0].value['num'] = -1
#     s.values[0]['list'][0] = -2
#     y['num'] = -4
#     assert sc.identical(
#         s,
#         sc.concatenate(make_variable({
#             'num': -1,
#             'list': [-2, 3]
#         }),
#                        make_variable({
#                            'num': 4,
#                            'list': [5, 6]
#                        }),
#                        dim='x'))
#     assert x == {'num': 1, 'list': [2, 3]}
#     assert y == {'num': -4, 'list': [5, 6]}


# def test_own_var_scalar_pyobj_get():
#     # .value getter shares ownership of the object.
#     s = make_variable({'num': 1, 'list': [2, 3]})
#     x = s.value
#     x['num'] = -1
#     s.value['list'][0] = -2
#     expected = {'num': -1, 'list': [-2, 3]}
#     assert sc.identical(s, make_variable(expected))
#     assert x == expected
#
#
# def test_own_var_scalar_pyobj_copy():
#     # Copies of variables are always deep.
#     data = {'num': 1, 'list': [2, 3]}
#     s = make_variable(data)
#     s_copy = copy(s)
#     s_deepcopy = deepcopy(s)
#     s_methcopy = s.copy()
#
#     s.value['num'] = -1
#     s.value['list'][0] = -2
#     assert sc.identical(s, make_variable({'num': -1, 'list': [-2, 3]}))
#     assert sc.identical(s_copy, make_variable(data))
#     assert sc.identical(s_deepcopy, make_variable(data))
#     assert sc.identical(s_methcopy, make_variable(data))


def test_own_var_1d_str_set():
    # Input arrays are not shared.
    a = np.array(['abc', 'def'])
    v = make_variable(a)
    v['x', 0] = sc.scalar('xyz')
    assert sc.identical(v, make_variable(np.array(['xyz', 'def'])))
    np.testing.assert_array_equal(a, np.array(['abc', 'def']))


def test_own_var_1d_str_get():
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


def test_own_var_1d_str_copy():
    # Copies of variables are always deep.
    v = make_variable(np.array(['abc', 'def']))
    v_copy = copy(v)
    v_deepcopy = deepcopy(v)
    v_methcopy = v.copy()

    v['x', 0] = sc.scalar('asd')
    v.values[1] = 'qwe'
    assert sc.identical(v, make_variable(np.array(['asd', 'qwe'])))
    assert sc.identical(v_copy, make_variable(np.array(['abc', 'def'])))
    assert sc.identical(v_deepcopy, make_variable(np.array(['abc', 'def'])))
    assert sc.identical(v_methcopy, make_variable(np.array(['abc', 'def'])))


# TODO possible?
# def test_own_var_1d_nested_set():
#     # Variables are shared when nested.
#     a, b = np.arange(4), np.arange(5, 9)
#     inner_a, inner_b = make_variable(a, unit='m'), make_variable(b, unit='m')
#     outer = sc.Variable(dims=['y'], shape=[2], dtype=sc.dtype.Variable)
#     outer.values[0] = inner_a
#     outer.values[1] = inner_b
#     print(outer)
#     print(outer['y', 0])
#     outer['y', 0]['x', 0] = -1
#     outer['y', 0].values[1] = -2
#     outer.values[4] = -6
#     inner_a['x', 2] = -3
#     inner_a.unit = 's'
#     inner_b.values[2] = -8
#     assert sc.identical(
#         outer, sc.concatenate(make_variable([-1, -2, 2, 3], unit='m'),
#                               make_variable([-6, 6, 7, 8], unit='m'), dim='y'))
#     assert sc.identical(inner_a, make_variable([0, 1, -3, 3], unit='s'))
#     assert sc.identical(inner_b, make_variable([5, 6, -8, 8], unit='m'))
#
#     # Assignment creates a new inner Variable.
#     outer.value = make_variable(np.arange(5, 8), unit='kg')
#     outer.value['x', 0] = -5
#     assert sc.identical(outer,
#                         make_variable(make_variable([-5, 6, 7], unit='kg')))
#     assert sc.identical(inner, make_variable([-1, -2, -3, 3, 4], unit='s'))
#
#     # TODO this causes and infinite loop and segfaults at the moment
#     # outer.value = outer
#     # assert sc.identical(outer, outer.value)


# def test_own_var_scalar_nested_get():
#     # Variables are shared when nested.
#     outer = make_variable(make_variable(np.arange(5)))
#     inner = outer.value
#     a = inner.values
#     outer.value['x', 0] = -1
#     outer.value.values[1] = -2
#     inner['x', 2] = -3
#     a[3] = -4
#     expected = [-1, -2, -3, -4, 4]
#     assert sc.identical(outer, make_variable(make_variable(expected)))
#     assert sc.identical(inner, make_variable(expected))
#     np.testing.assert_array_equal(a, expected)
#
#     # Assignment writes the values into the inner Variable.
#     outer.value = make_variable(np.arange(5, 8))
#     outer.value['x', 0] = -5
#     inner['x', 1] = -10
#     assert sc.identical(outer, make_variable(make_variable([-5, -10, 7])))
#     assert sc.identical(inner, make_variable([-5, -10, 7]))
#     # TODO `a` seems to be invalidated
#     # np.testing.assert_array_equal(a, [-1, -2, -3, -4, 4])
#
#
# def test_own_var_scalar_nested_copy():
#     # Copies of variables never copy nested variables.
#     # TODO intentional? Seems to contradict the behaviour with PyObjects.
#     outer = make_variable(make_variable(np.arange(5)))
#     inner = outer.value
#     a = inner.values
#     outer_copy = copy(outer)
#     outer_deepcopy = deepcopy(outer)
#     outer_methcopy = outer.copy()
#
#     outer.value['x', 0] = -1
#     outer.value.values[1] = -2
#     inner['x', 2] = -3
#     a[3] = -4
#     expected = [-1, -2, -3, -4, 4]
#     assert sc.identical(outer, make_variable(make_variable(expected)))
#     assert sc.identical(outer_copy, make_variable(make_variable(expected)))
#     assert sc.identical(outer_deepcopy, make_variable(make_variable(expected)))
#     assert sc.identical(outer_methcopy, make_variable(make_variable(expected)))


def test_own_var_2d_set():
    # Input arrays are not shared.
    a = np.arange(10).reshape(2, 5)
    v = make_variable(a)
    v['x', 0] = -np.arange(5)
    v.values[1] = -30
    v['y', 3:]['x', 1] = [-40, -50]
    a[0, 0] = -100
    assert sc.identical(
        v, make_variable([[0, -1, -2, -3, -4], [-30, -30, -30, -40, -50]]))
    np.testing.assert_array_equal(a, [[-100, 1, 2, 3, 4], [5, 6, 7, 8, 9]])


def test_own_var_2d_get():
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

