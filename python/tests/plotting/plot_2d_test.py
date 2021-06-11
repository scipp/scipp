# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

from pathlib import Path
from tempfile import TemporaryDirectory
import pytest
import numpy as np
import scipp as sc
from ..factory import make_dense_data_array, make_dense_dataset, \
                      make_binned_data_array
from .plot_helper import plot


def test_plot_2d():
    plot(make_dense_data_array(ndim=2))


def test_plot_2d_dataset():
    plot(make_dense_dataset(ndim=2))


def test_plot_2d_with_variances():
    plot(make_dense_data_array(ndim=2, variances=True))


def test_plot_2d_with_log():
    plot(make_dense_data_array(ndim=2), norm='log')


def test_plot_2d_with_log_and_variances():
    plot(make_dense_data_array(ndim=2, variances=True), norm='log')


def test_plot_2d_with_vmin_vmax():
    da = make_dense_data_array(ndim=2)
    plot(da, vmin=0.1 * da.unit, vmax=0.9 * da.unit)


def test_plot_2d_with_unit():
    plot(make_dense_data_array(ndim=2, unit=sc.units.kg))


def test_plot_2d_with_vmin_vmax_with_log():
    da = make_dense_data_array(ndim=2)
    plot(da, vmin=0.1 * da.unit, vmax=0.9 * da.unit, norm='log')


def test_plot_2d_with_log_scale_x():
    plot(make_dense_data_array(ndim=2), scale={'xx': 'log'})


def test_plot_2d_with_log_scale_y():
    plot(make_dense_data_array(ndim=2), scale={'yy': 'log'})


def test_plot_2d_with_log_scale_xy():
    plot(make_dense_data_array(ndim=2), scale={'xx': 'log', 'yy': 'log'})


def test_plot_2d_with_aspect():
    plot(make_dense_data_array(ndim=2), aspect='equal')
    plot(make_dense_data_array(ndim=2), aspect='auto')


def test_plot_2d_with_with_nan():
    da = make_dense_data_array(ndim=2)
    da.values[0, 0] = np.nan
    plot(da)


def test_plot_2d_with_with_nan_with_log():
    da = make_dense_data_array(ndim=2)
    da.values[0, 0] = np.nan
    plot(da, norm='log')


def test_plot_2d_with_cmap():
    plot(make_dense_data_array(ndim=2), cmap='jet')


def test_plot_2d_with_xaxis_specified():
    plot(make_dense_data_array(ndim=2), axes={'x': 'yy'})


def test_plot_2d_with_yaxis_specified():
    plot(make_dense_data_array(ndim=2), axes={'y': 'xx'})


def test_plot_2d_with_labels():
    plot(make_dense_data_array(ndim=2, labels=True), axes={'x': 'lab'})


def test_plot_2d_with_attrs():
    plot(make_dense_data_array(ndim=2, attrs=True), axes={'x': 'attr'})


def test_plot_2d_with_filename():
    with TemporaryDirectory() as dirname:
        plot(make_dense_data_array(ndim=2),
             filename=Path(dirname) / 'image.pdf',
             close=False)


def test_plot_2d_with_bin_edges():
    plot(make_dense_data_array(ndim=2, binedges=True))


def test_plot_2d_with_masks():
    plot(make_dense_data_array(ndim=2, masks=True))


def test_plot_2d_with_masks_and_labels():
    plot(make_dense_data_array(ndim=2, masks=True, labels=True),
         axes={'x': 'lab'})


def test_plot_2d_with_non_regular_bin_edges():
    da = make_dense_data_array(ndim=2, binedges=True)
    da.coords['xx'].values = da.coords['xx'].values**2
    plot(da)


def test_plot_2d_with_non_regular_bin_edges_resolution():
    da = make_dense_data_array(ndim=2, binedges=True)
    da.coords['xx'].values = da.coords['xx'].values**2
    plot(da, resolution=128)


