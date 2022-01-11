# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet
import pytest
from pathlib import Path
from tempfile import TemporaryDirectory
import numpy as np
import scipp as sc
from ..factory import make_dense_data_array, make_dense_dataset, \
                      make_binned_data_array
from .plot_helper import plot
import matplotlib

matplotlib.use('Agg')


def test_plot_2d():
    da = make_dense_data_array(ndim=2)
    plot(da)
    plot(da, resampling_mode='sum')
    plot(da, resampling_mode='mean')


def test_plot_2d_dataset():
    plot(make_dense_dataset(ndim=2))


def test_plot_2d_with_variances():
    plot(make_dense_data_array(ndim=2, with_variance=True))


def test_plot_2d_with_log():
    da = make_dense_data_array(ndim=2)
    plot(da, norm='log')
    plot(da, norm='log', resampling_mode='sum')
    plot(da, norm='log', resampling_mode='mean')


def test_plot_2d_with_log_and_variances():
    da = make_dense_data_array(ndim=2, with_variance=True)
    plot(da, norm='log')
    plot(da, norm='log', resampling_mode='sum')
    plot(da, norm='log', resampling_mode='mean')


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


def test_plot_2d_with_grid():
    plot(make_dense_data_array(ndim=2), grid=True)


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


def test_plot_2d_with_labels():
    plot(make_dense_data_array(ndim=2, labels=True), labels={'xx': 'lab'})


def test_plot_2d_with_attrs():
    plot(make_dense_data_array(ndim=2, attrs=True), labels={'xx': 'attr'})


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
    plot(make_dense_data_array(ndim=2, masks=True, labels=True), labels={'xx': 'lab'})


