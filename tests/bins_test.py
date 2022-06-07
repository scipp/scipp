# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import pytest
import scipp as sc
import numpy as np
from math import isnan


def test_dense_data_properties_are_none():
    var = sc.scalar(1)
    assert var.bins is None


def test_bins_default_begin_end():
    data = sc.Variable(dims=['x'], values=[1, 2, 3, 4])
    var = sc.bins(dim='x', data=data)
    assert var.dims == data.dims
    assert var.shape == data.shape
    for i in range(4):
        assert sc.identical(var['x', i].value, data['x', i:i + 1])


def test_bins_default_end():
    data = sc.Variable(dims=['x'], values=[1, 2, 3, 4])
    begin = sc.Variable(dims=['y'], values=[1, 3], dtype=sc.DType.int64, unit=None)
    var = sc.bins(begin=begin, dim='x', data=data)
    assert var.dims == begin.dims
    assert var.shape == begin.shape
    assert sc.identical(var['y', 0].value, data['x', 1:3])
    assert sc.identical(var['y', 1].value, data['x', 3:4])


def test_bins_fail_only_end():
    data = sc.Variable(dims=['x'], values=[1, 2, 3, 4])
    end = sc.Variable(dims=['y'], values=[1, 3], dtype=sc.DType.int64, unit=None)
    with pytest.raises(RuntimeError):
        sc.bins(end=end, dim='x', data=data)


def test_bins_constituents():
    var = sc.Variable(dims=['x'], values=[1, 2, 3, 4])
    data = sc.DataArray(data=var,
                        coords={'coord': var},
                        masks={'mask': var},
                        attrs={'attr': var})
    begin = sc.Variable(dims=['y'], values=[0, 2], dtype=sc.DType.int64, unit=None)
    end = sc.Variable(dims=['y'], values=[2, 4], dtype=sc.DType.int64, unit=None)
    binned = sc.bins(begin=begin, end=end, dim='x', data=data)
    events = binned.bins.constituents['data']
    assert 'coord' in events.coords
    assert 'mask' in events.masks
    assert 'attr' in events.attrs
    del events.coords['coord']
    del events.masks['mask']
    del events.attrs['attr']
    # sc.bins makes a (shallow) copy of `data`
    assert 'coord' in data.coords
    assert 'mask' in data.masks
    assert 'attr' in data.attrs
    # ... but when buffer is accessed we can insert/delete meta data
    assert 'coord' not in events.coords
    assert 'mask' not in events.masks
    assert 'attr' not in events.attrs
    events.coords['coord'] = var
    events.masks['mask'] = var
    events.attrs['attr'] = var
    assert 'coord' in events.coords
    assert 'mask' in events.masks
    assert 'attr' in events.attrs


def test_bins():
    data = sc.Variable(dims=['x'], values=[1, 2, 3, 4])
    begin = sc.Variable(dims=['y'], values=[0, 2], dtype=sc.DType.int64, unit=None)
    end = sc.Variable(dims=['y'], values=[2, 4], dtype=sc.DType.int64, unit=None)
    var = sc.bins(begin=begin, end=end, dim='x', data=data)
    assert var.dims == begin.dims
    assert var.shape == begin.shape
    assert sc.identical(var['y', 0].value, data['x', 0:2])
    assert sc.identical(var['y', 1].value, data['x', 2:4])


def test_bins_of_transpose():
    data = sc.Variable(dims=['row'], values=[1, 2, 3, 4])
    begin = sc.Variable(dims=['x', 'y'],
                        values=[[0, 1], [2, 3]],
                        dtype=sc.DType.int64,
                        unit=None)
    end = begin + sc.index(1)
    var = sc.bins(begin=begin, end=end, dim='row', data=data)
    assert sc.identical(sc.bins(**var.transpose().bins.constituents), var.transpose())


def make_binned():
    col = sc.Variable(dims=['event'], values=[1, 2, 3, 4])
    table = sc.DataArray(data=col,
                         coords={'time': col * 2.2},
                         attrs={'attr': col * 3.3},
                         masks={'mask': col == col})
    begin = sc.Variable(dims=['y'], values=[0, 2], dtype=sc.DType.int64, unit=None)
    end = sc.Variable(dims=['y'], values=[2, 4], dtype=sc.DType.int64, unit=None)
    return sc.bins(begin=begin, end=end, dim='event', data=table)


