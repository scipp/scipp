# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen
from copy import copy, deepcopy

import numpy as np
import scipp as sc


def make_variable(data, **kwargs):
    """
    Make a Variable with default dimensions from data
    while avoiding copies beyond what sc.Variable does.
    """
    if isinstance(data, (list, tuple)):
        data = np.array(data)
    if isinstance(data, np.ndarray):
        dims = ['x', 'y'][:np.ndim(data)]
        return sc.array(dims=dims, values=data, **kwargs)
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
    assert sc.identical(v_deepcopy, make_variable(1.0, variance=10.0, unit='m'))
    assert sc.identical(v_methcopy, make_variable(1.0, variance=10.0, unit='m'))


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


def test_own_var_scalar_nested_set():
    # Variables are shared when nested.
    a = np.arange(5)
    inner = make_variable(a)
    outer = make_variable(inner)
    outer.value['x', 0] = -1
    outer.value.values[1] = -2
    inner['x', 2] = -3
    a[3] = -4
    assert sc.identical(outer, make_variable(make_variable([-1, -2, -3, 3,
                                                            4])))
    assert sc.identical(inner, make_variable([-1, -2, -3, 3, 4]))
    np.testing.assert_array_equal(a, [0, 1, 2, -4, 4])

    # Assigning a new Variable replaces the old without modifying it.
    outer.value = make_variable(np.arange(5, 8))
    outer.value['x', 0] = -5
    assert sc.identical(outer, make_variable(make_variable([-5, 6, 7])))
    assert sc.identical(inner, make_variable([-1, -2, -3, 3, 4]))

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

    #
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

