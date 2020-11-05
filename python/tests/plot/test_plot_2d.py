# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

import numpy as np
import scipp as sc
from plot_helper import make_dense_dataset
from scipp.plot import plot


def test_plot_2d_image():
    d = make_dense_dataset(ndim=2)
    plot(d)


def test_plot_2d_image_with_log():
    d = make_dense_dataset(ndim=2)
    plot(d, norm='log')


def test_plot_2d_image_with_vmin_vmax():
    d = make_dense_dataset(ndim=2)
    plot(d, vmin=0.1, vmax=0.9)


def test_plot_2d_image_with_vmin_vmax_with_log():
    d = make_dense_dataset(ndim=2)
    plot(d, vmin=0.1, vmax=0.9, norm='log')


def test_plot_2d_image_with_log_scale_x():
    d = make_dense_dataset(ndim=2)
    plot(d, scale={'tof': 'log'})


def test_plot_2d_image_with_log_scale_y():
    d = make_dense_dataset(ndim=2)
    plot(d, scale={'x': 'log'})


def test_plot_2d_image_with_log_scale_xy():
    d = make_dense_dataset(ndim=2)
    plot(d, scale={'tof': 'log', 'x': 'log'})


def test_plot_2d_image_with_with_nan():
    d = make_dense_dataset(ndim=2)
    d['Sample'].values[0, 0] = np.nan
    plot(d)


def test_plot_2d_image_with_with_nan_with_log():
    d = make_dense_dataset(ndim=2)
    d['Sample'].values[0, 0] = np.nan
    plot(d, norm='log')


def test_plot_2d_image_with_cmap():
    d = make_dense_dataset(ndim=2)
    plot(d, cmap='jet')


def test_plot_2d_image_with_xaxis_specified():
    d = make_dense_dataset(ndim=2)
    plot(d, axes={'x': 'x'})


def test_plot_2d_image_with_yaxis_specified():
    d = make_dense_dataset(ndim=2)
    plot(d, axes={'y': 'tof'})


def test_plot_2d_image_with_labels():
    d = make_dense_dataset(ndim=2, labels=True)
    plot(d, axes={'x': 'somelabels'})


def test_plot_2d_image_with_filename():
    d = make_dense_dataset(ndim=2)
    plot(d, filename='image.pdf')


def test_plot_2d_image_with_bin_edges():
    d = make_dense_dataset(ndim=2, binedges=True)
    plot(d)


def test_plot_2d_with_masks():
    d = make_dense_dataset(ndim=2, masks=True)
    plot(d)


def test_plot_2d_with_masks_and_labels():
    d = make_dense_dataset(ndim=2, masks=True, labels=True)
    plot(d, axes={'x': 'somelabels'})


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
    N = 10
    M = 5
    x = np.arange(N).astype(np.float64)
    y = np.arange(M).astype(np.float64)
    xx, yy = np.meshgrid(x, y)
    z = np.random.random([M, N])
    for i in range(M):
        xx[i] *= (i + 1.0)
    d = sc.Dataset()
    d.coords['x'] = sc.Variable(['y', 'x'], values=xx, unit=sc.units.m)
    d.coords['y'] = sc.Variable(['y'], values=y, unit=sc.units.m)
    d['a'] = sc.Variable(['y', 'x'], values=z, unit=sc.units.counts)
    plot(d)


def test_plot_2d_ragged_coord_x_edges():
    N = 10
    M = 5
    x = np.arange(N + 1).astype(np.float64)
    y = np.arange(M).astype(np.float64)
    xx, yy = np.meshgrid(x, y)
    z = np.random.random([M, N])
    for i in range(M):
        xx[i] *= (i + 1.0)
    d = sc.Dataset()
    d.coords['x'] = sc.Variable(['y', 'x'], values=xx, unit=sc.units.m)
    d.coords['y'] = sc.Variable(['y'], values=y, unit=sc.units.m)
    d['a'] = sc.Variable(['y', 'x'], values=z, unit=sc.units.kg)
    plot(d)


def test_plot_2d_ragged_coord_y_edges():
    N = 10
    M = 5
    x = np.arange(N).astype(np.float64)
    y = np.arange(M + 1).astype(np.float64)
    xx, yy = np.meshgrid(x, y[:-1])
    z = np.random.random([M, N])
    for i in range(M):
        xx[i] *= (i + 1.0)
    d = sc.Dataset()
    d.coords['x'] = sc.Variable(['y', 'x'], values=xx, unit=sc.units.m)
    d.coords['y'] = sc.Variable(['y'], values=y, unit=sc.units.m)
    d['a'] = sc.Variable(['y', 'x'], values=z, unit=sc.units.counts)
    plot(d)


def test_plot_2d_ragged_coord_x_and_y_edges():
    N = 10
    M = 5
    x = np.arange(N).astype(np.float64)
    y = np.arange(M).astype(np.float64)
    xx, yy = np.meshgrid(x, y)
    z = np.random.random([M, N])
    for i in range(M):
        xx[i] *= (i + 1.0)
    d = sc.Dataset()
    d.coords['x'] = sc.Variable(['y', 'x'], values=xx, unit=sc.units.m)
    d.coords['y'] = sc.Variable(['y'], values=y, unit=sc.units.m)
    d['a'] = sc.Variable(['y', 'x'], values=z, unit=sc.units.counts)
    plot(d)


def test_plot_2d_ragged_coord_with_masks():
    N = 10
    M = 5
    x = np.arange(N + 1).astype(np.float64)
    y = np.arange(M).astype(np.float64)
    xx, yy = np.meshgrid(x, y)
    z = np.random.random([M, N])
    for i in range(M):
        xx[i] *= (i + 1.0)
    d = sc.Dataset()
    d.coords['x'] = sc.Variable(['y', 'x'], values=xx, unit=sc.units.m)
    d.coords['y'] = sc.Variable(['y'], values=y, unit=sc.units.m)
    d['a'] = sc.Variable(['y', 'x'], values=z, unit=sc.units.counts)
    d['a'].masks['b'] = sc.Variable(['y', 'x'],
                                    values=np.where(z < 0.5, True, False),
                                    dtype=bool)
    plot(d)


def test_plot_2d_with_labels_but_no_dimension_coord():
    N = 50
    M = 10
    y = np.arange(M).astype(np.float)
    z = np.random.random([M, N])
    d = sc.Dataset()
    d.coords['y'] = sc.Variable(['y'], values=y, unit=sc.units.m)
    d['Signal'] = sc.Variable(['y', 'x'], values=z, unit=sc.units.kg)
    d.coords['somelabels'] = sc.Variable(['x'],
                                         values=np.linspace(101., 155., N),
                                         unit=sc.units.s)
    plot(d, axes={'x': 'somelabels'})


def test_plot_2d_with_decreasing_edges():
    a = sc.DataArray(data=sc.Variable(dims=['y', 'x'],
                                      values=np.arange(12).reshape(3, 4)),
                     coords={
                         'x': sc.Variable(dims=['x'], values=[4, 3, 2, 1]),
                         'y': sc.Variable(dims=['y'], values=[1, 2, 3])
                     })
    plot(a)