def test_plot_2d_with_non_regular_bin_edges_with_masks():
    da = make_dense_data_array(ndim=2, masks=True, binedges=True)
    da.coords['xx'].values = da.coords['xx'].values**2
    plot(da)


def test_plot_variable_2d():
    N = 50
    v2d = sc.Variable(['yy', 'xx'], values=np.random.rand(N, N), unit='K')
    plot(v2d)


def test_plot_ndarray_2d():
    plot(np.random.random([10, 50]))


def test_plot_dict_of_ndarrays_2d():
    plot({'a': np.arange(50).reshape(5, 10), 'b': np.random.random([30, 40])})


def test_plot_from_dict_variable_2d():
    plot({'dims': ['yy', 'xx'], 'values': np.random.random([20, 10])})


def test_plot_from_dict_data_array_2d():
    plot({
        'data': {
            'dims': ['yy', 'xx'],
            'values': np.random.random([20, 10])
        },
        'coords': {
            'xx': {
                'dims': ['xx'],
                'values': np.arange(11)
            },
            'yy': {
                'dims': ['yy'],
                'values': np.arange(21)
            }
        }
    })


def test_plot_string_and_vector_axis_labels_2d():
    N = 10
    M = 5
    vecs = []
    for i in range(N):
        vecs.append(np.random.random(3))
    da = sc.DataArray(data=sc.Variable(['yy', 'xx'],
                                       values=np.random.random([M, N]),
                                       unit='counts'),
                      coords={
                          'xx':
                          sc.vectors(dims=['xx'], values=vecs, unit='m'),
                          'yy':
                          sc.Variable(['yy'],
                                      values=['a', 'b', 'c', 'd', 'e'],
                                      unit='m')
                      })
    plot(da)


def test_plot_2d_with_dimension_of_size_1():
    N = 10
    M = 1
    x = np.arange(N, dtype=np.float64)
    y = np.arange(M, dtype=np.float64)
    z = np.arange(M + 1, dtype=np.float64)
    d = sc.Dataset()
    d['a'] = sc.Variable(['yy', 'xx'],
                         values=np.random.random([M, N]),
                         unit=sc.units.counts)
    d['b'] = sc.Variable(['zz', 'xx'],
                         values=np.random.random([M, N]),
                         unit=sc.units.counts)
    d.coords['xx'] = sc.Variable(['xx'], values=x, unit=sc.units.m)
    d.coords['yy'] = sc.Variable(['yy'], values=y, unit=sc.units.m)
    d.coords['zz'] = sc.Variable(['zz'], values=z, unit=sc.units.m)
    plot(d['a'])
    plot(d['b'])


def test_plot_2d_with_dimension_of_size_2():
    a = sc.DataArray(data=sc.Variable(dims=['yy', 'xx'], shape=[2, 4]),
                     coords={
                         'xx': sc.Variable(dims=['xx'], values=[1, 2, 3, 4]),
                         'yy': sc.Variable(dims=['yy'], values=[1, 2])
                     })
    plot(a)


def test_plot_2d_ragged_coord():
    plot(make_dense_data_array(ndim=2, ragged=True))


def test_plot_2d_ragged_coord_bin_edges():
    plot(make_dense_data_array(ndim=2, ragged=True, binedges=True))


def test_plot_2d_ragged_coord_with_masks():
    plot(make_dense_data_array(ndim=2, ragged=True, masks=True))


def test_plot_2d_with_labels_but_no_dimension_coord():
    da = make_dense_data_array(ndim=2, labels=True)
    del da.coords['xx']
    plot(da, axes={'x': 'lab'})


