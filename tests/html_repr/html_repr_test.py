# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

import numpy as np
import scipp as sc
import pytest
from ..factory import make_dense_data_array, make_dense_dataset, \
    make_binned_data_array, make_scalar, make_variable, make_scalar_array

# TODO:
# For now,  we are just checking that creating the repr does not throw.


def maybe_variances(variances, dtype):
    if dtype in [sc.DType.float64, sc.DType.float32]:
        return variances
    else:
        return False


@pytest.mark.parametrize("variance", [False, True])
@pytest.mark.parametrize(
    "dtype", [sc.DType.float64, sc.DType.float32, sc.DType.int64, sc.DType.int32])
@pytest.mark.parametrize("unit", ['dimensionless', 'counts', 's', 'us'])
def test_html_repr_scalar(variance, dtype, unit):
    var = make_scalar(with_variance=maybe_variances(variance, dtype),
                      dtype=dtype,
                      unit=unit)
    sc.make_html(var)


@pytest.mark.parametrize("variance", [False, True])
@pytest.mark.parametrize("label", [False, True])
@pytest.mark.parametrize("attr", [False, True])
@pytest.mark.parametrize("mask", [False, True])
@pytest.mark.parametrize(
    "dtype", [sc.DType.float64, sc.DType.float32, sc.DType.int64, sc.DType.int32])
@pytest.mark.parametrize("unit", ['dimensionless', 'counts', 's'])
def test_html_repr_scalar_array(variance, label, attr, mask, dtype, unit):
    da = make_scalar_array(with_variance=maybe_variances(variance, dtype),
                           label=label,
                           attr=attr,
                           mask=mask,
                           dtype=dtype,
                           unit=unit)
    sc.make_html(da)


@pytest.mark.parametrize("ndim", [1, 2, 3, 4])
@pytest.mark.parametrize("variances", [False, True])
@pytest.mark.parametrize("dtype", [sc.DType.float64, sc.DType.int64])
@pytest.mark.parametrize("unit", ['dimensionless', 'counts', 's'])
def test_html_repr_variable(ndim, variances, dtype, unit):
    var = make_variable(ndim=ndim,
                        with_variance=maybe_variances(variances, dtype),
                        dtype=dtype,
                        unit=unit)
    sc.make_html(var)
    sc.make_html(var['xx', 1:10])


def test_html_repr_variable_strings():
    sc.make_html(sc.array(dims=['x'], values=list(map(chr, range(97, 123)))))


def test_html_repr_variable_vector():
    sc.make_html(sc.vectors(dims=['x'], values=np.arange(30.).reshape(10, 3)))


@pytest.mark.parametrize("ndim", [1, 2, 3, 4])
@pytest.mark.parametrize("with_all", [True, False])
@pytest.mark.parametrize("dtype", [sc.DType.float64, sc.DType.int64])
@pytest.mark.parametrize("unit", ['dimensionless', 'counts', 's'])
def test_html_repr_data_array(ndim, with_all, dtype, unit):
    da = make_dense_data_array(ndim=ndim,
                               with_variance=maybe_variances(with_all, dtype),
                               binedges=with_all,
                               labels=with_all,
                               attrs=with_all,
                               masks=with_all,
                               ragged=with_all,
                               dtype=dtype,
                               unit=unit)
    sc.make_html(da)
    sc.make_html(da['xx', 1:10])


@pytest.mark.parametrize("ndim", [1, 2, 3, 4])
@pytest.mark.parametrize("variances", [False, True])
@pytest.mark.parametrize("masks", [False, True])
def test_html_repr_binned_data_array(ndim, variances, masks):
    da = make_binned_data_array(ndim=ndim, with_variance=variances, masks=masks)
    sc.make_html(da)
    sc.make_html(da['xx', 1:10])


@pytest.mark.parametrize("ndim", [1, 2, 3, 4])
@pytest.mark.parametrize("with_all", [True, False])
@pytest.mark.parametrize("dtype", [sc.DType.float64, sc.DType.int64])
@pytest.mark.parametrize("unit", ['dimensionless', 'counts', 's'])
def test_html_repr_dataset(ndim, with_all, dtype, unit):
    da = make_dense_dataset(ndim=ndim,
                            with_variance=maybe_variances(with_all, dtype),
                            binedges=with_all,
                            labels=with_all,
                            attrs=with_all,
                            masks=with_all,
                            ragged=with_all,
                            dtype=dtype,
                            unit=unit)
    sc.make_html(da)
    sc.make_html(da['xx', 1:10])
