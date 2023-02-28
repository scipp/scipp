# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

import numpy as np
import pytest

import scipp as sc

from ..factory import make_dense_data_array, make_dense_dataset

# TODO: For now we are just checking that the plot does not throw any errors.
# In the future it would be nice to check the output by either comparing
# checksums or by using tools like squish.


def test_plot_sliceviewer_2d():
    sc.plot(make_dense_data_array(ndim=3))


def test_plot_sliceviewer_2d_dataset():
    sc.plot(make_dense_dataset(ndim=3))


def test_plot_sliceviewer_2d_with_two_sliders():
    sc.plot(make_dense_data_array(ndim=4))


def test_plot_sliceviewer_2d_transposed_axes():
    sc.plot(sc.transpose(make_dense_data_array(ndim=3), dims=['xx', 'zz', 'yy']))
    sc.plot(sc.transpose(make_dense_data_array(ndim=3), dims=['yy', 'xx', 'zz']))


def test_plot_sliceviewer_2d_with_labels():
    sc.plot(make_dense_data_array(ndim=3, labels=True), labels={'xx': 'lab'})


def test_plot_sliceviewer_2d_with_attrs():
    sc.plot(make_dense_data_array(ndim=3, attrs=True), labels={'xx': 'attr'})


def test_plot_sliceviewer_2d_with_binedges():
    sc.plot(make_dense_data_array(ndim=3, binedges=True))


def test_plot_variable_3d():
    N = 50
    v3d = sc.Variable(
        dims=['z', 'y', 'x'], values=np.random.rand(N, N, N), unit=sc.units.m
    )
    sc.plot(v3d)


def test_plot_4d_with_masks_no_coords():
    da = sc.DataArray(
        data=sc.Variable(
            dims=['pack', 'tube', 'straw', 'pixel'], values=np.random.rand(2, 8, 7, 256)
        ),
        coords={},
    )
    a = np.sin(np.linspace(0, 3.14, num=256))
    da += sc.Variable(dims=['pixel'], values=a)
    da.masks['tube_ends'] = sc.Variable(
        dims=['pixel'], values=np.where(a > 0.5, True, False)
    )
    sc.plot(da)
    sc.plot(sc.transpose(da, dims=['pack', 'tube', 'straw', 'pixel']))
    sc.plot(sc.transpose(da, dims=['pack', 'straw', 'tube', 'pixel']))
    sc.plot(sc.transpose(da, dims=['pack', 'tube', 'pixel', 'straw']))
    sc.plot(sc.transpose(da, dims=['straw', 'pixel', 'pack', 'tube']))


def test_plot_3d_data_ragged():
    """
    This test has caught MANY bugs and should not be disabled.
    """
    da = make_dense_data_array(ndim=3, ragged=True)
    sc.plot(da)
    # Also check that it raises an error if we try to have ragged coord along
    # slider dim
    with pytest.raises(RuntimeError) as e:
        sc.plot(sc.transpose(da))
    assert str(e.value) == (
        'A ragged coordinate cannot lie along '
        'a slider dimension, it must be one of '
        'the displayed dimensions.'
    )


def test_plot_3d_data_ragged_with_edges():
    sc.plot(make_dense_data_array(ndim=3, ragged=True, binedges=True))


def test_plot_3d_data_ragged_with_masks():
    sc.plot(make_dense_data_array(ndim=3, ragged=True, masks=True))
