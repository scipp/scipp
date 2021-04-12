# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen
import numpy as np
import scipp as sc


def make_variable(data):
    """
    Make a Variable with default dimensions from data
    while avoiding copies beyond what sc.Variable does.
    """
    if isinstance(data, (list, tuple)):
        data = np.array(data)
    if isinstance(data, np.ndarray):
        dims = ['x', 'y'][:np.ndim(data)]
        return sc.array(dims=dims, values=data)
    return sc.scalar(data)


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
    assert sc.identical(v, make_variable([[0, -1, -2, -3, -4],
                                          [-30, -30, -30, -40, -50]]))
    np.testing.assert_array_equal(a, [[-100, 1, 2, 3, 4],
                                      [5, 6, 7, 8, 9]])


def test_own_var_2d_get():
    # .values getter shares ownership of the array.
    v = make_variable(np.arange(10).reshape(2, 5))
    a = v.values
    v['x', 0] = -np.arange(5)
    v.values[1] = -30
    v['y', 3:]['x', 1] = [-40, -50]
    a[0, 0] = -100
    expected = [[-100, -1, -2, -3, -4],
                [-30, -30, -30, -40, -50]]
    assert sc.identical(v, make_variable(expected))
    np.testing.assert_array_equal(a, expected)


def test_own_var_scalar_fundamental():
    # Python does not allow for any sharing with fundamental types.
    x = 1
    s = sc.scalar(x)
    s.value = 2
    assert s.value == 2
    assert x == 1


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


def test_own_var_scalar_nested_set():
    # Variables are shared when nested.
    a = np.arange(5)
    inner = make_variable(a)
    outer = make_variable(inner)
    outer.value['x', 0] = -1
    outer.value.values[1] = -2
    inner['x', 2] = -3
    a[3] = -4
    assert sc.identical(outer, make_variable(make_variable([-1, -2, -3, 3, 4])))
    assert sc.identical(inner, make_variable([-1, -2, -3, 3, 4]))
    np.testing.assert_array_equal(a, [0, 1, 2, -4, 4])

    # Assigning a new Variable replaces the old without modifying it.
    outer.value = make_variable(np.arange(5, 8))
    outer.value['x', 0] = -5
    assert sc.identical(outer, make_variable(make_variable([-5, 6, 7])))
    assert sc.identical(inner, make_variable([-1, -2, -3, 3, 4]))

    # outer.value = outer
    # assert sc.identical(outer, outer.value)