def test_bins_view():
    var = make_binned()
    assert 'time' in var.bins.coords
    assert 'time' in var.bins.meta
    assert 'attr' in var.bins.meta
    assert 'attr' in var.bins.attrs
    assert 'mask' in var.bins.masks
    col = sc.Variable(dims=['event'], values=[1, 2, 3, 4])
    with pytest.raises(sc.DTypeError):
        var.bins.coords['time2'] = col  # col is not binned

    def check(a, b):
        # sc.identical does not work for us directly since we have owning and
        # non-owning views which currently never compare equal.
        assert sc.identical(1 * a, 1 * b)

    var.bins.coords['time'] = var.bins.data
    assert sc.identical(var.bins.coords['time'], var.bins.data)
    var.bins.coords['time'] = var.bins.data * 2.0
    check(var.bins.coords['time'], var.bins.data * 2.0)
    var.bins.coords['time2'] = var.bins.data
    assert sc.identical(var.bins.coords['time2'], var.bins.data)
    var.bins.coords['time3'] = var.bins.data * 2.0
    check(var.bins.coords['time3'], var.bins.data * 2.0)
    var.bins.data = var.bins.coords['time']
    assert sc.identical(var.bins.data, var.bins.coords['time'])
    var.bins.data = var.bins.data * 2.0
    del var.bins.coords['time3']
    assert 'time3' not in var.bins.coords


def test_bins_view_data_array_unit():
    var = make_binned()
    with pytest.raises(sc.UnitError):
        var.unit = 'K'
    assert var.bins.unit == ''
    var.bins.unit = 'K'
    assert var.bins.unit == 'K'


def test_bins_view_coord_unit():
    var = make_binned()
    with pytest.raises(sc.UnitError):
        var.bins.coords['time'].unit = 'K'
    assert var.bins.coords['time'].bins.unit == ''
    var.bins.coords['time'].bins.unit = 'K'
    assert var.bins.coords['time'].bins.unit == 'K'


def test_bins_view_data_unit():
    var = make_binned()
    with pytest.raises(sc.UnitError):
        var.bins.data.unit = 'K'
    assert var.bins.data.bins.unit == ''
    var.bins.data.bins.unit = 'K'
    assert var.bins.data.bins.unit == 'K'


def test_bins_arithmetic():
    var = sc.Variable(dims=['event'], values=[1.0, 2.0, 3.0, 4.0])
    table = sc.DataArray(var, coords={'x': var})
    binned = sc.bin(table, edges=[sc.Variable(dims=['x'], values=[1.0, 5.0])])
    hist = sc.DataArray(data=sc.Variable(dims=['x'], values=[1.0, 2.0]),
                        coords={'x': sc.Variable(dims=['x'], values=[1.0, 3.0, 5.0])})
    binned.bins *= sc.lookup(func=hist, dim='x')
    assert sc.identical(binned.bins.constituents['data'].data,
                        sc.Variable(dims=['event'], values=[1.0, 2.0, 6.0, 8.0]))


@pytest.mark.parametrize("dtype", ['bool', 'int32', 'int64', 'float32', 'float64'])
def test_lookup_getitem(dtype):
    x_lin = sc.linspace(dim='xx', start=0, stop=1, num=4)
    x = x_lin.copy()
    x.values[0] -= 0.01
    data = sc.array(dims=['xx'], values=[0, 1, 0], dtype=dtype)
    hist_lin = sc.DataArray(data=data, coords={'xx': x_lin})
    hist = sc.DataArray(data=data, coords={'xx': x})
    var = sc.array(dims=['event'], values=[0.1, 0.4, 0.1, 0.6, 0.9, 0.2])
    lut = sc.lookup(hist, 'xx')
    expected = sc.array(dims=['event'], values=[0, 1, 0, 1, 0, 0], dtype=dtype)
    assert sc.identical(lut[var], expected)
    lut = sc.lookup(hist_lin, 'xx')
    assert sc.identical(lut[var], expected)


