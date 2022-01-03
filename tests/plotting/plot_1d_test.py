# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

import numpy as np
import scipp as sc
from ..factory import make_dense_data_array, make_dense_dataset
from .plot_helper import plot
import matplotlib

matplotlib.use('Agg')

# TODO: For now we are just checking that the plot does not throw any errors.
# In the future it would be nice to check the output by either comparing
# checksums or by using tools like squish.


def test_plot_1d():
    da = make_dense_data_array(ndim=1)
    plot(da)
    plot(da, resampling_mode='sum')
    plot(da, resampling_mode='mean')


def test_plot_1d_with_variances():
    plot(make_dense_data_array(ndim=1, with_variance=True))


def test_plot_1d_bin_edges():
    da = make_dense_data_array(ndim=1, binedges=True)
    plot(da)
    plot(da, resampling_mode='sum')
    plot(da, resampling_mode='mean')


def test_plot_1d_with_labels():
    plot(make_dense_data_array(ndim=1, labels=True), labels={"xx": "lab"})


def test_plot_1d_with_attrs():
    plot(make_dense_data_array(ndim=1, attrs=True), labels={"xx": "attr"})


def test_plot_1d_log_axes():
    da = make_dense_data_array(ndim=1)
    da = sc.abs(da) + 1.0 * sc.units.counts
    plot(da, scale={'x': 'log'})
    plot(da, norm='log')
    plot(da, norm='log', scale={'x': 'log'})
    plot(da, norm='log', scale={'x': 'log'}, resampling_mode='sum')
    plot(da, norm='log', scale={'x': 'log'}, resampling_mode='mean')


def test_plot_1d_bin_edges_with_variances():
    plot(make_dense_data_array(ndim=1, with_variance=True, binedges=True))


def test_plot_1d_two_separate_entries():
    ds = make_dense_dataset(ndim=1)
    ds['b'].unit = 'kg'
    plot(ds)


def test_plot_1d_two_entries_on_same_plot():
    ds = make_dense_dataset(ndim=1)
    plot(ds)


def test_plot_1d_two_entries_hide_variances():
    ds = make_dense_dataset(ndim=1, with_variance=True)
    ds['b'].data.variances = None
    plot(ds, errorbars=False)
    # When variances are not present, the plot does not fail, is silently does
    # not show variances
    plot(ds, errorbars={"a": False, "b": True})


def test_plot_1d_log_axes_two_entries_zero_data():
    a = sc.linspace(dim='xx', unit='K', start=1, stop=2, num=4)
    b = sc.zeros_like(a)  # zero data triggers special branch in limit finding
    plot({'a': a, 'b': b}, norm='log')


def test_plot_1d_with_masks():
    plot(make_dense_data_array(ndim=1, masks=True))


def test_plot_collapse():
    da = make_dense_data_array(ndim=2)
    plot(sc.collapse(da['yy', :10], keep='xx'))


def test_plot_sliceviewer_with_1d_projection():
    da = make_dense_data_array(ndim=3)
    plot(da, projection="1d")
    plot(da, projection="1d", resampling_mode='sum')
    plot(da, projection="1d", resampling_mode='mean')


def test_plot_sliceviewer_with_1d_projection_with_nans():
    da = make_dense_data_array(ndim=3, binedges=True, with_variance=True)
    da.values = np.where(da.values < 0.0, np.nan, da.values)
    da.variances = np.where(da.values < 0.2, np.nan, da.variances)
    plot(da, projection='1d')

    # TODO: moving the sliders is disabled for now, because we are not in a
    # Jupyter backend and once the plot has returned, the widgets no longer
    # exist. We need to re-enable this once we introduce unit tests for the
    # widgets themselves, and find a good way to test slider events.
    # # Move the sliders
    # p['tof.x.y.counts'].controller.widgets.slider[sc.Dim('tof')].value = 10
    # p['tof.x.y.counts'].controller.widgets.slider[sc.Dim('x')].value = 10
    # p['tof.x.y.counts'].controller.widgets.slider[sc.Dim('y')].value = 10


def test_plot_projection_1d_two_entries():
    ds = make_dense_dataset(ndim=2, unit='K')
    p = sc.plot(ds, projection="1d")
    assert not hasattr(p, "len")
    p.close()


def test_plot_projection_1d_two_entries_different_dims():
    da1 = make_dense_data_array(ndim=2, unit='K')
    da2 = make_dense_data_array(ndim=2, dims=['zz', 'yy'], unit='K')
    p = sc.plot({'a': da1, 'b': da2}, projection="1d")
    assert len(p) == 2
    p.close()


def test_plot_variable_1d():
    plot(sc.arange('xx', 50., unit='counts'))


def test_plot_dict_of_variables_1d():
    v1 = sc.arange('xx', 50.0, unit='s')
    v2 = 5.0 * v1
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
    ds = make_dense_dataset(ndim=2)
    plot(ds['xx', 0])


def test_plot_vector_axis_labels_1d():
    N = 10
    da = sc.DataArray(data=sc.Variable(dims=['xx'],
                                       values=np.random.random(N),
                                       unit='counts'),
                      coords={
                          'xx':
                          sc.vectors(dims=['xx'],
                                     values=np.random.random([N, 3]),
                                     unit=sc.units.m)
                      })
    plot(da)


