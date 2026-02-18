# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

import numpy as np
import pytest
from hypothesis import given, settings

import scipp as sc
import scipp.testing.strategies as scst

# TODO:
# For now,  we are just checking that creating the repr does not throw.


@given(var=scst.variables(ndim=1))
@settings(max_examples=10, deadline=None)
def test_table_variable(var: sc.Variable) -> None:
    sc.table(var)
    sc.table(var[var.dim, 1:10])


def test_table_variable_strings() -> None:
    sc.table(sc.array(dims=['x'], values=list(map(chr, range(97, 123)))))


def test_table_variable_vector() -> None:
    sc.table(sc.vectors(dims=['x'], values=np.arange(30.0).reshape(10, 3)))


def test_table_variable_linear_transform() -> None:
    col = sc.spatial.linear_transforms(
        dims=['x'], values=np.arange(90.0).reshape(10, 3, 3)
    )
    sc.table(col)


def test_table_variable_datetime() -> None:
    col = sc.epoch(unit='s') + sc.arange('time', 4, unit='s')
    sc.table(col)


@given(da=scst.dataarrays(data_args={'ndim': 1}))
@settings()
def test_table_data_array(da: sc.DataArray) -> None:
    sc.table(da)
    sc.table(da[da.dim, 1:10])


@given(buffer=scst.dataarrays(data_args={'ndim': 1}))
@settings()
def test_table_binned_data_array(buffer: sc.DataArray) -> None:
    buffer.coords['xx'] = sc.arange(buffer.dim, len(buffer))
    binned = buffer.bin(xx=5)
    sc.table(binned)
    sc.table(binned['xx', 1:10])


@given(da=scst.dataarrays(data_args={'ndim': 1}))
@settings()
def test_table_dataset(da: sc.DataArray) -> None:
    ds = sc.Dataset({'a': da, 'b': 3 * da})
    sc.table(ds)
    sc.table(ds[da.dim, 1:10])


@pytest.mark.parametrize('ndim', [0, 2, 4])
def test_table_raises_with_none_1d_variable(ndim: int) -> None:
    var = sc.ones(sizes={f'dim{i}': 4 for i in range(ndim)})
    with pytest.raises(ValueError, match='one-dimensional'):
        sc.table(var)


@given(da=scst.dataarrays(data_args={'ndim': 2}))
@settings()
def test_table_raises_with_2d_dataset(da: sc.DataArray) -> None:
    ds = sc.Dataset({'a': da, 'b': 3 * da})
    with pytest.raises(ValueError, match='one-dimensional'):
        sc.table(ds)