def test_bins_sum_with_masked_buffer():
    N = 5
    values = np.ones(N)
    data = sc.DataArray(data=sc.Variable(dims=['position'],
                                         unit=sc.units.counts,
                                         values=values,
                                         variances=values),
                        coords={
                            'position':
                            sc.Variable(dims=['position'],
                                        values=['site-{}'.format(i) for i in range(N)]),
                            'x':
                            sc.Variable(dims=['position'],
                                        unit=sc.units.m,
                                        values=[0.2] * 5),
                            'y':
                            sc.Variable(dims=['position'],
                                        unit=sc.units.m,
                                        values=[0.2] * 5)
                        },
                        masks={
                            'test-mask':
                            sc.Variable(dims=['position'],
                                        values=[True, False, True, False, True])
                        })
    xbins = sc.Variable(dims=['x'], unit=sc.units.m, values=[0.1, 0.5, 0.9])
    binned = sc.bin(data, edges=[xbins])
    assert binned.bins.sum().values[0] == 2


def test_bins_mean():
    data = sc.DataArray(data=sc.Variable(dims=['position'],
                                         unit=sc.units.counts,
                                         values=[0.1, 0.2, 0.3, 0.4, 0.5]),
                        coords={
                            'x':
                            sc.Variable(dims=['position'],
                                        unit=sc.units.m,
                                        values=[1, 2, 3, 4, 5])
                        })
    xbins = sc.Variable(dims=['x'], unit=sc.units.m, values=[0, 5, 6, 7])
    binned = sc.bin(data, edges=[xbins])

    # Mean of [0.1, 0.2, 0.3, 0.4]
    assert binned.bins.mean().values[0] == 0.25

    # Mean of [0.5]
    assert binned.bins.mean().values[1] == 0.5

    # Mean of last (empty) bin should be NaN
    assert isnan(binned.bins.mean().values[2])

    assert binned.bins.mean().dims == ('x', )
    assert binned.bins.mean().shape == (3, )
    assert binned.bins.mean().unit == sc.units.counts


def test_bins_mean_with_masks():
    data = sc.DataArray(data=sc.Variable(dims=['position'],
                                         unit=sc.units.counts,
                                         values=[0.1, 0.2, 0.3, 0.4, 0.5]),
                        coords={
                            'x':
                            sc.Variable(dims=['position'],
                                        unit=sc.units.m,
                                        values=[1, 2, 3, 4, 5])
                        },
                        masks={
                            'test-mask':
                            sc.Variable(dims=['position'],
                                        values=[False, True, False, True, False])
                        })
    xbins = sc.Variable(dims=['x'], unit=sc.units.m, values=[0, 5, 6, 7])
    binned = sc.bin(data, edges=[xbins])

    # Mean of [0.1, 0.3] (0.2 and 0.4 are masked)
    assert binned.bins.mean().values[0] == 0.2

    # Mean of [0.5]
    assert binned.bins.mean().values[1] == 0.5

    # Mean of last (empty) bin should be NaN
    assert isnan(binned.bins.mean().values[2])

    assert binned.bins.mean().dims == ('x', )
    assert binned.bins.mean().shape == (3, )
    assert binned.bins.mean().unit == sc.units.counts


def test_bins_mean_using_bins():
    # Call to sc.bins gives different data structure compared to sc.bin

    buffer = sc.arange('event', 5, unit=sc.units.ns, dtype=sc.DType.float64)
    begin = sc.array(dims=['x'], values=[0, 2], dtype=sc.DType.int64, unit=None)
    end = sc.array(dims=['x'], values=[2, 5], dtype=sc.DType.int64, unit=None)
    binned = sc.bins(data=buffer, dim='event', begin=begin, end=end)
    means = binned.bins.mean()

    assert sc.identical(
        means,
        sc.array(dims=["x"], values=[0.5, 3], unit=sc.units.ns, dtype=sc.DType.float64))


def test_bins_nanmean():
    data = sc.DataArray(
        data=sc.array(dims=['position'], values=[1.0, 2.0, -np.nan, 5.0, np.nan]),
        coords={'group': sc.array(dims=['position'], values=[0, 1, 1, 0, 2])})
    groups = sc.array(dims=['group'], values=[0, 1, 2])
    binned = sc.bin(data, groups=[groups])

    assert binned.bins.nanmean().values[0] == 3.0
    assert binned.bins.nanmean().values[1] == 2.0
    # The last bin only contains NaN and is thus effectively empty,
    # so even nanmean returns NaN.
    assert isnan(binned.bins.nanmean().values[2])


