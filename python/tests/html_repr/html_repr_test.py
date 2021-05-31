# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

import numpy as np
import scipp as sc
from ..factory import make_dense_data_array, make_dense_dataset, \
                      make_binned_data_array

# TODO:
# For now,  we are just checking that creating the repr does not throw.


def test_html_repr_variable_1d():
    sc.make_html(sc.arange('x', 50.))


def test_html_repr_variable_1d_unit():
    sc.make_html(sc.arange('x', 50., unit='m'))


def test_html_repr_variable_1d_slice():
    sc.make_html(sc.arange('x', 50., unit='m')['x', 1:10])


def test_html_repr_variable_1d_variances():
    N = 20
    sc.make_html(
        sc.array(dims=['x'],
                 values=np.random.random(20),
                 variances=np.random.random(20) * 0.1,
                 unit='m'))


def test_html_repr_variable_2d():
    sc.make_html(
        sc.array(dims=['y', 'x'], values=np.random.random([10, 20]), unit='s'))


def test_html_repr_data_array():
    sc.make_html(make_dense_data_array(ndim=1))


def test_html_repr_data_array_slice():
    sc.make_html(make_dense_data_array(ndim=1)['tof', 1:10])


def test_html_repr_data_array_bin_edges():
    sc.make_html(make_dense_data_array(ndim=1, binedges=True))


def test_html_repr_data_array_labels():
    sc.make_html(make_dense_data_array(ndim=1, labels=True))


def test_html_repr_data_array_attrs():
    sc.make_html(make_dense_data_array(ndim=1, attrs=True))


def test_html_repr_data_array_masks():
    sc.make_html(make_dense_data_array(ndim=1, masks=True))


def test_html_repr_data_array_2d_simple():
    sc.make_html(make_dense_data_array(ndim=2))


def test_html_repr_data_array_2d_bin_edges():
    sc.make_html(make_dense_data_array(ndim=2, binedges=True))


def test_html_repr_data_array_2d_complex():
    sc.make_html(
        make_dense_data_array(ndim=2,
                              variances=True,
                              binedges=True,
                              labels=True,
                              attrs=True,
                              masks=True))


def test_html_repr_data_array_2d_ragged():
    sc.make_html(make_dense_data_array(ndim=2, ragged=True))


def test_html_repr_data_array_2d_ragged_binedges():
    sc.make_html(make_dense_data_array(ndim=2, ragged=True, binedges=True))


def test_html_repr_data_array_4d_simple():
    sc.make_html(make_dense_data_array(ndim=4))


def test_html_repr_dataset_1d():
    sc.make_html(make_dense_dataset(ndim=1))


def test_html_repr_dataset_2d():
    sc.make_html(make_dense_dataset(ndim=2))


def test_html_repr_dataset_2d_complex():
    sc.make_html(
        make_dense_dataset(ndim=2,
                           binedges=True,
                           labels=True,
                           attrs=True,
                           masks=True))


def test_html_repr_1d_binned_data_simple():
    sc.make_html(make_binned_data_array(ndim=1))


def test_html_repr_1d_binned_data_slice():
    sc.make_html(make_binned_data_array(ndim=1)['tof', 0])


def test_html_repr_1d_binned_data_complex():
    sc.make_html(make_binned_data_array(ndim=1, variances=True, masks=True))


def test_html_repr_2d_binned_data_simple():
    sc.make_html(make_binned_data_array(ndim=2))


def test_html_repr_2d_binned_data_slice():
    sc.make_html(make_binned_data_array(ndim=2)['x', 0])


def test_html_repr_2d_binned_data_complex():
    sc.make_html(make_binned_data_array(ndim=2, variances=True, masks=True))