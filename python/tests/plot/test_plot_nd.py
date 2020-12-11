# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

import numpy as np
import pytest
import scipp as sc
from plot_helper import make_dense_dataset
from scipp.plot import plot

# TODO: For now we are just checking that the plot does not throw any errors.
# In the future it would be nice to check the output by either comparing
# checksums or by using tools like squish.


def test_plot_sliceviewer_2d():
    plot(make_dense_dataset(ndim=3))


def test_plot_sliceviewer_2d_with_two_sliders():
    plot(make_dense_dataset(ndim=4))


def test_plot_sliceviewer_2d_with_axes():
    plot(make_dense_dataset(ndim=3), axes={'y': 'tof'})


def test_plot_sliceviewer_2d_with_axes_redundant():
    plot(make_dense_dataset(ndim=3), axes={'y': 'tof', 'x': 'x'})


def test_plot_sliceviewer_2d_with_two_axes():
    plot(make_dense_dataset(ndim=3), axes={'x': 'y', 'y': 'tof'})


def test_plot_sliceviewer_2d_with_labels():
    plot(make_dense_dataset(ndim=3, labels=True), axes={'x': "somelabels"})


def test_plot_sliceviewer_2d_with_attrs():
    plot(make_dense_dataset(ndim=3, attrs=True), axes={'x': "attr"})


def test_plot_sliceviewer_2d_with_binedges():
    plot(make_dense_dataset(ndim=3, binedges=True))


def test_plot_convenience_methods():
    d = make_dense_dataset(ndim=3)
    sc.plot.superplot(d)
    sc.plot.image(d)
    sc.plot.scatter3d(d)


def test_plot_variable_3d():
    N = 50
    v3d = sc.Variable(['tof', 'x', 'y'],
                      values=np.random.rand(N, N, N),
                      unit=sc.units.m)
    plot(v3d)


def test_plot_4d_with_masks_no_coords():
    data = sc.DataArray(data=sc.Variable(
        dims=['pack', 'tube', 'straw', 'pixel'],
        values=np.random.rand(2, 8, 7, 256)),
                        coords={})
    a = np.sin(np.linspace(0, 3.14, num=256))
    data += sc.Variable(dims=['pixel'], values=a)
    data.masks['tube_ends'] = sc.Variable(dims=['pixel'],
                                          values=np.where(
                                              a > 0.5, True, False))
    plot(data)
    plot(data, axes={'y': 'tube'})


def test_plot_3d_data_ragged():
    """
    This test has caught MANY bugs and should not be disabled.
    """
    d = make_dense_dataset(ndim=3, ragged=True)
    plot(d)
    # Also check that it raises an error if we try to have ragged coord along
    # slider dim
    with pytest.raises(RuntimeError) as e:
        plot(d, axes={'x': 'tof', 'y': 'x'})
    assert str(e.value) == ("A ragged coordinate cannot lie along "
                            "a slider dimension, it must be one of "
                            "the displayed dimensions.")


def test_plot_3d_data_ragged_with_edges():
    plot(make_dense_dataset(ndim=3, ragged=True, binedges=True))


def test_plot_3d_data_ragged_with_masks():
    plot(make_dense_dataset(ndim=3, ragged=True, masks=True))
