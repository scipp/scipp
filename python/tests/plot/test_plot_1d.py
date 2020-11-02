# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

import numpy as np
import scipp as sc
from plot_helper import make_dense_dataset, make_events_dataset
from scipp.plot import plot

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
    plot(d, axes={"x": "somelabels"})


def test_plot_1d_log_axes():
    d = make_dense_dataset(ndim=1)
    plot(d, scale={'tof': 'log'})
    plot(d, norm='log')
    plot(d, norm='log', scale={'tof': 'log'})


def test_plot_1d_bin_edges_with_variances():
    d = make_dense_dataset(ndim=1, variances=True, binedges=True)
    plot(d)


def test_plot_1d_two_separate_entries():
    d = make_dense_dataset(ndim=1)
    d["Background"] = sc.Variable(['tof'],
                                  values=2.0 * np.random.random(50),
                                  unit=sc.units.kg)
    plot(d)


def test_plot_1d_two_entries_on_same_plot():
    d = make_dense_dataset(ndim=1)
    d["Background"] = sc.Variable(['tof'],
                                  values=2.0 * np.random.random(50),
                                  unit=sc.units.counts)
    plot(d)


def test_plot_1d_two_entries_hide_variances():
    d = make_dense_dataset(ndim=1, variances=True)
    d["Background"] = sc.Variable(['tof'],
                                  values=2.0 * np.random.random(50),
                                  unit=sc.units.counts)
    plot(d, errorbars=False)
    # When variances are not present, the plot does not fail, is silently does
    # not show variances
    plot(d, errorbars={"Sample": False, "Background": True})


def test_plot_1d_with_masks():
    d = make_dense_dataset(ndim=1, masks=True)
    plot(d)


def test_plot_collapse():
    d = make_dense_dataset(ndim=2)
    plot(sc.collapse(d["Sample"], keep='tof'))


def test_plot_sliceviewer_with_1d_projection():
    d = make_dense_dataset(ndim=3)
    plot(d, projection="1d")


def test_plot_sliceviewer_with_1d_projection_with_nans():
    d = make_dense_dataset(ndim=3, binedges=True, variances=True)
    d['Sample'].values = np.where(d['Sample'].values < 0.0, np.nan,
                                  d['Sample'].values)
    d['Sample'].variances = np.where(d['Sample'].values < 0.2, np.nan,
                                     d['Sample'].variances)
    plot(d, projection='1d')

    # TODO: moving the sliders is disabled for now, because we are not in a
    # Jupyter backend and once the plot has returned, the widgets no longer
    # exist. We need to re-enable this once we introduce unit tests for the
    # widgets themselves, and find a good way to test slider events.
    # # Move the sliders
    # p['tof.x.y.counts'].controller.widgets.slider[sc.Dim('tof')].value = 10
    # p['tof.x.y.counts'].controller.widgets.slider[sc.Dim('x')].value = 10
    # p['tof.x.y.counts'].controller.widgets.slider[sc.Dim('y')].value = 10


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
    v1d = sc.Variable(['tof'],
                      values=np.random.random(N),
                      unit=sc.units.counts)
    plot(v1d)


def test_plot_dict_of_variables_1d():
    N = 50
    v1 = sc.Variable(['tof'], values=np.random.random(N), unit=sc.units.counts)
    v2 = sc.Variable(['tof'],
                     values=5.0 * np.random.random(N),
                     unit=sc.units.counts)
    plot({'v1': v1, 'v2': v2})


def test_plot_ndarray_1d():
    plot(np.random.random(50))


def test_plot_dict_of_ndarrays_1d():
    plot({'a': np.arange(20), 'b': np.random.random(50)})


def test_plot_from_dict_variable_1d():
    plot({"dims": ['adim'], "values": np.random.random(20)})


def test_plot_from_dict_data_array_1d():
    plot({
        "data": {
            "dims": ["adim"],
            "values": np.random.random(20)
        },
        "coords": {
            "adim": {
                "dims": ["adim"],
                "values": np.arange(21)
            }
        }
    })


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
                              values=np.random.random(N),
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
                              values=np.random.random(N),
                              unit=sc.units.counts)
    plot(d)


def test_plot_string_axis_labels_1d_short():
    d = sc.Dataset()
    N = 5
    d.coords['x'] = sc.Variable(dims=['x'],
                                values=["a", "b", "c", "d", "e"],
                                unit=sc.units.m)
    d["Sample"] = sc.Variable(['x'],
                              values=np.random.random(N),
                              unit=sc.units.counts)
    plot(d)