def test_plot_2d_with_2d_coord_1d_mask():
    da = sc.DataArray(sc.arange('a', 6.0).fold('a', {
        'x': 2,
        'y': 3
    }),
                      coords={
                          'x': sc.arange('x', 2.0),
                          'y': sc.arange('a', 6.0).fold('a', {
                              'x': 2,
                              'y': 3
                          })
                      },
                      masks={'m': sc.array(dims=['y'], values=[True, False, True])})
    plot(da)


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
    v2d = sc.Variable(dims=['yy', 'xx'], values=np.random.rand(N, N), unit='K')
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
    da = sc.DataArray(data=sc.Variable(dims=['yy', 'xx'],
                                       values=np.random.random([M, N]),
                                       unit='counts'),
                      coords={
                          'xx':
                          sc.vectors(dims=['xx'], values=vecs, unit='m'),
                          'yy':
                          sc.Variable(dims=['yy'],
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
    d['a'] = sc.Variable(dims=['yy', 'xx'],
                         values=np.random.random([M, N]),
                         unit=sc.units.counts)
    d['b'] = sc.Variable(dims=['zz', 'xx'],
                         values=np.random.random([M, N]),
                         unit=sc.units.counts)
    d.coords['xx'] = sc.Variable(dims=['xx'], values=x, unit=sc.units.m)
    d.coords['yy'] = sc.Variable(dims=['yy'], values=y, unit=sc.units.m)
    d.coords['zz'] = sc.Variable(dims=['zz'], values=z, unit=sc.units.m)
    plot(d['a'])
    plot(d['b'])


def test_plot_2d_with_dimension_of_size_2():
    a = sc.DataArray(data=sc.zeros(dims=['yy', 'xx'], shape=[2, 4]),
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
    plot(da, labels={'xx': 'lab'})


def test_plot_2d_with_decreasing_edges():
    a = sc.DataArray(data=sc.Variable(dims=['yy', 'xx'],
                                      values=np.arange(12).reshape(3, 4)),
                     coords={
                         'xx': sc.Variable(dims=['xx'], values=[4, 3, 2, 1]),
                         'yy': sc.Variable(dims=['yy'], values=[1, 2, 3])
                     })
    plot(a)


def test_plot_2d_binned_data():
    da = make_binned_data_array(ndim=2)
    plot(da)
    plot(da, resampling_mode='sum')
    plot(da, resampling_mode='mean')
    # Try without event-coord so implementation cannot use `histogram`
    for dim in ['xx', 'yy']:
        copy = da.copy()
        del copy.bins.coords[dim]
        # With edge coord, cannot use `bin` directly
        plot(copy)
        copy.coords[dim] = copy.coords[dim][dim, 1:]
        plot(copy)


def test_plot_2d_binned_data_non_counts():
    da = make_binned_data_array(ndim=2)
    da.bins.unit = 'K'
    plot(da)
    # Try without event-coord so implementation cannot use `histogram`
    for dim in ['xx', 'yy']:
        copy = da.copy()
        del copy.bins.coords[dim]
        # With edge coord, cannot use `bin` directly
        plot(copy)
        copy.coords[dim] = copy.coords[dim][dim, 1:]
        plot(copy)


def test_plot_2d_binned_data_float32_coord():
    da = make_binned_data_array(ndim=2)
    da.bins.coords['xx'] = da.bins.coords['xx'].astype('float32')
    plot(da)
    # Try without event-coord so implementation cannot use `histogram`
    for dim in ['xx', 'yy']:
        copy = da.copy()
        del copy.bins.coords[dim]
        # With edge coord, cannot use `bin` directly
        plot(copy)
        copy.coords[dim] = copy.coords[dim][dim, 1:]
        plot(copy)


def test_plot_2d_binned_data_datetime64():
    da = make_binned_data_array(ndim=2, masks=True)
    start = sc.scalar(np.datetime64('now'))
    offset = (1000 * da.coords['xx']).astype('int64')
    offset.unit = 's'
    da.coords['xx'] = start + offset
    offset = (1000 * da.bins.coords['xx']).astype('int64') * sc.scalar(1, unit='s/m')
    da.bins.coords['xx'] = start + offset
    plot(da)
    # Try without event-coord so implementation cannot use `histogram`
    for dim in ['xx', 'yy']:
        copy = da.copy()
        del copy.bins.coords[dim]
        # With edge coord, cannot use `bin` directly
        plot(copy)
        copy.coords[dim] = copy.coords[dim][dim, 1:]
        plot(copy)


def test_plot_3d_binned_data_where_outer_dimension_has_no_event_coord():
    data = make_binned_data_array(ndim=2, masks=True)
    data = sc.concat([data, data * sc.scalar(2.0)], 'run')
    plot_obj = sc.plot(data)
    plot_obj.widgets._controls['run']['slider'].value = 1
    plot_obj.close()


def test_plot_3d_binned_data_where_inner_dimension_has_no_event_coord():
    data = make_binned_data_array(ndim=2)
    data = sc.concat([data, data * sc.scalar(2.0)], 'run')
    plot(sc.transpose(data, dims=['yy', 'xx', 'run']))


def test_plot_2d_binned_data_with_variances():
    plot(make_binned_data_array(ndim=2, with_variance=True))


def test_plot_2d_binned_data_with_variances_resolution():
    plot(make_binned_data_array(ndim=2, with_variance=True), resolution=64)


def test_plot_2d_binned_data_with_masks():
    da = make_binned_data_array(ndim=2, masks=True)
    p = da.plot()
    unmasked = p.view.figure.image_values.get_array()
    da.masks['all'] = da.data.bins.sum() == da.data.bins.sum()
    p = da.plot()
    # Bin masks are *not* applied
    assert np.allclose(p.view.figure.image_values.get_array(), unmasked)
    assert not np.isclose(p.view.figure.image_values.get_array().sum(), 0.0)


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
    da = sc.DataArray(data=sc.Variable(dims=['yy', 'xx'],
                                       values=np.random.random([M, N]),
                                       unit='K'),
                      coords={
                          'xx': sc.arange('xx', N + 1, unit='m'),
                          'yy': sc.arange('yy', M, unit='m')
                      })
    plot(da)


def test_plot_2d_datetime():
    time = sc.array(dims=['time'],
                    values=np.arange(np.datetime64('2017-01-01T12:00:00'),
                                     np.datetime64('2017-01-01T12:00:00.0001')))
    N, M = time.sizes['time'], 200
    da = sc.DataArray(data=sc.array(dims=['time', 'xx'],
                                    values=np.random.normal(0, 1, (N, M))),
                      coords={
                          'time': time,
                          'xx': sc.Variable(dims=['xx'], values=np.linspace(0, 10, M))
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
    da = make_binned_data_array(ndim=2)
    p = sc.plot(da, resolution=64)
    before = p.view.figure.image_values.get_array()
    da *= 5.0
    p.redraw()
    assert np.allclose(p.view.figure.image_values.get_array(), 5.0 * before)
    p.close()


@pytest.mark.skip(reason="Require in-place concatenate")
def test_plot_redraw_binned_concat_inplace():
    a = make_binned_data_array(ndim=2)
    pa = sc.plot(a, resolution=64)
    asum = pa.view.figure.image_values.get_array().sum()
    b = make_binned_data_array(ndim=2)
    pb = sc.plot(b, resolution=64)
    bsum = pb.view.figure.image_values.get_array().sum()

    a.data = a.bins.concatenate(b).data
    a.data = a.bins.concatenate(b).data
    # TODO would need to change data inplace rather than replacing
    a.data = a.bins.concatenate(other=b).data
    pa.redraw()
    assert np.isclose(pa.view.figure.image_values.get_array().sum(), asum + bsum)
    pa.close()
    pb.close()


def test_plot_various_2d_coord():
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

    # Dimension coord for xx
    a = make_array(['xx', 'yy'], 'xx')
    plot(a)
    # Dimension coord for xx
    b = make_array(['yy', 'xx'], 'xx')
    plot(b)
    # Non-dim coord for yy
    c = make_array(['xx', 'yy'], 'zz')
    plot(c)
    plot(c, labels={'yy': 'zz'})
