# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

from pathlib import Path
from tempfile import TemporaryDirectory
import pytest
import numpy as np
import scipp as sc
from plot_helper import make_dense_dataset, make_binned_data_array, plot


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


def test_plot_2d_image_with_attrs():
    plot(make_dense_dataset(ndim=2, attrs=True), axes={'x': 'attr'})


def test_plot_2d_image_with_filename():
    with TemporaryDirectory() as dirname:
        plot(make_dense_dataset(ndim=2),
             filename=Path(dirname) / 'image.pdf',
             close=False)


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
    d['Signal'] = sc.Variable(['y', 'x'],
                              values=np.random.random([M, N]),
                              unit=sc.units.counts)
    d.coords['x'] = sc.vectors(dims=['x'], values=vecs, unit=sc.units.m)
    d.coords['y'] = sc.Variable(['y'],
                                values=['a', 'b', 'c', 'd', 'e'],
                                unit=sc.units.m)
    plot(d)


def test_plot_2d_with_dimension_of_size_1():
    N = 10
    M = 1
    x = np.arange(N, dtype=np.float64)
    y = np.arange(M, dtype=np.float64)
    z = np.arange(M + 1, dtype=np.float64)
    d = sc.Dataset()
    d['a'] = sc.Variable(['y', 'x'],
                         values=np.random.random([M, N]),
                         unit=sc.units.counts)
    d['b'] = sc.Variable(['z', 'x'],
                         values=np.random.random([M, N]),
                         unit=sc.units.counts)
    d.coords['x'] = sc.Variable(['x'], values=x, unit=sc.units.m)
    d.coords['y'] = sc.Variable(['y'], values=y, unit=sc.units.m)
    d.coords['z'] = sc.Variable(['z'], values=z, unit=sc.units.m)
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
    data = sc.concatenate(data, data * sc.scalar(2.0), 'run')
    plot_obj = sc.plot(data)
    plot_obj.widgets.slider[0].value = 1
    plot_obj.close()


def test_plot_3d_binned_data_where_inner_dimension_nas_no_event_coord():
    data = make_binned_data_array(ndim=2)
    data = sc.concatenate(data, data * sc.scalar(2.0), 'run')
    plot(data, axes={'x': 'run', 'y': 'tof'})


def test_plot_2d_binned_data_with_variances():
    plot(make_binned_data_array(ndim=2, variances=True))


def test_plot_2d_binned_data_with_variances_resolution():
    plot(make_binned_data_array(ndim=2, variances=True), resolution=64)


def test_plot_2d_binned_data_with_masks():
    plot(make_binned_data_array(ndim=2, masks=True))


def test_plot_customized_mpl_axes():
    d = make_dense_dataset(ndim=2)
    plot(d["Sample"], title="MyTitle", xlabel="MyXlabel", ylabel="MyYlabel")


def test_plot_access_ax_and_fig():
    d = make_dense_dataset(ndim=2)
    out = sc.plot(d["Sample"], title="MyTitle")
    out.ax.set_xlabel("MyXlabel")
    out.fig.set_dpi(120.)
    out.close()


def test_plot_2d_image_int32():
    plot(make_dense_dataset(ndim=2, dtype=sc.dtype.int32))


def test_plot_2d_image_int64_with_unit():
    plot(make_dense_dataset(ndim=2, unit='K', dtype=sc.dtype.int64))


def test_plot_2d_image_int_coords():
    N = 20
    M = 10
    x = np.arange(N + 1)
    y = np.arange(M)
    d = sc.Dataset()
    d['a'] = sc.Variable(['y', 'x'],
                         values=np.random.random([M, N]),
                         unit=sc.units.K)
    d.coords['x'] = sc.Variable(['x'], values=x, unit=sc.units.m)
    d.coords['y'] = sc.Variable(['y'], values=y, unit=sc.units.m)
    plot(d)


def test_plot_2d_datetime():
    time = sc.array(dims=['time'],
                    values=np.arange(
                        np.datetime64('2017-01-01T12:00:00'),
                        np.datetime64('2017-01-01T12:00:00.0001')))
    N, M = time.sizes['time'], 200
    data2d = sc.DataArray(data=sc.array(dims=['time', 'x'],
                                        values=np.random.normal(0, 1, (N, M))),
                          coords={
                              'time': time,
                              'x': sc.Variable(['x'],
                                               values=np.linspace(0, 10, M))
                          })
    data2d.plot().close()


def test_plot_redraw_dense():
    d = make_dense_dataset(ndim=2, unit='K')
    p = sc.plot(d)
    before = p.view.figure.image_values.get_array()
    d *= 5.0
    p.redraw()
    assert np.allclose(p.view.figure.image_values.get_array(), 5.0 * before)
    p.close()


def test_plot_redraw_dense_int64():
    d = make_dense_dataset(ndim=2, unit='K', dtype=sc.dtype.int64)
    p = sc.plot(d)
    before = p.view.figure.image_values.get_array()
    d *= 5
    p.redraw()
    assert np.allclose(p.view.figure.image_values.get_array(), 5 * before)
    p.close()


def test_plot_redraw_counts():
    d = make_dense_dataset(ndim=2, unit=sc.units.counts)
    p = sc.plot(d)
    before = p.view.figure.image_values.get_array()
    d *= 5.0
    p.redraw()
    assert np.allclose(p.view.figure.image_values.get_array(), 5.0 * before)
    p.close()


def test_plot_redraw_binned():
    a = make_binned_data_array(ndim=2)
    pa = sc.plot(a, resolution=64)
    asum = pa.view.figure.image_values.get_array().sum()
    b = make_binned_data_array(ndim=2)
    pb = sc.plot(b, resolution=64)
    bsum = pb.view.figure.image_values.get_array().sum()

    a.data = a.bins.concatenate(b).data
    pa.redraw()
    assert np.isclose(pa.view.figure.image_values.get_array().sum(),
                      asum + bsum)
    pa.close()
    pb.close()


def test_plot_bad_2d_coord():
    def make_data_array(dims, coord_name):
        return sc.DataArray(data=sc.fold(sc.arange('x', 2 * 10), 'x', {
            dims[0]: 10,
            dims[1]: 2
        }),
                            coords={
                                coord_name:
                                sc.fold(0.1 * sc.arange('x', 20), 'x', {
                                    dims[0]: 10,
                                    dims[1]: 2
                                })
                            })

    # Ill-formed
    a = make_data_array(['x', 'y'], 'x')
    with pytest.raises(sc.DimensionError):
        plot(a)
    # Good dim order
    b = make_data_array(['y', 'x'], 'x')
    plot(b)
    # Non-dim coord
    c = make_data_array(['x', 'y'], 'z')
    plot(c)
    plot(c, axes={'x': 'z'})
