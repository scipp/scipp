# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025 Scipp contributors (https://github.com/scipp)
import scipp as sc


def test_new_dim_for_scalar() -> None:
    var = sc.scalar(5, unit='m')
    dim = sc.new_dim_for(var)
    assert dim


def test_new_dim_for_1d_array() -> None:
    var = sc.array(dims=['abc'], values=[1, 2], unit='m')
    dim = sc.new_dim_for(var)
    assert dim
    assert dim != 'abc'


def test_new_dim_for_2d_array() -> None:
    var = sc.array(dims=['abc', 'y'], values=[[1, 2], [3, 4]], unit='m')
    dim = sc.new_dim_for(var)
    assert dim
    assert dim not in ('abc', 'y')


def test_new_dim_for_data_array() -> None:
    var = sc.DataArray(sc.array(dims=['x', 'asd'], values=[[1, 2], [3, 4]], unit='m'))
    dim = sc.new_dim_for(var)
    assert dim
    assert dim not in ('x', 'asd')


def test_new_dim_for_multiple() -> None:
    a = sc.scalar(4, unit='s')
    b = sc.array(dims=['y'], values=[2, 3])
    c = sc.array(dims=['abc'], values=[1, 2, 3])
    d = sc.DataArray(sc.array(dims=['y', 't'], values=[[1, 2], [3, 4]], unit='m'))
    dim = sc.new_dim_for(a, b, c, d)
    assert dim
    assert dim not in ('y', 'abc', 't')
