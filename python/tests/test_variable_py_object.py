# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import scipp as sc


def test_scalar_Variable_py_object_dict():
    var = sc.Variable(value={'a': 1, 'b': 2})
    assert var.dtype == sc.dtype.PyObject
    assert var.value == {'a': 1, 'b': 2}
    var.value['a'] = 3
    var.value['c'] = 4
    assert var.value == {'a': 3, 'b': 2, 'c': 4}


def test_scalar_Variable_py_object_list():
    var = sc.Variable(value=[1, 2, 3])
    assert var.dtype == sc.dtype.PyObject
    assert var.value == [1, 2, 3]
    var.value[0] = 2
    assert var.value == [2, 2, 3]


def test_scalar_Variable_py_object_change():
    var = sc.Variable(value=[1, 2, 3])
    assert var.dtype == sc.dtype.PyObject
    assert var.value == [1, 2, 3]
    # Value assignment cannot change dtype, so the result is still PyObject,
    # contrary to creating a variable directly from an integer.
    var.value = 1
    assert var.dtype == sc.dtype.PyObject
    assert var.value == 1


def test_scalar_Variable_py_object_copy_is_deep_copy():
    var = sc.Variable(value=[1, 2, 3])
    # Dataset.copy() releases the GIL. This will segfault unless
    # scipp::python::PyObject::PyObject acquires the GIL.
    copy = var.copy()
    copy.value[0] = 666
    assert copy.value == [666, 2, 3]
    assert var.value == [1, 2, 3]


def test_scalar_Variable_py_object_comparison():
    a = sc.Variable(value=[1, 2])
    b = sc.Variable(value=[1, 2])
    assert a == b
    assert not (a != b)


def test_py_object_delitem():
    d = sc.Dataset()
    d['a'] = sc.Variable(value=[1, 2])
    # Dataset.__delitem__ releases the GIL. This will segfault unless
    # scipp::python::PyObject::~PyObject acquires the GIL.
    del d['a']
