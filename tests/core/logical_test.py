# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen
import numpy as np

import scipp as sc


def test_bitwise_not_variable():
    assert sc.identical(~sc.scalar(False), sc.scalar(True))
    assert sc.identical(~sc.scalar(True), sc.scalar(False))


def test_bitwise_ior_variable_with_variable():
    a = sc.scalar(False)
    b = sc.scalar(True)
    a |= b
    assert sc.identical(a, sc.scalar(True))

    a = sc.Variable(dims=['x'], values=np.array([False, True, False, True]))
    b = sc.Variable(dims=['x'], values=np.array([False, False, True, True]))
    a |= b
    assert sc.identical(
        a, sc.Variable(dims=['x'], values=np.array([False, True, True, True])))


def test_bitwise_or_variable_with_variable():
    a = sc.scalar(False)
    b = sc.scalar(True)
    assert sc.identical((a | b), sc.scalar(True))

    a = sc.Variable(dims=['x'], values=np.array([False, True, False, True]))
    b = sc.Variable(dims=['x'], values=np.array([False, False, True, True]))
    assert sc.identical((a | b),
                        sc.Variable(dims=['x'],
                                    values=np.array([False, True, True, True])))


def test_bitwise_iand_variable_with_variable():
    a = sc.scalar(False)
    b = sc.scalar(True)
    a &= b
    assert sc.identical(a, sc.scalar(False))

    a = sc.Variable(dims=['x'], values=np.array([False, True, False, True]))
    b = sc.Variable(dims=['x'], values=np.array([False, False, True, True]))
    a &= b
    assert sc.identical(
        a, sc.Variable(dims=['x'], values=np.array([False, False, False, True])))


def test_bitwise_and_variable_with_variable():
    a = sc.scalar(False)
    b = sc.scalar(True)
    assert sc.identical((a & b), sc.scalar(False))

    a = sc.Variable(dims=['x'], values=np.array([False, True, False, True]))
    b = sc.Variable(dims=['x'], values=np.array([False, False, True, True]))
    assert sc.identical((a & b),
                        sc.Variable(dims=['x'],
                                    values=np.array([False, False, False, True])))


def test_bitwise_ixor_variable_with_variable():
    a = sc.scalar(False)
    b = sc.scalar(True)
    a ^= b
    assert sc.identical(a, sc.scalar(True))

    a = sc.Variable(dims=['x'], values=np.array([False, True, False, True]))
    b = sc.Variable(dims=['x'], values=np.array([False, False, True, True]))
    a ^= b
    assert sc.identical(
        a, sc.Variable(dims=['x'], values=np.array([False, True, True, False])))


def test_bitwise_xor_variable_with_variable():
    a = sc.scalar(False)
    b = sc.scalar(True)
    assert sc.identical((a ^ b), sc.scalar(True))

    a = sc.Variable(dims=['x'], values=np.array([False, True, False, True]))
    b = sc.Variable(dims=['x'], values=np.array([False, False, True, True]))
    assert sc.identical((a ^ b),
                        sc.Variable(dims=['x'],
                                    values=np.array([False, True, True, False])))
