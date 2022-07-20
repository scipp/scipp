# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

import numpy as np
import scipp as sc
import pytest
from ..factory import make_dense_data_array, make_dense_dataset, \
    make_binned_data_array, make_variable

# TODO:
# For now,  we are just checking that creating the repr does not throw.


def maybe_variances(variances, dtype):
    if dtype in [sc.DType.float64, sc.DType.float32]:
        return variances
    else:
        return False


@pytest.mark.parametrize("variances", [False, True])
@pytest.mark.parametrize("dtype", [sc.DType.float64, sc.DType.int64])
@pytest.mark.parametrize("unit", ['dimensionless', 'counts', 's'])
def test_table_variable(variances, dtype, unit):
    var = make_variable(ndim=1,
                        with_variance=maybe_variances(variances, dtype),
                        dtype=dtype,
                        unit=unit)
    sc.table(var)
    sc.table(var['xx', 1:10])


def test_column_with_zero_variance():
    col = sc.zeros(dims=['row'], shape=(4, ), with_variances=True)
    sc.table(col)


def test_table_variable_strings():
    sc.table(sc.array(dims=['x'], values=list(map(chr, range(97, 123)))))


def test_table_variable_vector():
    sc.table(sc.vectors(dims=['x'], values=np.arange(30.).reshape(10, 3)))


def test_table_variable_linear_transform():
    col = sc.spatial.linear_transforms(dims=['x'],
                                       values=np.arange(90.).reshape(10, 3, 3))
    sc.table(col)


def test_table_variable_datetime():
    col = sc.epoch(unit='s') + sc.arange('time', 4, unit='s')
    sc.table(col)


@pytest.mark.parametrize("with_all", [True, False])
@pytest.mark.parametrize("dtype", [sc.DType.float64, sc.DType.int64])
@pytest.mark.parametrize("unit", ['dimensionless', 'counts', 's'])
def test_table_data_array(with_all, dtype, unit):
    da = make_dense_data_array(ndim=1,
                               with_variance=maybe_variances(with_all, dtype),
                               binedges=with_all,
                               labels=with_all,
                               attrs=with_all,
                               masks=with_all,
                               dtype=dtype,
                               unit=unit)
    sc.table(da)
    sc.table(da['xx', 1:10])


@pytest.mark.parametrize("variances", [False, True])
@pytest.mark.parametrize("masks", [False, True])
def test_table_binned_data_array(variances, masks):
    da = make_binned_data_array(ndim=1, with_variance=variances, masks=masks)
    sc.table(da)
    sc.table(da['xx', 1:10])


@pytest.mark.parametrize("with_all", [True, False])
@pytest.mark.parametrize("dtype", [sc.DType.float64, sc.DType.int64])
@pytest.mark.parametrize("unit", ['dimensionless', 'counts', 's'])
def test_table_dataset(with_all, dtype, unit):
    ds = make_dense_dataset(ndim=1,
                            with_variance=maybe_variances(with_all, dtype),
                            binedges=with_all,
                            labels=with_all,
                            attrs=with_all,
                            masks=with_all,
                            dtype=dtype,
                            unit=unit)
    sc.table(ds)
    sc.table(ds['xx', 1:10])


def test_table_raises_with_2d_dataset():
    ds = make_dense_dataset(ndim=2)
    with pytest.raises(ValueError):
        sc.table(ds)


def test_table_dataset_with_0d_elements():
    ds = make_dense_dataset(ndim=1, attrs=True, masks=True)
    ds.coords['scalar_coord'] = sc.scalar(44.)
    ds['scalar_data'] = sc.scalar(111.)
    ds['a'].masks['0d_mask'] = sc.scalar(True)
    sc.table(ds)


def test_table_dataset_with_0d_bin_edge_attributes():
    ds = make_dense_dataset(ndim=2, attrs=True, masks=True, binedges=True)
    sc.table(ds['yy', 0])