def test_plot_2d_with_decreasing_edges():
    a = sc.DataArray(data=sc.Variable(dims=['yy', 'xx'],
                                      values=np.arange(12).reshape(3, 4)),
                     coords={
                         'xx': sc.Variable(dims=['xx'], values=[4, 3, 2, 1]),
                         'yy': sc.Variable(dims=['yy'], values=[1, 2, 3])
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
    plot(data, axes={'x': 'run', 'y': 'xx'})


def test_plot_2d_binned_data_with_variances():
    plot(make_binned_data_array(ndim=2, variances=True))


def test_plot_2d_binned_data_with_variances_resolution():
    plot(make_binned_data_array(ndim=2, variances=True), resolution=64)


def test_plot_2d_binned_data_with_masks():
    plot(make_binned_data_array(ndim=2, masks=True))


def test_plot_customized_mpl_axes():
    da = make_dense_data_array(ndim=2)
    plot(da, title='MyTitle', xlabel='MyXlabel', ylabel='MyYlabel')


def test_plot_access_ax_and_fig():
    da = make_dense_data_array(ndim=2)
    out = sc.plot(da, title='MyTitle')
    out.ax.set_xlabel('MyXlabel')
    out.fig.set_dpi(120.)
    out.close()


def test_plot_2d_int32():
    plot(make_dense_data_array(ndim=2, dtype=sc.dtype.int32))


def test_plot_2d_int64_with_unit():
    plot(make_dense_data_array(ndim=2, unit='K', dtype=sc.dtype.int64))


def test_plot_2d_int_coords():
    N = 20
    M = 10
    da = sc.DataArray(data=sc.Variable(['yy', 'xx'],
                                       values=np.random.random([M, N]),
                                       unit='K'),
                      coords={
                          'xx': sc.arange('xx', N + 1, unit='m'),
                          'yy': sc.arange('yy', M, unit='m')
                      })
    plot(da)


def test_plot_2d_datetime():
    time = sc.array(dims=['time'],
                    values=np.arange(
                        np.datetime64('2017-01-01T12:00:00'),
                        np.datetime64('2017-01-01T12:00:00.0001')))
    N, M = time.sizes['time'], 200
    da = sc.DataArray(data=sc.array(dims=['time', 'xx'],
                                    values=np.random.normal(0, 1, (N, M))),
                      coords={
                          'time': time,
                          'xx': sc.Variable(['xx'],
                                            values=np.linspace(0, 10, M))
                      })
    da.plot().close()


def test_plot_redraw_dense():
    da = make_dense_data_array(ndim=2, unit='K')
    p = sc.plot(da)
    before = p.view.figure.image_values.get_array()
    da *= 5.0
    p.redraw()
    assert np.allclose(p.view.figure.image_values.get_array(), 5.0 * before)
    p.close()


def test_plot_redraw_dense_int64():
    da = make_dense_data_array(ndim=2, unit='K', dtype=sc.dtype.int64)
    p = sc.plot(da)
    before = p.view.figure.image_values.get_array()
    da *= 5
    p.redraw()
    assert np.allclose(p.view.figure.image_values.get_array(), 5 * before)
    p.close()


def test_plot_redraw_counts():
    da = make_dense_data_array(ndim=2, unit='counts')
    p = sc.plot(da)
    before = p.view.figure.image_values.get_array()
    da *= 5.0
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
    def make_array(dims, coord_name):
        return sc.DataArray(data=sc.fold(sc.arange('xx', 2 * 10), 'xx', {
            dims[0]: 10,
            dims[1]: 2
        }),
                            coords={
                                coord_name:
                                sc.fold(0.1 * sc.arange('xx', 20), 'xx', {
                                    dims[0]: 10,
                                    dims[1]: 2
                                })
                            })

    # Ill-formed
    a = make_array(['xx', 'yy'], 'xx')
    with pytest.raises(sc.DimensionError):
        plot(a)
    # Good dim order
    b = make_array(['yy', 'xx'], 'xx')
    plot(b)
    # Non-dim coord
    c = make_array(['xx', 'yy'], 'zz')
    plot(c)
    plot(c, axes={'xx': 'zz'})
