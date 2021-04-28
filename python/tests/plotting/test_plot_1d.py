# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

import numpy as np
import scipp as sc
from plot_helper import make_dense_dataset, plot

# TODO: For now we are just checking that the plot does not throw any errors.
# In the future it would be nice to check the output by either comparing
# checksums or by using tools like squish.


def test_plot_1d():
    plot(make_dense_dataset(ndim=1))


def test_plot_1d_with_variances():
    plot(make_dense_dataset(ndim=1, variances=True))


def test_plot_1d_bin_edges():
    plot(make_dense_dataset(ndim=1, binedges=True))


def test_plot_1d_with_labels():
    plot(make_dense_dataset(ndim=1, labels=True), axes={"x": "somelabels"})


def test_plot_1d_with_attrs():
    plot(make_dense_dataset(ndim=1, attrs=True), axes={"x": "attr"})


def test_plot_1d_log_axes():
    d = make_dense_dataset(ndim=1)
    for key, val in d.items():
        d[key] = sc.abs(val) + 1.0 * sc.units.counts
    plot(d, scale={'tof': 'log'})
    plot(d, norm='log')
    plot(d, norm='log', scale={'tof': 'log'})


def test_plot_1d_bin_edges_with_variances():
    plot(make_dense_dataset(ndim=1, variances=True, binedges=True))


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
    plot(make_dense_dataset(ndim=1, masks=True))


def test_plot_collapse():
    d = make_dense_dataset(ndim=2)
    plot(sc.collapse(d["Sample"]['x', :10], keep='tof'))


def test_plot_sliceviewer_with_1d_projection():
    plot(make_dense_dataset(ndim=3), projection="1d")


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
    d["Sample"] = sc.Variable(['x'],
                              values=np.random.random(N),
                              unit=sc.units.counts)
    d.coords['x'] = sc.Variable(['x'],
                                values=np.random.random([N, 3]),
                                unit=sc.units.m,
                                dtype=sc.dtype.vector_3_float64)
    plot(d)


def test_plot_string_axis_labels_1d():
    d = sc.Dataset()
    N = 10
    d["Sample"] = sc.Variable(['x'],
                              values=np.random.random(N),
                              unit=sc.units.counts)
    d.coords['x'] = sc.Variable(
        dims=['x'],
        values=["a", "b", "c", "d", "e", "f", "g", "h", "i", "j"],
        unit=sc.units.m)
    plot(d)


def test_plot_string_axis_labels_1d_short():
    d = sc.Dataset()
    N = 5
    d["Sample"] = sc.Variable(['x'],
                              values=np.random.random(N),
                              unit=sc.units.counts)
    d.coords['x'] = sc.Variable(dims=['x'],
                                values=["a", "b", "c", "d", "e"],
                                unit=sc.units.m)
    plot(d)


def test_plot_with_vector_labels():
    N = 10
    d = sc.Dataset()
    d["Sample"] = sc.Variable(['x'],
                              values=np.random.random(N),
                              unit=sc.units.counts)
    d.coords['x'] = sc.Variable(['x'],
                                values=np.arange(N, dtype=np.float64),
                                unit=sc.units.m)
    d.coords['labs'] = sc.Variable(['x'],
                                   values=np.random.random([N, 3]),
                                   unit=sc.units.m,
                                   dtype=sc.dtype.vector_3_float64)
    plot(d)


def test_plot_vector_axis_with_labels():
    d = sc.Dataset()
    N = 10
    d["Sample"] = sc.Variable(['x'],
                              values=np.random.random(N),
                              unit=sc.units.counts)
    d.coords['labs'] = sc.Variable(['x'],
                                   values=np.arange(N, dtype=np.float64),
                                   unit=sc.units.m)
    d.coords['x'] = sc.Variable(['x'],
                                values=np.random.random([N, 3]),
                                unit=sc.units.m,
                                dtype=sc.dtype.vector_3_float64)
    plot(d)


def test_plot_customized_mpl_axes():
    d = make_dense_dataset(ndim=1)
    plot(d["Sample"], title="MyTitle", xlabel="MyXlabel", ylabel="MyYlabel")


def test_plot_access_ax_and_fig():
    d = make_dense_dataset(ndim=1)
    out = sc.plot(d["Sample"], title="MyTitle")
    out.ax.set_xlabel("MyXlabel")
    out.fig.set_dpi(120.)
    out.close()


def test_plot_access_ax_and_fig_two_entries():
    d = make_dense_dataset(ndim=1)
    d["Background"] = sc.Variable(['tof'],
                                  values=2.0 * np.random.random(50),
                                  unit=sc.units.kg)
    out = sc.plot(d)
    out['tof.counts'].ax.set_xlabel("MyXlabel")
    out['tof.counts'].fig.set_dpi(120.)
    out.close()


def test_plot_with_integer_coord():
    d = sc.Dataset()
    N = 10
    d["Sample"] = sc.Variable(['x'],
                              values=np.random.random(N),
                              unit=sc.units.counts)
    d.coords['x'] = sc.Variable(['x'], values=np.arange(N), unit=sc.units.m)
    plot(d)


def test_plot_with_integer_coord_binedges():
    d = sc.Dataset()
    N = 10
    d["Sample"] = sc.Variable(['x'],
                              values=np.random.random(N),
                              unit=sc.units.counts)
    d.coords['x'] = sc.Variable(['x'],
                                values=np.arange(N + 1),
                                unit=sc.units.m)
    plot(d)


def test_plot_1d_datetime():
    time = sc.array(dims=['time'],
                    values=np.arange(np.datetime64('2017-01-01T12:00:00'),
                                     np.datetime64('2017-01-01T13:00:00')))
    da = sc.DataArray(data=sc.array(dims=['time'],
                                    values=np.random.random(
                                        time.sizes['time'])),
                      coords={'time': time})
    da.plot().close()


def test_plot_1d_datetime_binedges():
    time = sc.array(dims=['time'],
                    values=np.arange(np.datetime64('2017-01-01T12:00:00'),
                                     np.datetime64('2017-01-01T13:00:00'), 20))

    da = sc.DataArray(data=sc.array(
        dims=['time'],
        values=np.random.random(time.sizes['time'] - 1),
        unit="K"),
                      coords={'time': time})
    da.plot().close()


def test_plot_1d_datetime_with_labels():
    time = sc.array(dims=['time'],
                    values=np.arange(np.datetime64('2017-01-01T12:00:00'),
                                     np.datetime64('2017-01-01T13:00:00')))
    da = sc.DataArray(data=sc.array(dims=['time'],
                                    values=np.random.random(
                                        time.sizes['time'])),
                      coords={'time2': time})
    da.plot().close()


def test_plot_legend():
    d = make_dense_dataset(ndim=1)
    plot(d, legend=False)
    plot(d, legend={"show": False})
    plot(d, legend={"loc": 5})
    plot(d, legend={"show": True, "loc": 4})
