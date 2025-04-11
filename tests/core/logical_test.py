# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen
import numpy as np

import scipp as sc


def test_logical_not_variable() -> None:
    assert sc.identical(~sc.scalar(False), sc.scalar(True))
    assert sc.identical(~sc.scalar(True), sc.scalar(False))


def test_logical_ior_variable_with_variable() -> None:
    a = sc.scalar(False)
    b = sc.scalar(True)
    a |= b
    assert sc.identical(a, sc.scalar(True))

    a = sc.Variable(dims=['x'], values=np.array([False, True, False, True]))
    b = sc.Variable(dims=['x'], values=np.array([False, False, True, True]))
    a |= b
    assert sc.identical(
        a, sc.Variable(dims=['x'], values=np.array([False, True, True, True]))
    )


def test_logical_or_variable_with_variable() -> None:
    a = sc.scalar(False)
    b = sc.scalar(True)
    assert sc.identical((a | b), sc.scalar(True))

    a = sc.Variable(dims=['x'], values=np.array([False, True, False, True]))
    b = sc.Variable(dims=['x'], values=np.array([False, False, True, True]))
    assert sc.identical(
        (a | b), sc.Variable(dims=['x'], values=np.array([False, True, True, True]))
    )


def test_logical_iand_variable_with_variable() -> None:
    a = sc.scalar(False)
    b = sc.scalar(True)
    a &= b
    assert sc.identical(a, sc.scalar(False))

    a = sc.Variable(dims=['x'], values=np.array([False, True, False, True]))
    b = sc.Variable(dims=['x'], values=np.array([False, False, True, True]))
    a &= b
    assert sc.identical(
        a, sc.Variable(dims=['x'], values=np.array([False, False, False, True]))
    )


def test_logical_and_variable_with_variable() -> None:
    a = sc.scalar(False)
    b = sc.scalar(True)
    assert sc.identical((a & b), sc.scalar(False))

    a = sc.Variable(dims=['x'], values=np.array([False, True, False, True]))
    b = sc.Variable(dims=['x'], values=np.array([False, False, True, True]))
    assert sc.identical(
        (a & b), sc.Variable(dims=['x'], values=np.array([False, False, False, True]))
    )


def test_logical_ixor_variable_with_variable() -> None:
    a = sc.scalar(False)
    b = sc.scalar(True)
    a ^= b
    assert sc.identical(a, sc.scalar(True))

    a = sc.Variable(dims=['x'], values=np.array([False, True, False, True]))
    b = sc.Variable(dims=['x'], values=np.array([False, False, True, True]))
    a ^= b
    assert sc.identical(
        a, sc.Variable(dims=['x'], values=np.array([False, True, True, False]))
    )


def test_logical_xor_variable_with_variable() -> None:
    a = sc.scalar(False)
    b = sc.scalar(True)
    assert sc.identical((a ^ b), sc.scalar(True))

    a = sc.Variable(dims=['x'], values=np.array([False, True, False, True]))
    b = sc.Variable(dims=['x'], values=np.array([False, False, True, True]))
    assert sc.identical(
        (a ^ b), sc.Variable(dims=['x'], values=np.array([False, True, True, False]))
    )


def test_logical_not_function() -> None:
    assert sc.identical(sc.logical_not(sc.scalar(False)), sc.scalar(True))
    assert sc.identical(sc.logical_not(sc.scalar(True)), sc.scalar(False))


def test_logical_and_function() -> None:
    assert sc.identical(
        sc.logical_and(sc.scalar(True), sc.scalar(False)), sc.scalar(False)
    )


def test_logical_or_function() -> None:
    assert sc.identical(
        sc.logical_or(sc.scalar(True), sc.scalar(False)), sc.scalar(True)
    )


def test_logical_xor_function() -> None:
    assert sc.identical(
        sc.logical_xor(sc.scalar(True), sc.scalar(False)), sc.scalar(True)
    )


def test_logical_not_data_array() -> None:
    assert sc.identical(~sc.DataArray(sc.scalar(False)), sc.DataArray(sc.scalar(True)))
    assert sc.identical(~sc.DataArray(sc.scalar(True)), sc.DataArray(sc.scalar(False)))