def test_plot_string_axis_labels_1d():
    N = 10
    da = sc.DataArray(data=sc.Variable(dims=['xx'],
                                       values=np.random.random(N),
                                       unit='counts'),
                      coords={
                          'xx':
                          sc.Variable(
                              dims=['xx'],
                              values=["a", "b", "c", "d", "e", "f", "g", "h", "i", "j"],
                              unit='m')
                      })
    plot(da)


def test_plot_string_axis_labels_1d_short():
    N = 5
    da = sc.DataArray(data=sc.Variable(dims=['xx'],
                                       values=np.random.random(N),
                                       unit='counts'),
                      coords={
                          'xx':
                          sc.Variable(dims=['xx'],
                                      values=["a", "b", "c", "d", "e"],
                                      unit='m')
                      })
    plot(da)


def test_plot_with_vector_labels():
    N = 10
    da = sc.DataArray(data=sc.Variable(dims=['xx'],
                                       values=np.random.random(N),
                                       unit='counts'),
                      coords={
                          'xx':
                          sc.arange('xx', float(N), unit='m'),
                          'labs':
                          sc.vectors(dims=['xx'],
                                     values=np.random.random([N, 3]),
                                     unit='m')
                      })
    plot(da, labels={'xx': 'labs'})


def test_plot_vector_axis_with_labels():
    N = 10
    da = sc.DataArray(data=sc.Variable(dims=['xx'],
                                       values=np.random.random(N),
                                       unit='counts'),
                      coords={
                          'labs':
                          sc.arange('xx', float(N), unit='m'),
                          'xx':
                          sc.vectors(dims=['xx'],
                                     values=np.random.random([N, 3]),
                                     unit='m')
                      })
    plot(da)


def test_plot_customized_mpl_axes():
    da = make_dense_data_array(ndim=1)
    plot(da, title="MyTitle", xlabel="MyXlabel", ylabel="MyYlabel")


def test_plot_access_ax_and_fig():
    da = make_dense_data_array(ndim=1)
    out = sc.plot(da, title="MyTitle")
    out.ax.set_xlabel("MyXlabel")
    out.fig.set_dpi(120.)
    out.close()


def test_plot_access_ax_and_fig_two_entries():
    d = make_dense_dataset(ndim=1)
    d['b'].unit = 'kg'
    out = sc.plot(d)
    out["['xx'].counts"].ax.set_xlabel("MyXlabel")
    out["['xx'].counts"].fig.set_dpi(120.)
    out.close()


def test_plot_with_integer_coord():
    da = make_dense_data_array(ndim=1)
    da.coords['xx'] = sc.arange('xx', 50, unit='m')
    plot(da)


def test_plot_with_integer_coord_binedges():
    da = make_dense_data_array(ndim=1, binedges=True)
    da.coords['xx'] = sc.arange('xx', 51, unit='m')
    plot(da)


def test_plot_1d_datetime():
    time = sc.array(dims=['time'],
                    values=np.arange(np.datetime64('2017-01-01T12:00:00'),
                                     np.datetime64('2017-01-01T13:00:00')))
    da = sc.DataArray(data=sc.array(dims=['time'],
                                    values=np.random.random(time.sizes['time'])),
                      coords={'time': time})
    da.plot().close()


def test_plot_1d_datetime_binedges():
    time = sc.array(dims=['time'],
                    values=np.arange(np.datetime64('2017-01-01T12:00:00'),
                                     np.datetime64('2017-01-01T13:00:00'), 20))

    da = sc.DataArray(data=sc.array(dims=['time'],
                                    values=np.random.random(time.sizes['time'] - 1),
                                    unit="K"),
                      coords={'time': time})
    da.plot().close()


def test_plot_1d_datetime_with_labels():
    time = sc.array(dims=['time'],
                    values=np.arange(np.datetime64('2017-01-01T12:00:00'),
                                     np.datetime64('2017-01-01T13:00:00')))
    da = sc.DataArray(data=sc.array(dims=['time'],
                                    values=np.random.random(time.sizes['time'])),
                      coords={'time2': time})
    da.plot().close()


def test_plot_legend():
    da = make_dense_data_array(ndim=1)
    plot(da, legend=False)
    plot(da, legend={"show": False})
    plot(da, legend={"loc": 5})
    plot(da, legend={"show": True, "loc": 4})


def test_plot_1d_with_grid():
    plot(make_dense_data_array(ndim=1), grid=True)


def test_plot_redraw():
    da = make_dense_data_array(ndim=1)
    p = sc.plot(da)
    assert p.view.figure._lines[''].data.get_ydata()[2] == 10.0 * np.sin(2.0)
    da *= 5.0
    p.redraw()
    assert p.view.figure._lines[''].data.get_ydata()[2] == 50.0 * np.sin(2.0)


def test_plot_redraw_int64():
    da = make_dense_data_array(ndim=1, dtype=sc.dtype.int64)
    p = sc.plot(da)
    assert p.view.figure._lines[''].data.get_ydata()[2] == int(10.0 * np.sin(2.0))
    da *= 5
    p.redraw()
    assert p.view.figure._lines[''].data.get_ydata()[2] == int(50.0 * np.sin(2.0))


def test_scale_arg_subplots_independent_dims():
    d = sc.Dataset()
    d['a'] = sc.DataArray(sc.arange('x', 10), coords={'x': sc.arange('x', 10)})
    d['b'] = sc.DataArray(sc.arange('y', 5), coords={'y': sc.arange('y', 5)})
    d.plot(scale={'x': 'log'}).close()