def test_bins_nansum():
    data = sc.DataArray(
        data=sc.array(dims=['position'], values=[1.0, 2.0, -np.nan, 5.0, np.nan]),
        coords={'group': sc.array(dims=['position'], values=[0, 1, 1, 0, 2])})
    groups = sc.array(dims=['group'], values=[0, 1, 2])
    binned = sc.bin(data, groups=[groups])
    expected = sc.DataArray(sc.array(dims=['group'], values=[6.0, 2.0, 0.0]),
                            coords={'group': groups})
    assert sc.identical(binned.bins.nansum(), expected)


def test_bins_max():
    data = sc.DataArray(
        data=sc.array(dims=['position'], values=[1.0, 2.0, 3.0, 5.0, 6.0]),
        coords={'group': sc.array(dims=['position'], values=[0, 1, 1, 0, 2])})
    groups = sc.array(dims=['group'], values=[0, 1, 2])
    binned = sc.bin(data, groups=[groups])
    expected = sc.DataArray(sc.array(dims=['group'], values=[5.0, 3.0, 6.0]),
                            coords={'group': groups})
    assert sc.identical(binned.bins.max(), expected)


def test_bins_nanmax():
    data = sc.DataArray(
        data=sc.array(dims=['position'], values=[1.0, 2.0, -np.nan, 5.0, 6.0]),
        coords={'group': sc.array(dims=['position'], values=[0, 1, 1, 0, 2])})
    groups = sc.array(dims=['group'], values=[0, 1, 2])
    binned = sc.bin(data, groups=[groups])
    expected = sc.DataArray(sc.array(dims=['group'], values=[5.0, 2.0, 6.0]),
                            coords={'group': groups})
    assert sc.identical(binned.bins.nanmax(), expected)


def test_bins_min():
    data = sc.DataArray(
        data=sc.array(dims=['position'], values=[1.0, 2.0, 3.0, 5.0, 6.0]),
        coords={'group': sc.array(dims=['position'], values=[0, 1, 1, 0, 2])})
    groups = sc.array(dims=['group'], values=[0, 1, 2])
    binned = sc.bin(data, groups=[groups])
    expected = sc.DataArray(sc.array(dims=['group'], values=[1.0, 2.0, 6.0]),
                            coords={'group': groups})
    assert sc.identical(binned.bins.min(), expected)


def test_bins_nanmin():
    data = sc.DataArray(
        data=sc.array(dims=['position'], values=[1.0, 2.0, -np.nan, 5.0, 6.0]),
        coords={'group': sc.array(dims=['position'], values=[0, 1, 1, 0, 2])})
    groups = sc.array(dims=['group'], values=[0, 1, 2])
    binned = sc.bin(data, groups=[groups])
    expected = sc.DataArray(sc.array(dims=['group'], values=[1.0, 2.0, 6.0]),
                            coords={'group': groups})
    assert sc.identical(binned.bins.nanmin(), expected)


def test_bins_all():
    data = sc.DataArray(
        data=sc.array(dims=['position'], values=[True, False, True, True, True]),
        coords={'group': sc.array(dims=['position'], values=[0, 1, 1, 0, 2])})
    groups = sc.array(dims=['group'], values=[0, 1, 2])
    binned = sc.bin(data, groups=[groups])
    expected = sc.DataArray(sc.array(dims=['group'], values=[True, False, True]),
                            coords={'group': groups})
    assert sc.identical(binned.bins.all(), expected)


def test_bins_any():
    data = sc.DataArray(
        data=sc.array(dims=['position'], values=[False, False, True, False, False]),
        coords={'group': sc.array(dims=['position'], values=[0, 1, 1, 0, 2])})
    groups = sc.array(dims=['group'], values=[0, 1, 2])
    binned = sc.bin(data, groups=[groups])
    expected = sc.DataArray(sc.array(dims=['group'], values=[False, True, False]),
                            coords={'group': groups})
    assert sc.identical(binned.bins.any(), expected)


