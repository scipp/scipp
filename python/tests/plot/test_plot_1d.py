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

# TODO: For now we are just checking that the plot does not throw any errors.
# In the future it would be nice to check the output by either comparing
# checksums or by using tools like squish.


def test_plot_1d():
    d = make_dense_dataset(ndim=1)
    plot(d)


def test_plot_1d_with_variances():
    d = make_dense_dataset(ndim=1, variances=True)
    plot(d)


def test_plot_1d_bin_edges():
    d = make_dense_dataset(ndim=1, binedges=True)
    plot(d)


def test_plot_1d_with_labels():
    d = make_dense_dataset(ndim=1, labels=True)
    plot(d, axes=["somelabels"])


def test_plot_1d_log_axes():
    d = make_dense_dataset(ndim=1)
    plot(d, logx=True)
    plot(d, logy=True)
    plot(d, logxy=True)


def test_plot_1d_bin_edges_with_variances():
    d = make_dense_dataset(ndim=1, variances=True, binedges=True)
    plot(d)


def test_plot_1d_two_separate_entries():
    d = make_dense_dataset(ndim=1)
    d["Background"] = sc.Variable(['tof'],
                                  values=2.0 * np.random.rand(50),
                                  unit=sc.units.kg)
    plot(d)


def test_plot_1d_two_entries_on_same_plot():
    d = make_dense_dataset(ndim=1)
    d["Background"] = sc.Variable(['tof'],
                                  values=2.0 * np.random.rand(50),
                                  unit=sc.units.counts)
    plot(d)


def test_plot_1d_two_entries_hide_variances():
    d = make_dense_dataset(ndim=1, variances=True)
    d["Background"] = sc.Variable(['tof'],
                                  values=2.0 * np.random.rand(50),
                                  unit=sc.units.counts)
    plot(d, errorbars=False)
    # When variances are not present, the plot does not fail, is silently does
    # not show variances
    plot(d, errorbars={"Sample": False, "Background": True})


def test_plot_1d_three_entries_with_labels():
    N = 50
    d = make_dense_dataset(ndim=1, labels=True)
    d["Background"] = sc.Variable(['tof'],
                                  values=2.0 * np.random.rand(N),
                                  unit=sc.units.counts)
    d.coords['x'] = sc.Variable(['x'],
                                values=np.arange(N).astype(np.float64),
                                unit=sc.units.m)
    d["Sample2"] = sc.Variable(['x'],
                               values=10.0 * np.random.rand(N),
                               unit=sc.units.counts)
    d.coords["Xlabels"] = sc.Variable(['x'],
                                      values=np.linspace(151., 155., N),
                                      unit=sc.units.s)
    plot(d, axes={'x': "Xlabels", 'tof': "somelabels"})


def test_plot_1d_with_masks():
    d = make_dense_dataset(ndim=1, masks=True)
    plot(d)


def test_plot_collapse():
    d = make_dense_dataset(ndim=2)
    plot(d, collapse='tof')


def test_plot_sliceviewer_with_1d_projection():
    d = make_dense_dataset(ndim=3)
    plot(d, projection="1d")


def test_plot_1d_events_data_with_bool_bins():
    d = make_events_dataset(ndim=1)
    plot(d, bins={'tof': True})


def test_plot_1d_events_data_with_int_bins():
    d = make_events_dataset(ndim=1)
    plot(d, bins={'tof': 50})


def test_plot_1d_events_data_with_nparray_bins():
    d = make_events_dataset(ndim=1)
    plot(d, bins={'tof': np.linspace(0.0, 105.0, 50)})


def test_plot_1d_events_data_with_Variable_bins():
    d = make_events_dataset(ndim=1)
    bins = sc.Variable(['tof'],
                       values=np.linspace(0.0, 105.0, 50),
                       unit=sc.units.us)
    plot(d, bins={'tof': bins})


def test_plot_variable_1d():
    N = 50
    v1d = sc.Variable(['tof'], values=np.random.rand(N), unit=sc.units.counts)
    plot(v1d)


def test_plot_dataset_view():
    d = make_dense_dataset(ndim=2)
    plot(d['x', 0])


def test_plot_data_array():
    d = make_dense_dataset(ndim=1)
    plot(d["Sample"])


def test_plot_vector_axis_labels_1d():
    d = sc.Dataset()
    N = 10
    vecs = []
    for i in range(N):
        vecs.append(np.random.random(3))
    d.coords['x'] = sc.Variable(['x'],
                                values=vecs,
                                unit=sc.units.m,
                                dtype=sc.dtype.vector_3_float64)
    d["Sample"] = sc.Variable(['x'],
                              values=np.random.rand(N),
                              unit=sc.units.counts)
    plot(d)


def test_plot_string_axis_labels_1d():
    d = sc.Dataset()
    N = 10
    d.coords['x'] = sc.Variable(
        dims=['x'],
        values=["a", "b", "c", "d", "e", "f", "g", "h", "i", "j"],
        unit=sc.units.m)
    d["Sample"] = sc.Variable(['x'],
                              values=np.random.rand(N),
                              unit=sc.units.counts)
    plot(d)


def test_plot_string_axis_labels_1d_short():
    d = sc.Dataset()
    N = 5
    d.coords['x'] = sc.Variable(dims=['x'],
                                values=["a", "b", "c", "d", "e"],
                                unit=sc.units.m)
    d["Sample"] = sc.Variable(['x'],
                              values=np.random.rand(N),
                              unit=sc.units.counts)
    plot(d)


def test_plot_realigned_1d():
    d = make_events_dataset(ndim=1)
    tbins = sc.Variable(dims=['tof'], unit=sc.units.us, values=np.arange(100.))
    r = sc.realign(d, {'tof': tbins})
    plot(r['x', 25])
