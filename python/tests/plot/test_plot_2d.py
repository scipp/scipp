# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

import numpy as np
import scipp as sc
from plot_helper import make_dense_dataset, make_events_dataset
from scipp.plot import plot

# Prevent figure from being displayed when running the tests
import matplotlib.pyplot as plt
plt.ioff()


def test_plot_2d_image():
    d = make_dense_dataset(ndim=2)
    plot(d)


def test_plot_2d_image_with_log():
    d = make_dense_dataset(ndim=2)
    plot(d, log=True)


def test_plot_2d_image_with_vmin_vmax():
    d = make_dense_dataset(ndim=2)
    plot(d, vmin=0.1, vmax=0.9)


def test_plot_2d_image_with_vmin_vmax_with_log():
    d = make_dense_dataset(ndim=2)
    plot(d, vmin=0.1, vmax=0.9, log=True)


def test_plot_2d_image_with_with_nan():
    d = make_dense_dataset(ndim=2)
    d["Sample"].values[0, 0] = np.nan
    plot(d)


def test_plot_2d_image_with_with_nan_with_log():
    d = make_dense_dataset(ndim=2)
    d["Sample"].values[0, 0] = np.nan
    plot(d, log=True)


def test_plot_2d_image_with_cmap():
    d = make_dense_dataset(ndim=2)
    plot(d, cmap="jet")


def test_plot_2d_image_with_axes():
    d = make_dense_dataset(ndim=2)
    plot(d, axes=['tof', 'x'])


def test_plot_2d_image_with_labels():
    d = make_dense_dataset(ndim=2, labels=True)
    plot(d, axes=['x', "somelabels"])


def test_plot_2d_image_with_filename():
    d = make_dense_dataset(ndim=2)
    plot(d, filename="image.pdf")


def test_plot_2d_image_with_bin_edges():
    d = make_dense_dataset(ndim=2, binedges=True)
    plot(d)


def test_plot_2d_with_masks():
    d = make_dense_dataset(ndim=2, masks=True)
    plot(d)


def test_plot_2d_with_masks_and_labels():
    d = make_dense_dataset(ndim=2, masks=True, labels=True)
    plot(d, axes=['x', "somelabels"])


def test_plot_2d_image_with_non_regular_bin_edges():
    d = make_dense_dataset(ndim=2, binedges=True)
    d.coords['tof'].values = d.coords['tof'].values**2
    plot(d)


def test_plot_2d_image_with_non_regular_bin_edges_resolution():
    d = make_dense_dataset(ndim=2, binedges=True)
    d.coords['tof'].values = d.coords['tof'].values**2
    plot(d, resolution=128)


def test_plot_2d_image_with_non_regular_bin_edges_with_masks():
    d = make_dense_dataset(ndim=2, masks=True, binedges=True)
    d.coords['tof'].values = d.coords['tof'].values**2
    plot(d)


def test_plot_2d_events_data_with_int_bins():
    d = make_events_dataset(ndim=1)
    plot(d, bins={'tof': 50})


def test_plot_2d_events_data_with_nparray_bins():
    d = make_events_dataset(ndim=1)
    plot(d, bins={'tof': np.linspace(0.0, 105.0, 50)})


def test_plot_2d_events_data_with_Variable_bins():
    d = make_events_dataset(ndim=1)
    bins = sc.Variable(['tof'],
                       values=np.linspace(0.0, 105.0, 50),
                       unit=sc.units.us)
    plot(d, bins={'tof': bins})


def test_plot_2d_events_data_with_nparray_bins_and_extra_dim():
    d = make_events_dataset(ndim=2)
    plot(d, bins={'tof': np.linspace(0.0, 105.0, 50)})


def test_plot_variable_2d():
    N = 50
    v2d = sc.Variable(['tof', 'x'],
                      values=np.random.rand(N, N),
                      unit=sc.units.K)
    plot(v2d)


def test_plot_string_and_vector_axis_labels_2d():
    N = 10
    M = 5
    vecs = []
    for i in range(N):
        vecs.append(np.random.random(3))
    d = sc.Dataset()
    d.coords['x'] = sc.Variable(['x'],
                                values=vecs,
                                unit=sc.units.m,
                                dtype=sc.dtype.vector_3_float64)
    d.coords['y'] = sc.Variable(['y'],
                                values=["a", "b", "c", "d", "e"],
                                unit=sc.units.m)
    d["Signal"] = sc.Variable(['y', 'x'],
                              values=np.random.random([M, N]),
                              unit=sc.units.counts)
    plot(d)


def test_plot_2d_with_dimension_of_size_1():
    N = 10
    M = 1
    x = np.arange(N, dtype=np.float64)
    y = np.arange(M, dtype=np.float64)
    z = np.arange(M + 1, dtype=np.float64)
    d = sc.Dataset()
    d.coords['x'] = sc.Variable(['x'], values=x, unit=sc.units.m)
    d.coords['y'] = sc.Variable(['y'], values=y, unit=sc.units.m)
    d.coords['z'] = sc.Variable(['z'], values=z, unit=sc.units.m)
    d["a"] = sc.Variable(['y', 'x'],
                         values=np.random.random([M, N]),
                         unit=sc.units.counts)
    d["b"] = sc.Variable(['z', 'x'],
                         values=np.random.random([M, N]),
                         unit=sc.units.counts)
    plot(d["a"])
    plot(d["b"])


def test_plot_realigned_2d():
    d = make_events_dataset(ndim=1)
    tbins = sc.Variable(dims=['tof'], unit=sc.units.us, values=np.arange(100.))
    r = sc.realign(d, {'tof': tbins})
    plot(r)
