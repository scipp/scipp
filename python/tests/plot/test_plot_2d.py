# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

import numpy as np
import scipp as sc
from plot_helper import make_dense_dataset, make_binned_data_array
from scipp.plot import plot


def test_plot_2d_image():
    plot(make_dense_dataset(ndim=2))


def test_plot_2d_image_with_variances():
    plot(make_dense_dataset(ndim=2, variances=True))


def test_plot_2d_image_with_log():
    plot(make_dense_dataset(ndim=2), norm='log')


def test_plot_2d_image_with_log_and_variances():
    plot(make_dense_dataset(ndim=2, variances=True), norm='log')


def test_plot_2d_image_with_vmin_vmax():
    plot(make_dense_dataset(ndim=2), vmin=0.1, vmax=0.9)


def test_plot_2d_image_with_unit():
    plot(make_dense_dataset(ndim=2, unit=sc.units.kg))


def test_plot_2d_image_with_vmin_vmax_with_log():
    plot(make_dense_dataset(ndim=2), vmin=0.1, vmax=0.9, norm='log')


def test_plot_2d_image_with_log_scale_x():
    plot(make_dense_dataset(ndim=2), scale={'tof': 'log'})


def test_plot_2d_image_with_log_scale_y():
    plot(make_dense_dataset(ndim=2), scale={'x': 'log'})


def test_plot_2d_image_with_log_scale_xy():
    plot(make_dense_dataset(ndim=2), scale={'tof': 'log', 'x': 'log'})


def test_plot_2d_image_with_aspect():
    plot(make_dense_dataset(ndim=2), aspect="equal")
    plot(make_dense_dataset(ndim=2), aspect="auto")


def test_plot_2d_image_with_with_nan():
    d = make_dense_dataset(ndim=2)
    d['Sample'].values[0, 0] = np.nan
    plot(d)


def test_plot_2d_image_with_with_nan_with_log():
    d = make_dense_dataset(ndim=2)
    d['Sample'].values[0, 0] = np.nan
    plot(d, norm='log')


def test_plot_2d_image_with_cmap():
    plot(make_dense_dataset(ndim=2), cmap='jet')


def test_plot_2d_image_with_xaxis_specified():
    plot(make_dense_dataset(ndim=2), axes={'x': 'x'})


def test_plot_2d_image_with_yaxis_specified():
    plot(make_dense_dataset(ndim=2), axes={'y': 'tof'})


def test_plot_2d_image_with_labels():
    plot(make_dense_dataset(ndim=2, labels=True), axes={'x': 'somelabels'})


def test_plot_2d_image_with_attrss():
    plot(make_dense_dataset(ndim=2, attrs=True), axes={'x': 'attr'})


def test_plot_2d_image_with_filename():
    plot(make_dense_dataset(ndim=2), filename='image.pdf')


def test_plot_2d_image_with_bin_edges():
    plot(make_dense_dataset(ndim=2, binedges=True))


def test_plot_2d_with_masks():
    plot(make_dense_dataset(ndim=2, masks=True))


def test_plot_2d_with_masks_and_labels():
    plot(make_dense_dataset(ndim=2, masks=True, labels=True),
         axes={'x': 'somelabels'})


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


def test_plot_variable_2d():
    N = 50
    v2d = sc.Variable(['tof', 'x'],
                      values=np.random.rand(N, N),
                      unit=sc.units.K)
    plot(v2d)


def test_plot_ndarray_2d():
    plot(np.random.random([10, 50]))


def test_plot_dict_of_ndarrays_2d():
    plot({'a': np.arange(50).reshape(5, 10), 'b': np.random.random([30, 40])})


def test_plot_from_dict_variable_2d():
    plot({"dims": ['x', 'y'], "values": np.random.random([20, 10])})


def test_plot_from_dict_data_array_2d():
    plot({
        "data": {
            "dims": ["x", "y"],
            "values": np.random.random([20, 10])
        },
        "coords": {
            "x": {
                "dims": ["x"],
                "values": np.arange(21)
            },
            "y": {
                "dims": ["y"],
                "values": np.arange(11)
            }
        }
    })


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
                                values=['a', 'b', 'c', 'd', 'e'],
                                unit=sc.units.m)
    d['Signal'] = sc.Variable(['y', 'x'],
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
    d['a'] = sc.Variable(['y', 'x'],
                         values=np.random.random([M, N]),
                         unit=sc.units.counts)
    d['b'] = sc.Variable(['z', 'x'],
                         values=np.random.random([M, N]),
                         unit=sc.units.counts)
    plot(d['a'])
    plot(d['b'])


def test_plot_2d_with_dimension_of_size_2():
    a = sc.DataArray(data=sc.Variable(dims=['y', 'x'], shape=[2, 4]),
                     coords={
                         'x': sc.Variable(dims=['x'], values=[1, 2, 3, 4]),
                         'y': sc.Variable(dims=['y'], values=[1, 2])
                     })
    plot(a)


def test_plot_2d_ragged_coord():
    plot(make_dense_dataset(ndim=2, ragged=True))


def test_plot_2d_ragged_coord_bin_edges():
    plot(make_dense_dataset(ndim=2, ragged=True, binedges=True))


def test_plot_2d_ragged_coord_with_masks():
    plot(make_dense_dataset(ndim=2, ragged=True, masks=True))


def test_plot_2d_with_labels_but_no_dimension_coord():
    d = make_dense_dataset(ndim=2, labels=True)
    del d.coords['x']
    plot(d, axes={'x': 'somelabels'})


def test_plot_2d_with_decreasing_edges():
    a = sc.DataArray(data=sc.Variable(dims=['y', 'x'],
                                      values=np.arange(12).reshape(3, 4)),
                     coords={
                         'x': sc.Variable(dims=['x'], values=[4, 3, 2, 1]),
                         'y': sc.Variable(dims=['y'], values=[1, 2, 3])
                     })
    plot(a)


def test_plot_2d_binned_data():
    plot(make_binned_data_array(ndim=2))


def test_plot_3d_binned_data_where_outer_dimension_has_no_event_coord():
    data = make_binned_data_array(ndim=2)
    data = sc.concatenate(data, data + data, 'run')
    plot_obj = sc.plot.plot(data)
    plot_obj.widgets.slider[0].value = 1


def test_plot_3d_binned_data_where_inner_dimension_nas_no_event_coord():
    data = make_binned_data_array(ndim=2)
    data = sc.concatenate(data, data + data, 'run')
    sc.plot.plot(data, axes={'x': 'run', 'y': 'tof'})


def test_plot_2d_binned_data_with_variances():
    plot(make_binned_data_array(ndim=2, variances=True))


def test_plot_2d_binned_data_with_variances_nbin():
    plot(make_binned_data_array(ndim=2, variances=True), bins={'tof': 3})


def test_plot_2d_binned_data_with_masks():
    plot(make_binned_data_array(ndim=2, masks=True))


def test_plot_customized_mpl_axes():
    d = make_dense_dataset(ndim=2)
    plot(d["Sample"], title="MyTitle", xlabel="MyXlabel", ylabel="MyYlabel")


def test_plot_access_ax_and_fig():
    d = make_dense_dataset(ndim=2)
    out = plot(d["Sample"], title="MyTitle")
    out.ax.set_xlabel("MyXlabel")
    out.fig.set_dpi(120.)


def test_plot_2d_image_int32():
    plot(make_dense_dataset(ndim=2, dtype=sc.dtype.int32))