def test_bins_like():
    data = sc.array(dims=['row'], values=[1, 2, 3, 4])
    begin = sc.array(dims=['x'], values=[0, 3], dtype=sc.DType.int64, unit=None)
    end = sc.array(dims=['x'], values=[3, 4], dtype=sc.DType.int64, unit=None)
    binned = sc.bins(begin=begin, end=end, dim='row', data=data)
    dense = sc.array(dims=['x'], values=[1.1, 2.2])
    expected_data = sc.array(dims=['row'], values=[1.1, 1.1, 1.1, 2.2])
    expected = sc.bins(begin=begin, end=end, dim='row', data=expected_data)
    # Prototype is binned variable
    assert sc.identical(sc.bins_like(binned, dense), expected)
    # Prototype is data array with binned data
    binned = sc.DataArray(data=binned)
    assert sc.identical(sc.bins_like(binned, dense), expected)
    # Broadcast
    expected_data = sc.array(dims=['row'], values=[1.1, 1.1, 1.1, 1.1])
    expected = sc.bins(begin=begin, end=end, dim='row', data=expected_data)
    assert sc.identical(sc.bins_like(binned, dense['x', 0]), expected)
    with pytest.raises(sc.DimensionError):
        dense = dense.rename_dims({'x': 'y'})
        sc.bins_like(binned, dense),


def test_histogram_table_define_edges_from_bin_count():
    da = sc.data.table_xyz(100)
    histogrammed = sc.histogram(da, y=4)
    edges = histogrammed.coords['y']
    assert len(edges) == 5
    assert edges.min().value == da.coords['y'].min().value
    assert edges.max().value > da.coords['y'].max().value
    assert edges.max().value == np.nextafter(da.coords['y'].max().value, np.inf)


def test_histogram_binned_define_edges_from_bin_count():
    da = sc.data.binned_x(100, 10)
    histogrammed = sc.histogram(da, y=4)
    edges = histogrammed.coords['y']
    assert len(edges) == 5
    assert edges.min().value == da.bins.coords['y'].min().value
    assert edges.max().value > da.bins.coords['y'].max().value
    assert edges.max().value == np.nextafter(da.bins.coords['y'].max().value, np.inf)


def test_histogram_table_define_edges_from_bin_size():
    da = sc.data.table_xyz(100)
    histogrammed = sc.histogram(da, y=sc.scalar(100, unit='mm'))
    edges = histogrammed.coords['y']
    assert len(edges) == 11
    assert edges.min().value == da.coords['y'].min().value
    assert edges.max().value > da.coords['y'].max().value


def test_histogram_binned_define_edges_from_bin_size():
    da = sc.data.binned_x(100, 10)
    histogrammed = sc.histogram(da, y=sc.scalar(100, unit='mm'))
    edges = histogrammed.coords['y']
    assert len(edges) == 11
    assert edges.min().value == da.bins.coords['y'].min().value
    assert edges.max().value > da.bins.coords['y'].max().value


def test_histogram_table_custom_edges():
    da = sc.data.table_xyz(100)
    y = sc.linspace('y', 0.2, 0.6, num=3, unit='m')
    histogrammed = sc.histogram(da, y=y)
    assert sc.identical(histogrammed.coords['y'], y)


def test_histogram_binned_custom_edges():
    da = sc.data.binned_x(100, 10)
    y = sc.linspace('y', 0.2, 0.6, num=3, unit='m')
    histogrammed = sc.histogram(da, y=y)
    assert sc.identical(histogrammed.coords['y'], y)


def test_histogram_x_and_edges_arg_are_position_only_and_are_ok_as_keyword_args():
    da = sc.data.table_xyz(100)
    da.coords['edges'] = da.coords['x']
    sc.histogram(da, x=4)
    sc.histogram(da, edges=4)


def test_histogram_raises_if_edges_specified_positional_and_as_kwarg():
    da = sc.data.table_xyz(100)
    with pytest.raises(TypeError):
        sc.histogram(da, {'y': 4}, y=4)


def test_histogram_edges_from_positional_arg():
    da = sc.data.table_xyz(100)
    assert sc.identical(sc.histogram(da, {'y': 4}), sc.histogram(da, y=4))
