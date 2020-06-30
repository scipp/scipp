# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

import numpy as np
import pytest
import scipp as sc
from plot_helper import make_dense_dataset, make_events_dataset
from scipp.plot import plot

# Prevent figure from being displayed when running the tests
import matplotlib.pyplot as plt
plt.ioff()

# TODO: For now we are just checking that the plot does not throw any errors.
# In the future it would be nice to check the output by either comparing
# checksums or by using tools like squish.


def test_plot_sliceviewer():
    d = make_dense_dataset(ndim=3)
    plot(d)


def test_plot_sliceviewer_with_two_sliders():
    d = make_dense_dataset(ndim=4)
    plot(d)


def test_plot_sliceviewer_with_axes():
    d = make_dense_dataset(ndim=3)
    plot(d, axes=['x', 'tof', 'y'])


def test_plot_sliceviewer_with_labels():
    d = make_dense_dataset(ndim=3, labels=True)
    plot(d, axes=['x', 'y', "somelabels"])


def test_plot_sliceviewer_with_3d_projection():
    d = make_dense_dataset(ndim=3)
    plot(d, projection="3d")


@pytest.mark.skip(reason="3D plotting with labels is currently broken after"
                  "dims API refactor.")
def test_plot_sliceviewer_with_3d_projection_with_labels():
    d = make_dense_dataset(ndim=3, labels=True)
    plot(d, projection="3d", axes=['x', 'y', "somelabels"])


def test_plot_3d_with_filename():
    d = make_dense_dataset(ndim=3)
    plot(d, projection="3d", filename="a3dplot.html")


def test_plot_convenience_methods():
    d = make_dense_dataset(ndim=3)
    sc.plot.image(d)
    sc.plot.threeslice(d)
    sc.plot.superplot(d)


@pytest.mark.skip(reason="RuntimeError: Only the simple case histograms may "
                  "be constructed for now: 2 dims including events.")
def test_plot_3d_events_data_with_int_bins():
    d = make_events_dataset(ndim=3)
    plot(d, bins=50)


@pytest.mark.skip(reason="RuntimeError: Only the simple case histograms may "
                  "be constructed for now: 2 dims including events.")
def test_plot_3d_events_data_with_nparray_bins():
    d = make_events_dataset(ndim=3)
    plot(d, bins=np.linspace(0.0, 105.0, 50))


@pytest.mark.skip(reason="RuntimeError: Only the simple case histograms may "
                  "be constructed for now: 2 dims including events.")
def test_plot_3d_events_data_with_Variable_bins():
    d = make_events_dataset(ndim=3)
    bins = sc.Variable(['tof'],
                       values=np.linspace(0.0, 105.0, 50),
                       unit=sc.units.us)
    plot(d, bins=bins)


def test_plot_variable_3d():
    N = 50
    v3d = sc.Variable(['tof', 'x', 'y'],
                      values=np.random.rand(N, N, N),
                      unit=sc.units.m)
    plot(v3d)


def test_plot_realigned_3d():
    d = make_events_dataset(ndim=2)
    tbins = sc.Variable(dims=['tof'], unit=sc.units.us, values=np.arange(100.))
    r = sc.realign(d, {'tof': tbins})
    plot(r)


def test_plot_4d_with_masks():
    data = sc.DataArray(data=sc.Variable(
        dims=['pack', 'tube', 'straw', 'pixel'],
        values=np.random.rand(2, 8, 7, 256)),
                        coords={})
    data += sc.Variable(dims=['pixel'],
                        values=np.sin(np.linspace(0, 3.14, num=256)))
    data.masks['tube_ends'] = sc.Variable(dims=['pixel'],
                                          values=np.full(256, False))
    plot(data)
