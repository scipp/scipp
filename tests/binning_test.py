# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import itertools

import numpy as np
import pytest
from numpy.random import default_rng

import scipp as sc
from scipp.testing import assert_identical


@pytest.mark.parametrize('op', ['bin', 'hist', 'nanhist', 'rebin'])
def test_raises_CoordError_if_coord_not_found(op) -> None:
    table = sc.data.table_xyz(100)
    with pytest.raises(sc.CoordError):
        getattr(table, op)(abc=5)


def test_group_raises_CoordError_if_coord_not_found() -> None:
    table = sc.data.table_xyz(100)
    with pytest.raises(sc.CoordError):
        table.group('abc')


@pytest.mark.parametrize('coord_dtype', ['int32', 'int64', 'float32', 'float64'])
def test_many_combinations(coord_dtype) -> None:
    table = sc.data.table_xyz(100, coord_max=10, coord_dtype=coord_dtype)
    table.coords['label'] = table.coords['x'].to(dtype='int64')

    assert table.group('label').hist(x=5, y=3).sizes == {'label': 10, 'x': 5, 'y': 3}
    assert table.group('label').hist(x=5).sizes == {'label': 10, 'x': 5}
    assert table.group('label').hist().sizes == {'label': 10}
    assert table.hist(x=5, y=3).sizes == {'x': 5, 'y': 3}
    assert table.hist(x=5).sizes == {'x': 5}

    # rebin only supports float64
    if coord_dtype == 'float64':
        assert table.hist(x=5).rebin(x=3).sizes == {'x': 3}
        assert table.hist(x=5, y=3).rebin(x=3, y=2).sizes == {'x': 3, 'y': 2}

    assert table.bin(x=5).bin(x=6).bin(y=6).sizes == {'x': 6, 'y': 6}
    assert table.bin(x=5).bin(y=6, x=7).sizes == {'x': 7, 'y': 6}
    assert table.bin(x=5).hist().sizes == {'x': 5}
    assert table.bin(x=5).hist(x=7).sizes == {'x': 7}
    assert table.bin(x=5).hist(y=7).sizes == {'x': 5, 'y': 7}
    assert table.bin(x=5).hist(x=3, y=7).sizes == {'x': 3, 'y': 7}
    assert table.bin(x=5).group('label').hist().sizes == {'x': 5, 'label': 10}
    assert table.bin(x=5).group('label').hist(y=5).sizes == {
        'x': 5,
        'label': 10,
        'y': 5,
    }


@pytest.mark.parametrize('dtype', ['float32', 'float64'])
def test_hist_table_define_edges_from_bin_count(dtype) -> None:
    da = sc.data.table_xyz(100)
    for c in list(da.coords.keys()):
        da.coords[c] = da.coords[c].to(dtype=dtype)
    histogrammed = da.hist(y=4)
    edges = histogrammed.coords['y']
    assert len(edges) == 5
    assert histogrammed.sizes['y'] == 4
    assert edges.min().value == da.coords['y'].min().value
    assert edges.max().value > da.coords['y'].max().value
    ymax = da.coords['y'].max()
    assert edges.max().value == np.nextafter(
        ymax.value, (ymax + sc.scalar(1.0, unit=ymax.unit, dtype=ymax.dtype)).value
    )


@pytest.mark.parametrize('dtype', ['float32', 'float64'])
def test_hist_table_with_nan_define_edges_from_bin_count(dtype) -> None:
    da = sc.data.table_xyz(100)
    for c in list(da.coords.keys()):
        da.coords[c] = da.coords[c].to(dtype=dtype)
        da.coords[c][30] = float('nan')
    histogrammed = da.hist(y=4)
    edges = histogrammed.coords['y']
    assert len(edges) == 5
    assert histogrammed.sizes['y'] == 4
    assert edges.min().value == da.coords['y'].nanmin().value
    assert edges.max().value > da.coords['y'].nanmax().value
    ymax = da.coords['y'].nanmax()
    assert edges.max().value == np.nextafter(
        ymax.value, (ymax + sc.scalar(1.0, unit=ymax.unit, dtype=ymax.dtype)).value
    )


def test_hist_binned_define_edges_from_bin_count() -> None:
    da = sc.data.binned_x(100, 10)
    histogrammed = da.hist(y=4)
    edges = histogrammed.coords['y']
    assert len(edges) == 5
    assert histogrammed.sizes['y'] == 4
    assert edges.min().value == da.bins.coords['y'].min().value
    assert edges.max().value > da.bins.coords['y'].max().value
    assert edges.max().value == np.nextafter(da.bins.coords['y'].max().value, np.inf)


def test_hist_binned_define_datetime_edges_from_bin_count() -> None:
    size = 100
    table = sc.data.table_xyz(size)
    table.coords['time'] = sc.epoch(unit='s') + sc.arange(table.dim, 0, 100, unit='s')
    histogrammed = table.hist(time=4)
    edges = histogrammed.coords['time']
    assert len(edges) == 5
    assert histogrammed.sizes['time'] == 4
    assert edges.min().value == table.coords['time'].min().value
    assert edges.max().value > table.coords['time'].max().value


def test_hist_binned_no_additional_edges() -> None:
    da = sc.data.binned_x(100, 10)
    assert sc.identical(da.hist(), da.bins.sum())


def test_hist_table_define_edges_from_bin_size() -> None:
    da = sc.data.table_xyz(100)
    histogrammed = da.hist(y=sc.scalar(100, unit='mm'))
    edges = histogrammed.coords['y']
    assert len(edges) == 11
    assert histogrammed.sizes['y'] == 10
    assert edges.min().value == da.coords['y'].min().value
    assert edges.max().value > da.coords['y'].max().value


def test_hist_binned_define_edges_from_bin_size() -> None:
    da = sc.data.binned_x(100, 10)
    histogrammed = da.hist(y=sc.scalar(100, unit='mm'))
    edges = histogrammed.coords['y']
    assert len(edges) == 11
    assert histogrammed.sizes['y'] == 10
    assert edges.min().value == da.bins.coords['y'].min().value
    assert edges.max().value > da.bins.coords['y'].max().value


def test_hist_table_custom_edges() -> None:
    da = sc.data.table_xyz(100)
    y = sc.linspace('y', 0.2, 0.6, num=3, unit='m')
    histogrammed = da.hist(y=y)
    assert sc.identical(histogrammed.coords['y'], y)


def test_hist_binned_custom_edges() -> None:
    da = sc.data.binned_x(100, 10)
    y = sc.linspace('y', 0.2, 0.6, num=3, unit='m')
    histogrammed = da.hist(y=y)
    assert sc.identical(histogrammed.coords['y'], y)


def test_hist_x_and_edges_arg_are_position_only_and_are_ok_as_keyword_args() -> None:
    da = sc.data.table_xyz(100)
    da.coords['edges'] = da.coords['x']
    da.hist(x=4)
    da.hist(edges=4)


def test_hist_raises_if_edges_specified_positional_and_as_kwarg() -> None:
    da = sc.data.table_xyz(100)
    with pytest.raises(TypeError):
        da.hist({'y': 4}, y=4)


def test_hist_edges_from_positional_arg() -> None:
    da = sc.data.table_xyz(100)
    assert sc.identical(da.hist({'y': 4}), da.hist(y=4))


def test_hist_without_event_coord_uses_outer_coord() -> None:
    table = sc.data.table_xyz(100)
    table.data = sc.arange('row', 100.0)
    da = table.bin(x=10)
    da.coords['outer'] = sc.arange('x', 10)
    expected = da.bin(x=2).hist().rename(x='outer')
    expected.coords['outer'] = sc.array(dims=['outer'], values=[0, 5, 10])
    assert sc.identical(da.hist(outer=2), expected)


def test_hist_2d_without_event_coord_uses_outer_dim_coord() -> None:
    table = sc.data.table_xyz(100)
    table.data = sc.arange('row', 100.0)
    da = table.bin(x=10)
    expected = da.bin(x=2, y=4).hist().rename(x='outer')
    expected.coords['outer'] = sc.array(dims=['outer'], values=[0, 5, 10])
    da = da.rename_dims(x='outer')
    da.coords['outer'] = sc.arange('outer', 10)
    assert sc.identical(da.hist(outer=2, y=4), expected)
    assert sc.identical(da.hist(y=4, outer=2), expected.transpose())


def test_hist_2d_without_event_coord_uses_outer_non_dim_coord() -> None:
    table = sc.data.table_xyz(100)
    table.data = sc.arange('row', 100.0)
    da = table.bin(x=10)
    expected = da.bin(x=2, y=4).hist().rename(x='outer')
    expected.coords['outer'] = sc.array(dims=['outer'], values=[0, 5, 10])
    da.coords['outer'] = sc.arange('x', 10)
    assert sc.identical(da.hist(outer=2, y=4), expected)
    assert sc.identical(da.hist(y=4, outer=2), expected.transpose())


def test_hist_3d_without_event_coord_uses_outer_non_dim_coord_and_erases() -> None:
    table = sc.data.table_xyz(100)
    table.data = sc.arange('row', 100.0)
    da = table.bin(x=10)
    expected = 2 * da.bin(x=2, y=4).hist().rename(x='outer')
    expected.coords['outer'] = sc.array(dims=['outer'], values=[0, 5, 10])
    da = sc.concat([da, da], dim='z')
    da.coords['outer'] = sc.arange('x', 10)
    assert sc.identical(da.hist(outer=2, y=4, dim=('x', 'z')), expected)
    assert sc.identical(da.hist(y=4, outer=2, dim=('x', 'z')), expected.transpose())


def test_hist_3d_without_event_coord_uses_outer_2d_non_dim_coord_and_erases() -> None:
    table = sc.data.table_xyz(100)
    table.data = sc.arange('row', 100.0)
    da = table.bin(x=10)
    expected = 2 * da.bin(x=2, y=4).hist().rename(x='outer')
    expected.coords['outer'] = sc.array(dims=['outer'], values=[0, 5, 10])
    outer = sc.arange('x', 10)
    da = sc.concat([da, da], dim='z')
    da.coords['outer'] = sc.concat([outer, outer], dim='z')
    assert sc.identical(da.hist(outer=2, y=4, dim=('x', 'z')), expected)
    assert sc.identical(da.hist(y=4, outer=2, dim=('x', 'z')), expected.transpose())


def test_hist_without_event_coord_raises_with_outer_bin_edge_coord() -> None:
    table = sc.data.table_xyz(100)
    table.data = sc.arange('row', 100.0)
    da = table.bin(x=10)
    da.coords['outer'] = sc.arange('x', 11.0)
    with pytest.raises(sc.BinEdgeError):
        da.hist(outer=2)


@pytest.mark.parametrize('dtype', ['int32', 'int64'])
def test_bin_integer_coord_by_int(dtype) -> None:
    table = sc.data.table_xyz(100)
    table.coords['label'] = (table.coords['x'] * 10).to(dtype=dtype)
    coord = table.bin(label=2).coords['label']
    expected = sc.array(dims=['label'], unit='m', dtype=dtype, values=[0, 5, 10])
    assert sc.identical(coord, expected)


@pytest.mark.parametrize('dtype', ['int32', 'int64'])
def test_bin_integer_coord_by_stepsize(dtype) -> None:
    table = sc.data.table_xyz(100)
    table.coords['label'] = (table.coords['x'] * 10).to(dtype=dtype)
    coord = table.bin(label=sc.scalar(5, unit='m')).coords['label']
    expected = sc.array(dims=['label'], unit='m', dtype=dtype, values=[0, 5, 10])
    assert sc.identical(coord, expected)


@pytest.mark.parametrize('dtype', ['int32', 'int64'])
def test_bin_integer_coord_by_stepsize_in_different_unit(dtype) -> None:
    table = sc.data.table_xyz(100)
    table.coords['label'] = (table.coords['x'] * 10).to(dtype=dtype)
    coord = table.bin(label=sc.scalar(500, unit='cm')).coords['label']
    expected = sc.array(dims=['label'], unit='m', dtype=dtype, values=[0, 5, 10])
    assert sc.identical(coord, expected)


@pytest.mark.parametrize('dtype', ['int32', 'int64'])
def test_bin_integer_coord_by_float_stepsize(dtype) -> None:
    table = sc.data.table_xyz(100)
    table.coords['label'] = (table.coords['x'] * 10).to(dtype=dtype)
    coord = table.bin(label=sc.scalar(3.3, unit='m')).coords['label']
    expected = sc.array(dims=['label'], unit='m', dtype=dtype, values=[0, 3, 6, 9, 12])
    assert sc.identical(coord, expected)


@pytest.mark.parametrize('dtype', ['int32', 'int64'])
def test_bin_integer_coord_by_fractional_stepsize_raises(dtype) -> None:
    table = sc.data.table_xyz(100)
    table.coords['label'] = (table.coords['x'] * 10).to(dtype=dtype)
    with pytest.raises(ValueError, match='Step size'):
        table.bin(label=sc.scalar(0.5, unit='m'))


def test_bin_with_automatic_bin_bounds_raises_if_no_events() -> None:
    table = sc.data.table_xyz(0)
    with pytest.raises(ValueError, match='Empty data range'):
        table.bin(x=4)


def test_bin_with_manual_bin_bounds_not_raises_if_no_events() -> None:
    table = sc.data.table_xyz(0)
    table.bin(x=sc.linspace('x', 0, 1, 4, unit=table.coords['x'].unit))


def test_group_after_bin_considers_event_value() -> None:
    table = sc.data.table_xyz(100)
    table.coords['label'] = (table.coords['x'] * 10).to(dtype='int64')
    da = table.bin(label=2).group('label')
    assert da.sizes == {'label': 10}


def test_group_by_2d_yields_1d() -> None:
    table = sc.data.table_xyz(100)
    table.coords['label'] = (table.coords['x'] * 10).to(dtype='int64')
    da = table.bin(y=333).group('label')
    da.coords['label'] = (sc.arange('y', 0, 333) * da.coords['label']) % sc.scalar(
        23, unit='m'
    )
    grouped = da.group('label')
    assert grouped.sizes == {'label': 23}


def test_bin_erases_dims_automatically_if_labels_for_same_dim() -> None:
    table = sc.data.table_xyz(100)
    table.coords['x2'] = table.coords['x'] * 2
    da = table.bin(x=10)
    assert da.bin(x2=100).dims == ('x', 'x2')
    # With a bin-coord for x2 we can deduce that the new binning is by different
    # 'labels' for the same data dimension.
    da.coords['x2'] = da.coords['x'] * 2
    assert da.bin(x2=100).dims == ('x2',)


def test_bin_by_two_coords_depending_on_same_dim() -> None:
    table = sc.data.table_xyz(100)
    da = table.bin(x=10, y=12)
    assert da.dims == ('x', 'y')


def test_bin_by_2d_erases_2_input_dims() -> None:
    table = sc.data.table_xyz(100)
    table.coords['xy'] = table.coords['x'] + table.coords['y']
    da = table.bin(x=10, y=12)
    # Note: These are bin edges, but `bin` still works
    da.coords['xy'] = da.coords['x'][1:] + da.coords['y']
    assert da.bin(xy=20).dims == ('xy',)


def test_bin_by_2d_dimension_coord_erases_extra_dim() -> None:
    table = sc.data.table_xyz(100)
    table.coords['xy'] = table.coords['x'] + table.coords['y']
    da = table.bin(x=10, y=12)
    da.coords['xy'] = da.coords['x'][1:] + da.coords['y']
    da = da.rename_dims({'y': 'xy'})
    assert da.bin(xy=20).dims == ('xy',)


def test_bin_by_top_level_group_coord_while_binning_other_coord() -> None:
    table = sc.data.table_xyz(1000, coord_max=10)
    table.coords['lab'] = table.coords['x'].to(dtype='int64')
    grouped = table.group('lab').bin(x=3).bin(y=3)
    grouped.coords['lab2'] = 2 * grouped.coords['lab']

    da = grouped.bin(lab2=5, x=2)

    assert da.sizes == {'lab2': 5, 'x': 2, 'y': 3}
    assert da.coords.is_edges('lab2')
    assert da.coords.is_edges('x')
    assert da.coords.is_edges('y')


def test_hist_on_dense_data_without_edges_raisesTypeError() -> None:
    table = sc.data.table_xyz(10)
    with pytest.raises(TypeError):
        table.hist()
    with pytest.raises(TypeError):
        table.nanhist()


def test_hist_on_dataset_with_binned_data() -> None:
    da = sc.data.table_xyz(100)
    db = sc.data.table_xyz(150)
    db.coords['y'].values[0] = -5.0
    edges = sc.linspace(dim='x', start=0.5, stop=1.0, num=21, unit='m')
    a = da.bin(x=edges)
    b = db.bin(x=edges)
    ds = sc.Dataset({'a': a, 'b': b})
    res = ds.hist(y=50)
    assert res.coords['y'][0] == sc.scalar(-5.0, unit='m')


rng = default_rng(seed=1234)


def date_month_day_table_grouped_by_date():
    size = 100
    table = sc.DataArray(sc.ones(dims=['row'], shape=[size]))
    table.coords['date'] = sc.array(dims=['row'], values=rng.integers(365, size=size))
    table.coords['month'] = table.coords['date'] // 30
    table.coords['day'] = table.coords['date'] % 30
    # Using bin instead of group to preserve the original `date` coord in the buffer,
    # as needed by some tests.
    da = table.bin(date=365)
    da.coords['date'] = da.coords['date'][:-1].copy()
    return da


# In the following we test the automatic erasure of dimensions. It should be consistent
# across the different operations, bin, group, hist, and nanhist.

# 1. bin


def test_bin_without_coord_keeps_dim() -> None:
    da = date_month_day_table_grouped_by_date()
    # Binning is by event-coord
    assert da.bin(month=12).dims == ('date', 'month')


def test_bin_with_1d_dimcoord_keeps_dim() -> None:
    da = date_month_day_table_grouped_by_date()
    assert da.bin(date=4).dims == ('date',)


def test_bin_with_2d_dimcoord_replaces_dims() -> None:
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    da.coords['day'] = da.coords['date'] % 30
    da2d = da.group('month', 'day')
    da2d.coords['date'] = da2d.coords['day'] + 30 * da2d.coords['month']
    da2d = da2d.rename_dims({'month': 'date'})
    assert da2d.bin(date=4).dims == ('date',)


def test_bin_with_nondimcoord_removes_dim() -> None:
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    assert da.bin(month=12).dims == ('month',)


def test_bin_with_multiple_nondimcoords_removes_dim() -> None:
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    da.coords['day'] = da.coords['date'] % 30
    assert da.bin(month=12, day=30).dims == ('month', 'day')


def test_bin_with_dimcoord_and_nondimcoord_keeps_dim() -> None:
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    # 'date' exists, so new coord is inner dim
    assert da.bin(month=12, date=4).dims == ('date', 'month')


def test_bin_with_nondimcoord_removes_multiple_input_dims() -> None:
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    da.coords['day'] = da.coords['date'] % 30
    da2d = da.group('month', 'day')
    da2d.coords['date'] = da2d.coords['day'] + 30 * da2d.coords['month']
    assert da2d.bin(date=4).dims == ('date',)


# 2. group


def test_group_without_coord_keeps_dim() -> None:
    da = date_month_day_table_grouped_by_date()
    # Grouping is by event-coord
    assert da.group('month').dims == ('date', 'month')


def test_group_with_1d_dimcoord_keeps_dim() -> None:
    da = date_month_day_table_grouped_by_date()
    assert da.group(sc.array(dims=['date'], values=[1, 3, 5], dtype='int64')).dims == (
        'date',
    )


def test_group_with_2d_dimcoord_replaces_dims() -> None:
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    da.coords['day'] = da.coords['date'] % 30
    da2d = da.group('month', 'day')
    da2d.coords['date'] = da2d.coords['day'] + 30 * da2d.coords['month']
    da2d = da2d.rename_dims({'month': 'date'})
    assert da2d.group('date').dims == ('date',)


def test_group_with_nondimcoord_removes_dim() -> None:
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    assert da.group('month').dims == ('month',)


def test_group_with_multiple_nondimcoords_removes_dim() -> None:
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    da.coords['day'] = da.coords['date'] % 30
    assert da.group('month', 'day').dims == ('month', 'day')


def test_group_with_dimcoord_and_nondimcoord_keeps_dim() -> None:
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    # 'date' exists, so new coord is inner dim
    assert da.group('month', 'date').dims == ('date', 'month')


def test_group_with_nondimcoord_removes_multiple_input_dims() -> None:
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    da.coords['day'] = da.coords['date'] % 30
    da2d = da.group('month', 'day')
    da2d.coords['date'] = da2d.coords['day'] + 30 * da2d.coords['month']
    assert da2d.group('date').dims == ('date',)


# 3. hist


def test_hist_without_coord_keeps_dim() -> None:
    da = date_month_day_table_grouped_by_date()
    # Histogramming is by event-coord
    assert da.hist(month=12).dims == ('date', 'month')


def test_hist_with_1d_dimcoord_keeps_dim() -> None:
    da = date_month_day_table_grouped_by_date()
    assert da.hist(date=4).dims == ('date',)


def test_hist_with_2d_dimcoord_replaces_dims() -> None:
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    da.coords['day'] = da.coords['date'] % 30
    da2d = da.group('month', 'day')
    da2d.coords['date'] = da2d.coords['day'] + 30 * da2d.coords['month']
    da2d = da2d.rename_dims({'month': 'date'})
    assert da2d.hist(date=4).dims == ('date',)


def test_hist_with_nondimcoord_removes_dim() -> None:
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    assert da.hist(month=12).dims == ('month',)


def test_hist_with_multiple_nondimcoords_removes_dim() -> None:
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    da.coords['day'] = da.coords['date'] % 30
    assert da.hist(month=12, day=30).dims == ('month', 'day')


def test_hist_with_dimcoord_and_nondimcoord_keeps_dim() -> None:
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    assert da.hist(month=12, date=4).dims == ('month', 'date')


def test_hist_with_nondimcoord_removes_multiple_input_dims() -> None:
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    da.coords['day'] = da.coords['date'] % 30
    da2d = da.group('month', 'day')
    da2d.coords['date'] = da2d.coords['day'] + 30 * da2d.coords['month']
    assert da2d.hist(date=4).dims == ('date',)


# 4. nanhist


def test_nanhist_without_coord_keeps_dim() -> None:
    da = date_month_day_table_grouped_by_date()
    # Histogramming is by event-coord
    assert da.nanhist(month=12).dims == ('date', 'month')


def test_nanhist_with_1d_dimcoord_keeps_dim() -> None:
    da = date_month_day_table_grouped_by_date()
    assert da.nanhist(date=4).dims == ('date',)


def test_nanhist_with_2d_dimcoord_replaces_dims() -> None:
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    da.coords['day'] = da.coords['date'] % 30
    da2d = da.group('month', 'day')
    da2d.coords['date'] = da2d.coords['day'] + 30 * da2d.coords['month']
    da2d = da2d.rename_dims({'month': 'date'})
    assert da2d.nanhist(date=4).dims == ('date',)


def test_nanhist_with_nondimcoord_removes_dim() -> None:
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    assert da.nanhist(month=12).dims == ('month',)


def test_nanhist_with_multiple_nondimcoords_removes_dim() -> None:
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    da.coords['day'] = da.coords['date'] % 30
    assert da.nanhist(month=12, day=30).dims == ('month', 'day')


def test_nanhist_with_dimcoord_and_nondimcoord_keeps_dim() -> None:
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    assert da.nanhist(month=12, date=4).dims == ('date', 'month')


def test_nanhist_with_nondimcoord_removes_multiple_input_dims() -> None:
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    da.coords['day'] = da.coords['date'] % 30
    da2d = da.group('month', 'day')
    da2d.coords['date'] = da2d.coords['day'] + 30 * da2d.coords['month']
    assert da2d.nanhist(date=4).dims == ('date',)


def test_bin_along_existing_bin_edge_dims_keeps_bounds() -> None:
    binned = sc.data.table_xyz(100).bin(x=4)
    xold = binned.coords['x']
    xnew = binned.bin(x=5).coords['x']
    assert sc.identical(xnew[0], xold[0])
    assert sc.identical(xnew[-1], xold[-1])


def test_hist_along_existing_bin_edge_dims_keeps_bounds() -> None:
    binned = sc.data.table_xyz(100).bin(x=4)
    xold = binned.coords['x']
    xnew = binned.hist(x=5).coords['x']
    assert sc.identical(xnew[0], xold[0])
    assert sc.identical(xnew[-1], xold[-1])


def test_rebin_along_existing_bin_edge_dims_keeps_bounds() -> None:
    hist = sc.data.table_xyz(100).hist(x=4)
    xold = hist.coords['x']
    xnew = hist.rebin(x=5).coords['x']
    assert sc.identical(xnew[0], xold[0])
    assert sc.identical(xnew[-1], xold[-1])


def test_binning_low_level_functions_exist() -> None:
    from scipp import binning

    binning.make_binned
    binning.make_histogrammed


@pytest.mark.parametrize('op', ['bin', 'hist', 'nanhist'])
def test_raises_ValueError_given_variable_and_multiple_edges(op) -> None:
    var = sc.array(dims=['row'], values=[1, 2, 3])
    with pytest.raises(
        ValueError, match='Edges for exactly one dimension must be specified'
    ):
        getattr(var, op)(x=2, y=2)


def test_variable_hist_equivalent_to_hist_of_data_array_with_counts() -> None:
    data = sc.ones(dims=['row'], shape=[4], unit='counts')
    coord = sc.array(dims=['row'], values=[2, 2, 1, 3])
    da = sc.DataArray(data, coords={'x': coord})
    assert sc.identical(coord.hist(x=3), da.hist(x=3))


def test_variable_nanhist_equivalent_to_nanhist_of_data_array_with_counts() -> None:
    data = sc.ones(dims=['row'], shape=[4], unit='counts')
    coord = sc.array(dims=['row'], values=[2, 2, 1, 3])
    da = sc.DataArray(data, coords={'x': coord})
    assert sc.identical(coord.nanhist(x=3), da.nanhist(x=3))


def test_variable_bin_equivalent_to_bin_of_data_array_with_counts() -> None:
    data = sc.ones(dims=['row'], shape=[4], unit='counts')
    coord = sc.array(dims=['row'], values=[2, 2, 1, 3])
    da = sc.DataArray(data, coords={'x': coord})
    assert sc.identical(coord.bin(x=3), da.bin(x=3))


def test_binned_variable_hist_uses_binned_coord_to_determine_edges() -> None:
    var = sc.bins(
        data=sc.DataArray(sc.ones(sizes={'e': 4}), coords={'x': sc.arange('e', 4.0)}),
        dim='e',
        begin=sc.array(dims=['x'], values=[0, 1, 3], unit=None),
    )
    result = var.hist(x=3)
    assert result.sum().value == 4
    assert sc.identical(
        result.coords['x'], sc.linspace('x', 0.0, np.nextafter(3.0, 4), num=4)
    )


def test_binned_variable_nanhist_uses_binned_coord_to_determine_edges() -> None:
    var = sc.bins(
        data=sc.DataArray(sc.ones(sizes={'e': 4}), coords={'x': sc.arange('e', 4.0)}),
        dim='e',
        begin=sc.array(dims=['x'], values=[0, 1, 3], unit=None),
    )
    result = var.nanhist(x=3)
    assert result.sum().value == 4
    assert sc.identical(
        result.coords['x'], sc.linspace('x', 0.0, np.nextafter(3.0, 4), num=4)
    )


def test_binned_variable_bin_uses_binned_coord_to_determine_edges() -> None:
    var = sc.bins(
        data=sc.DataArray(sc.ones(sizes={'e': 4}), coords={'x': sc.arange('e', 4.0)}),
        dim='e',
        begin=sc.array(dims=['x'], values=[0, 1, 3], unit=None),
    )
    result = var.bin(x=3)
    assert result.bins.size().sum().value == 4
    assert sc.identical(
        result.coords['x'], sc.linspace('x', 0.0, np.nextafter(3.0, 4), num=4)
    )


def test_group_many_to_many_uses_optimized_codepath() -> None:
    size = 512
    table = sc.data.table_xyz(int(1e3))
    da = table.bin(x=size, y=size)
    xcoarse = sc.arange('x', size) // 2
    ycoarse = sc.arange('y', size) // 2
    da.coords['xcoarse'] = xcoarse
    da.coords['ycoarse'] = ycoarse
    # The old "standard" implementation uses a massive amount of memory, crashing even
    # with 256 GByte of RAM.
    assert da.group('xcoarse', 'ycoarse').dims == ('xcoarse', 'ycoarse')


def test_group_many_to_many_with_extra_inner_dim_uses_optimized_codepath() -> None:
    size = 512
    table = sc.data.table_xyz(int(1e3))
    da = table.bin(x=size, y=size, z=3)
    xcoarse = sc.arange('x', size) // 2
    ycoarse = sc.arange('y', size) // 2
    da.coords['xcoarse'] = xcoarse
    da.coords['ycoarse'] = ycoarse
    # The old "standard" implementation uses a massive amount of memory, crashing even
    # with 256 GByte of RAM.
    assert da.group('xcoarse', 'ycoarse').dims == ('z', 'xcoarse', 'ycoarse')


def test_group_many_to_many_with_extra_outer_dim_uses_optimized_codepath() -> None:
    size = 512
    table = sc.data.table_xyz(int(1e3))
    da = table.bin(z=3, x=size, y=size)
    xcoarse = sc.arange('x', size) // 2
    ycoarse = sc.arange('y', size) // 2
    da.coords['xcoarse'] = xcoarse
    da.coords['ycoarse'] = ycoarse
    # The old "standard" implementation uses a massive amount of memory, crashing even
    # with 256 GByte of RAM.
    assert da.group('xcoarse', 'ycoarse').dims == ('z', 'xcoarse', 'ycoarse')


def test_group_many_to_many_by_two_coords_along_same_dim_uses_optimized_codepath() -> (
    None
):
    size = 512
    table = sc.data.table_xyz(int(1e3))
    da = table.bin(x=size * size)
    x1 = sc.arange('x', size * size) // size
    x2 = sc.arange('x', size * size) % size
    da.coords['x1'] = x1
    da.coords['x2'] = x2
    # The old "standard" implementation uses a massive amount of memory, crashing even
    # with 256 GByte of RAM.
    assert da.group('x1', 'x2').dims == ('x1', 'x2')


def test_group_many_and_erase() -> None:
    from scipp import binning

    binning.make_binned
    size = 512
    table = sc.data.table_xyz(int(1e3))
    da = table.bin(z=3, x=size, y=size)
    xcoarse = sc.arange('x', size) // 2
    ycoarse = sc.arange('y', size) // 2
    da.coords['xcoarse'] = xcoarse
    da.coords['ycoarse'] = ycoarse
    result = binning.make_binned(
        da,
        edges=[],
        groups=[sc.arange('xcoarse', size // 2, dtype=xcoarse.dtype)],
        erase=['x', 'y'],
    )
    assert result.dims == ('z', 'xcoarse')


def test_erase_multiple() -> None:
    from scipp import binning

    binning.make_binned
    table = sc.data.table_xyz(int(1e3))
    da = table.bin(x=13, y=7)
    result = binning.make_binned(da, edges=[], groups=[], erase=['x', 'y'])
    assert sc.identical(result.value, da.bins.constituents['data'])


def test_erase_2d_mask() -> None:
    from scipp import binning

    binning.make_binned
    table = sc.data.table_xyz(100)
    table.data = sc.arange('row', 100)
    da = table.bin(x=2, y=2)
    da.masks['mask'] = sc.zeros(dims=da.dims, shape=da.shape, dtype=bool)
    da.masks['mask'].values[1, 1] = True
    result = binning.make_binned(da, edges=[], groups=[], erase=['x', 'y'])
    assert not sc.identical(result.sum(), table.sum())
    assert sc.identical(result.sum(), da.sum())


def test_erase_multiple_masks() -> None:
    from scipp import binning

    binning.make_binned
    table = sc.data.table_xyz(100)
    table.data = sc.arange('row', 100)
    da = table.bin(x=2, y=2)
    da.masks['xy'] = sc.zeros(dims=da.dims, shape=da.shape, dtype=bool)
    da.masks['xy'].values[1, 1] = True
    da.masks['x'] = sc.array(dims=['x'], values=[False, True])
    result = binning.make_binned(da, edges=[], groups=[], erase=['x', 'y'])
    assert not sc.identical(result.sum(), table.sum())
    assert sc.identical(result.sum(), da.sum())


def _make_base_array():
    nx = 7
    ny = 3
    nz = 13
    sizes = {'x': nx, 'y': ny, 'z': nz}
    nrow = 10000
    table = sc.data.table_xyz(nrow)
    table.data = sc.array(dims=['row'], values=rng.integers(1000000, size=nrow))
    da = table.bin(sizes)
    da.coords['x1'] = sc.arange('x', nx) // 2
    da.coords['x2'] = sc.array(dims=['x'], values=rng.integers(nx // 3, size=nx))
    da.coords['y1'] = sc.arange('y', ny) // 2
    da.coords['y2'] = sc.array(dims=['y'], values=rng.integers(ny // 3, size=ny))
    da.coords['z1'] = sc.arange('z', nz) // 2
    da.coords['z2'] = sc.array(dims=['z'], values=rng.integers(nz // 3, size=nz))
    return da


_base_array = _make_base_array()


def data_array_3d_with_outer_coords(names):
    da = _base_array.copy(deep=False)
    for coord in ['x1', 'x2', 'y1', 'y2', 'z1', 'z2']:
        if coord not in names:
            del da.coords[coord]
    return da


def to_table(da, coord_names):
    da = da.copy()
    for name in coord_names:
        da.bins.coords[name] = sc.bins_like(da, da.coords[name])
    return da.bins.constituents['data']


def cases(x, maxlen):
    out = []
    for i in range(1, maxlen + 1):
        for comb in itertools.combinations(x, i):
            out += list(itertools.permutations(comb))
    return out


params = cases(['x1', 'x2', 'y1', 'y2', 'z1', 'z2'], maxlen=4)


@pytest.mark.parametrize('params', params, ids=['-'.join(g) for g in params])
def test_make_binned_via_group_optimized_path_yields_equivalent_results(params) -> None:
    da = data_array_3d_with_outer_coords(params)

    result = da.group(*params)

    # Compare to directly grouping from table (or binned along unrelated dims). This
    # operates in the underlying events instead of bins.
    expected = to_table(da, params)
    sizes = {dim: size for dim, size in result.sizes.items() if dim in da.sizes}
    if sizes:
        expected = expected.bin(sizes)
    expected = expected.group(*params).hist()
    assert sc.identical(result.hist(), expected)


@pytest.mark.parametrize('params', params, ids=['-'.join(g) for g in params])
def test_make_binned_via_bin_optimized_path_yields_equivalent_results(params) -> None:
    da = data_array_3d_with_outer_coords(params)

    binning = {param: rng.integers(low=1, high=5) for param in params}
    result = da.bin(binning)

    # Compare to directly binning from table (or binned along unrelated dims). This
    # operates in the underlying events instead of bins.
    expected = to_table(da, params)
    sizes = {dim: size for dim, size in result.sizes.items() if dim in da.sizes}
    if sizes:
        expected = expected.bin(sizes)
    expected = expected.bin(binning).hist()
    assert sc.identical(result.hist(), expected)


def test_bin_linspace_handles_large_positive_values_correctly() -> None:
    table = sc.data.table_xyz(10)
    table.coords['x'].values[0] = 1e20
    x = sc.linspace('x', 0.0, 1.0, 3, unit='m', dtype='float64')
    da = table.bin(x=x)
    assert sc.identical(da, table[1:].bin(x=x))
    assert da.bins.size().sum().value == 9


def test_bin_linspace_handles_large_negative_values_correctly() -> None:
    table = sc.data.table_xyz(10)
    table.coords['x'].values[0] = -1e20
    x = sc.linspace('x', 0.0, 1.0, 3, unit='m', dtype='float64')
    da = table.bin(x=x)
    assert sc.identical(da, table[1:].bin(x=x))
    assert da.bins.size().sum().value == 9


def test_hist_linspace_handles_large_positive_values_correctly() -> None:
    table = sc.data.table_xyz(10)
    table.values[...] = 1.0
    table.coords['x'].values[0] = 1e20
    x = sc.linspace('x', 0.0, 1.0, 3, unit='m', dtype='float64')
    da = table.hist(x=x)
    assert sc.identical(da, table[1:].hist(x=x))
    assert da.sum().value == 9


def test_hist_linspace_handles_large_negative_values_correctly() -> None:
    table = sc.data.table_xyz(10)
    table.values[...] = 1.0
    table.coords['x'].values[0] = -1e20
    x = sc.linspace('x', 0.0, 1.0, 3, unit='m', dtype='float64')
    da = table.hist(x=x)
    assert sc.identical(da, table[1:].hist(x=x))
    assert da.sum().value == 9


def test_group_with_explicit_lower_precision_drops_rows_outside_domain() -> None:
    table = sc.data.table_xyz(100)
    table.coords['label'] = (table.coords['x'] * 10).to(dtype='int64')
    table.coords['label'].values[0] = 0
    da = table.group(sc.arange('label', 5, unit='m', dtype='int32'))
    size0 = da.bins.size()['label', 0].value
    size = da.bins.size().sum().value
    table.coords['label'].values[0] = np.iinfo(np.int32).max + 100
    da = table.group(sc.arange('label', 5, unit='m', dtype='int32'))
    reference = table[1:].group(sc.arange('label', 5, unit='m', dtype='int32'))
    assert sc.identical(da, reference)
    assert da.bins.size()['label', 0].value == size0 - 1
    assert da.bins.size().sum().value == size - 1


def test_bin_with_explicit_lower_precision_drops_rows_outside_domain() -> None:
    table = sc.data.table_xyz(100)
    x = sc.linspace('x', 0.0, 1.0, 3, unit='m', dtype='float32')
    da = table.bin(x=x)
    size = da.bins.size().sum().value
    table.coords['x'].values[0] = np.float64(2.0) * np.finfo(np.float32).max
    da = table.bin(x=x)
    reference = table[1:].bin(x=x)
    assert sc.identical(da, reference)
    assert da.bins.size().sum().value == size - 1


def test_hist_large_input_sections() -> None:
    table = sc.data.table_xyz(1_000_000)
    table.data = (table.data * 1000).to(dtype='int64').to(dtype='float64')
    hist1 = table.hist(x=127)
    edges = hist1.coords['x']
    for pivot in [0, 17, 3333, 50_000, 123_456, 654_321, 999_999]:
        hist2 = table[:pivot].hist(x=edges) + table[pivot:].hist(x=edges)
        assert sc.identical(hist1, hist2)


def make_groupable(ngroup: int) -> sc.DataArray:
    label = sc.concat(
        [
            sc.arange('row', ngroup - 1, -1, -1, dtype='int64'),
            sc.arange('row', ngroup, dtype='int64'),
        ],
        'row',
    )
    return sc.DataArray(label, coords={'label': label, 'other': label})


@pytest.mark.parametrize('ngroup', [0, 1, 17, 23_434, 102_111, 2_003_045])
def test_group_many_groups(ngroup) -> None:
    table = make_groupable(ngroup)

    grouped = table.group('label')
    expected = table[ngroup:].copy()
    del expected.coords['label']
    assert_identical(grouped.bins.constituents['data'][::2], expected)
    assert_identical(grouped.bins.constituents['data'][1::2], expected)

    reversed_labels = table.coords['label'][:ngroup].rename(row='label')
    grouped = table.group(reversed_labels)
    expected = table[:ngroup].copy()
    del expected.coords['label']
    assert_identical(grouped.bins.constituents['data'][::2], expected)
    assert_identical(grouped.bins.constituents['data'][1::2], expected)

    incomplete_labels = table.coords['label'][: ngroup // 2].rename(row='label')
    grouped = table.group(incomplete_labels)
    expected = table[: ngroup // 2].copy()
    del expected.coords['label']
    assert_identical(grouped.bins.constituents['data'][::2], expected)
    assert_identical(grouped.bins.constituents['data'][1::2], expected)


def test_hist_edges_referencing_original_variable() -> None:
    table = sc.data.table_xyz(100)
    edges = sc.linspace('x', 0, 1, 11, unit='m')
    histogrammed = table.hist(x=edges)
    doubled_edges = edges * 2
    assert not sc.identical(histogrammed.coords['x'], doubled_edges)
    edges *= 2
    assert_identical(histogrammed.coords['x'], doubled_edges)


def test_bin_edges_referencing_original_variable() -> None:
    table = sc.data.table_xyz(100)
    edges = sc.linspace('x', 0, 1, 11, unit='m')
    binned = table.bin(x=edges)
    doubled_edges = edges * 2
    assert not sc.identical(binned.coords['x'], doubled_edges)
    edges *= 2
    assert_identical(binned.coords['x'], doubled_edges)


def test_rebin_edges_referencing_original_variable() -> None:
    table = sc.data.table_xyz(100)
    edges = sc.linspace('x', 0, 1, 11, unit='m')
    smaller_edges = sc.linspace('x', 0, 1, 101, unit='m')
    rebinned = table.bin(x=edges).bin(x=smaller_edges)
    doubled_edges = smaller_edges * 2
    assert not sc.identical(rebinned.coords['x'], doubled_edges)
    smaller_edges *= 2
    assert_identical(rebinned.coords['x'], doubled_edges)


def test_rebin_multi_dim_edges_referencing_original_variable() -> None:
    table = sc.data.table_xyz(100)
    x_edges = sc.linspace('x', 0, 1, 11, unit='m')
    y_edges = sc.linspace('y', 0, 1, 11, unit='m')
    rebinned = table.bin(x=x_edges).bin(y=y_edges)
    doubled_x_edges = x_edges * 2
    doubled_y_edges = y_edges * 2
    assert not sc.identical(rebinned.coords['x'], doubled_x_edges)
    assert not sc.identical(rebinned.coords['y'], doubled_y_edges)
    x_edges *= 2
    y_edges *= 2
    assert_identical(rebinned.coords['x'], doubled_x_edges)
    assert_identical(rebinned.coords['y'], doubled_y_edges)


def test_group_edges_referencing_original_variable() -> None:
    table = sc.data.table_xyz(100)
    groups = sc.linspace('x', 0, 1, 10, unit='m')
    grouped = table.group(groups)
    doubled_groups = groups * 2
    assert not sc.identical(grouped.coords['x'], doubled_groups)
    groups *= 2
    assert_identical(grouped.coords['x'], doubled_groups)


def test_group_binned_edges_referencing_original_variable() -> None:
    table = sc.data.table_xyz(100)
    edges = sc.linspace('x', 0, 1, 11, unit='m')
    groups = sc.linspace('y', 0, 1, 10, unit='m')
    grouped = table.bin(x=edges).group(groups)
    doubled_edges = edges * 2
    doubled_groups = groups * 2
    assert not sc.identical(grouped.coords['x'], doubled_edges)
    assert not sc.identical(grouped.coords['y'], doubled_groups)
    edges *= 2
    groups *= 2
    assert_identical(grouped.coords['x'], doubled_edges)
    assert_identical(grouped.coords['y'], doubled_groups)


@pytest.fixture
def noncontiguous_var() -> sc.Variable:
    var = sc.linspace('x', 0, 1, 101, unit='m')[::2]
    assert not var.values.data.contiguous
    return var


@pytest.fixture
def contiguous_var(noncontiguous_var: sc.Variable) -> sc.Variable:
    var = noncontiguous_var.copy()
    assert var.values.data.contiguous
    return var


def test_noncontiguous_binning(
    noncontiguous_var: sc.Variable, contiguous_var: sc.Variable
):
    table = sc.data.table_xyz(1_000)
    assert sc.identical(table.bin(x=noncontiguous_var), table.bin(x=contiguous_var))


def test_noncontiguous_histogramming(
    noncontiguous_var: sc.Variable, contiguous_var: sc.Variable
):
    table = sc.data.table_xyz(1_000)
    assert_identical(table.hist(x=noncontiguous_var), table.hist(x=contiguous_var))


def test_noncontiguous_grouping(
    noncontiguous_var: sc.Variable, contiguous_var: sc.Variable
):
    table = sc.data.table_xyz(1_000)
    assert_identical(table.group(noncontiguous_var), table.group(contiguous_var))


def test_noncontiguous_int_grouping() -> None:
    table = sc.data.table_xyz(1_000)
    table.coords['idx'] = sc.arange(dim='row', start=0, stop=1000, step=1, dtype=int)

    nonnontiguous_idx = sc.arange(dim='idx', start=0, stop=1000, step=1, dtype=int)[
        ::10
    ]
    contiguous_idx = sc.arange(dim='idx', start=0, stop=1000, step=10, dtype=int)
    assert not nonnontiguous_idx.values.data.contiguous
    assert contiguous_idx.values.data.contiguous

    assert_identical(table.group(nonnontiguous_idx), table.group(contiguous_idx))


def test_group_automatic_groups_works_with_string_coord() -> None:
    keys = sc.array(dims=['row'], values=['a', 'b', 'c', 'a', 'b', 'c'])
    table = sc.DataArray(sc.ones(dims=['row'], shape=[6]), coords={'key': keys})
    assert sc.identical(
        table.group('key').coords['key'], sc.array(dims=['key'], values=['a', 'b', 'c'])
    )


def test_explicit_groups_with_mismatching_dtype() -> None:
    label = sc.arange('row', 10, dtype='int32')
    table = sc.DataArray(sc.ones(dims=['row'], shape=[10]), coords={'label': label})
    groups = sc.arange('label', 8, dtype='int64')
    assert sc.identical(table.group(groups).coords['label'], groups)
    # Access different code branch, triggering when groups are not contiguous
    groups[-1] += 1
    assert sc.identical(table.group(groups).coords['label'], groups)


def test_hist_dense_with_explicit_dim_arg_yields_expected_output_dims() -> None:
    xy = sc.data.table_xyz(1000).bin(x=4, y=6)
    xy.coords['z'] = xy.bins.coords['z'].bins.min()
    xy = xy.hist()

    assert xy.hist(z=4, dim=xy.dims).dims == ('z',)


def test_hist_dense_3d_with_explicit_dim_arg_yields_expected_output_dims() -> None:
    xyz = sc.data.table_xyz(1000).bin(x=3, y=4, z=5)
    xyz.data = (xyz.data * 1000).to(dtype='int64')
    xyz.coords['t'] = xyz.bins.coords['z'].bins.mean()
    xyz = xyz.hist()

    assert_identical(xyz.hist(t=4, dim=xyz.dims), xyz.flatten(to='t').hist(t=4))
    assert xyz.hist(t=4, dim=xyz.dims).dims == ('t',)
    assert xyz.hist(t=4, dim=('x', 'y')).dims == ('z', 't')
    assert xyz.hist(t=4, dim=('x', 'z')).dims == ('y', 't')
    assert xyz.hist(t=4, dim=('y', 'z')).dims == ('x', 't')
    assert xyz.hist(t=4, dim=('x', 'z', 'y')).dims == ('t',)  # order ignored
    assert xyz.hist(t=4, dim='x').dims == ('y', 'z', 't')


def test_hist_binned_3d_with_explicit_dim_arg_yields_expected_output_dims() -> None:
    xyz = sc.data.table_xyz(1000).bin(x=3, y=4, z=5)
    xyz.data = (xyz.data * 1000).to(dtype='int64')
    xyz.coords['t'] = xyz.bins.coords['z'].bins.mean()

    assert_identical(xyz.hist(t=4, dim=xyz.dims), xyz.flatten(to='t').hist(t=4))
    assert xyz.hist(t=4, dim=xyz.dims).dims == ('t',)
    assert xyz.hist(t=4, dim=('x', 'y')).dims == ('z', 't')
    assert xyz.hist(t=4, dim=('x', 'z')).dims == ('y', 't')
    assert xyz.hist(t=4, dim=('y', 'z')).dims == ('x', 't')
    assert xyz.hist(t=4, dim=('x', 'z', 'y')).dims == ('t',)  # order ignored
    assert xyz.hist(t=4, dim='x').dims == ('y', 'z', 't')


def test_hist_3d_binned_coord_with_explicit_dim_arg_yields_expected_output_dims() -> (
    None
):
    xyz = sc.data.table_xyz(1000).bin(x=3, y=4, z=5)
    xyz.data = (xyz.data * 1000).to(dtype='int64')
    xyz.bins.coords['t'] = xyz.bins.coords['z']

    assert_identical(xyz.hist(t=4, dim=xyz.dims), xyz.flatten(to='t').hist(t=4))
    assert xyz.hist(t=4, dim=xyz.dims).dims == ('t',)
    assert xyz.hist(t=4, dim=('x', 'y')).dims == ('z', 't')
    assert xyz.hist(t=4, dim=('x', 'z')).dims == ('y', 't')
    assert xyz.hist(t=4, dim=('y', 'z')).dims == ('x', 't')
    assert xyz.hist(t=4, dim=('x', 'z', 'y')).dims == ('t',)  # order ignored
    assert xyz.hist(t=4, dim='x').dims == ('y', 'z', 't')


def test_hist_by_lower_dim_coord_operates_on_slices() -> None:
    xyz = sc.data.table_xyz(1000).bin(x=3, y=4, z=5)
    xyz.coords['t'] = sc.linspace('y', 0, 1, 4, unit='s')

    assert xyz.hist(t=3).dims == ('x', 'z', 't')
    assert xyz.hist().hist(t=3).dims == ('x', 'z', 't')


def test_hist_by_two_lower_dim_coords_operates_on_slices() -> None:
    xyz = sc.data.table_xyz(1000).bin(x=3, y=4, z=5)
    xyz.coords['t1'] = sc.linspace('y', 0, 1, 4, unit='s')
    xyz.coords['t2'] = sc.linspace('z', 0, 1, 5, unit='s')

    assert xyz.hist(t1=3, t2=3).dims == ('x', 't1', 't2')
    assert xyz.hist().hist(t1=3, t2=3).dims == ('x', 't1', 't2')


def test_raises_if_explicit_dim_arg_clashes_with_edge_dims() -> None:
    xyz = sc.data.table_xyz(1000).bin(x=3, y=4, z=5)
    x = sc.linspace('x', 0, 1, 10, unit='m').broadcast(sizes={'y': 4, 'x': 10})
    with pytest.raises(ValueError, match='Clash of dimension\\(s\\) to reduce'):
        xyz.bin(x=x, dim=('y', 'z'))
    with pytest.raises(ValueError, match='Clash of dimension\\(s\\) to reduce'):
        xyz.hist(x=x, dim=('y', 'z'))
    with pytest.raises(ValueError, match='Clash of dimension\\(s\\) to reduce'):
        xyz.nanhist(x=x, dim=('y', 'z'))


@pytest.mark.parametrize(
    'z',
    [
        None,
        sc.linspace('x', 0, 1, 4, unit='m'),
        sc.linspace('y', 0, 1, 6, unit='m'),
        sc.linspace('x', 0, 1, 4, unit='m') + sc.linspace('y', 0, 1, 6, unit='m'),
    ],
)
def test_op_on_binned_with_explicit_dim_arg_yields_expected_output_dims(z) -> None:
    xy = sc.data.table_xyz(1000).bin(x=4, y=6)
    if z is not None:
        # Presence of z coord should not affect result if `dim` is explicit
        xy.coords['z'] = z
    assert xy.bin(z=4, dim=xy.dims).dims == ('z',)
    assert xy.bin(z=4, dim='y').dims == ('x', 'z')
    assert xy.bin(z=4, dim=()).dims == ('x', 'y', 'z')
    assert xy.group('z', dim=xy.dims).dims == ('z',)
    assert xy.group('z', dim='y').dims == ('x', 'z')
    assert xy.group('z', dim=()).dims == ('x', 'y', 'z')
    assert xy.hist(z=4, dim=xy.dims).dims == ('z',)
    assert xy.hist(z=4, dim='y').dims == ('x', 'z')
    assert xy.hist(z=4, dim=()).dims == ('x', 'y', 'z')
    assert xy.nanhist(z=4, dim=xy.dims).dims == ('z',)
    assert xy.nanhist(z=4, dim='y').dims == ('x', 'z')
    assert xy.nanhist(z=4, dim=()).dims == ('x', 'y', 'z')


@pytest.mark.parametrize('op', [sc.bin, sc.hist, sc.nanhist])
def test_op_with_explicit_dim_arg_keeps_aux_coord(op) -> None:
    data = sc.ones(dims=['xyz'], shape=(60,))
    da = sc.DataArray(data)
    da.coords['u'] = sc.sin(sc.linspace('xyz', 0.0, 2.0, num=60, unit='rad'))
    da.coords['v'] = sc.cos(sc.linspace('xyz', 0.0, 2.0, num=60, unit='rad'))
    da.coords['aux'] = sc.cos(sc.linspace('xyz', 0.0, 2.0, num=60, unit='rad'))
    da = da.fold(dim='xyz', sizes={'x': 3, 'y': 4, 'z': 5})
    da.coords['x'] = sc.linspace(dim='x', start=0.0, stop=1.0, num=3)
    result = op(da, u=2, v=2, dim=('y', 'z'))
    assert sc.identical(result.coords['x'], da.coords['x'])


@pytest.mark.parametrize(
    'z_coord',
    [
        None,
        sc.linspace('x', 0, 1, 4, unit='m'),
        sc.linspace('y', 0, 1, 6, unit='m'),
        sc.linspace('x', 0, 1, 4, unit='m') + sc.linspace('y', 0, 1, 6, unit='m'),
    ],
)
def test_bin_binned_with_explicit_dim_arg_equivalent_to_manual_concat_and_bin(
    z_coord,
) -> None:
    xy = sc.data.table_xyz(1000).bin(x=4, y=6)
    if z_coord is not None:
        # Presence of z coord should not affect result if `dim` is explicit
        xy.coords['z'] = z_coord
    z = sc.linspace('z', 0, 1, 4, unit='m')
    assert_identical(xy.bin(z=z, dim=xy.dims), xy.bins.concat().bin(z=z))


@pytest.mark.parametrize(
    'z_coord',
    [
        None,
        sc.linspace('y', 0, 1, 6, unit='m'),
        sc.linspace('x', 0, 1, 4, unit='m') + sc.linspace('y', 0, 1, 6, unit='m'),
    ],
)
def test_bin_binned_with_explicit_dim_arg_equivalent_to_manual_rename_dims_and_bin(
    z_coord,
):
    xy = sc.data.table_xyz(1000).bin(x=4, y=6)
    if z_coord is not None:
        # Presence of z coord should not affect result if `dim` is explicit
        xy.coords['z'] = z_coord
    z = sc.linspace('z', 0, 1, 4, unit='m')
    drop = 'z' if z_coord is not None else ()
    # For the reference implementation we drop the coord, since those bin edges are
    # "lying", leading scipp to drop rows. Note that this is explicitly documented as
    # unspecified behavior, so the reference implementation is not necessarily wrong.
    # Without renaming the dimension we do not hit that code path, so the behavior is
    # different.
    assert_identical(
        xy.bin(z=z, dim='y'), xy.drop_coords(drop).rename_dims(y='z').bin(z=z, dim=())
    )


def test_bin_rebin_existing_edge_cases() -> None:
    n_event = 4
    events = sc.DataArray(
        sc.ones(sizes={"e": n_event}), coords={'x': sc.linspace('e', 0, 1, num=n_event)}
    )
    binned = events.bin(x=2)

    # Bins after existing end
    result = binned.bin(x=sc.linspace('x', 0.0, 2.0, num=10))
    assert_identical(
        result.bins.constituents['begin'],
        sc.array(dims=['x'], values=[0, 1, 2, 2, 3, 4, 4, 4, 4], unit=None),
    )

    # Bins before existing start
    result = binned.bin(x=sc.linspace('x', -1.0, 1.0, num=10))
    assert_identical(
        result.bins.constituents['begin'],
        sc.array(dims=['x'], values=[0, 0, 0, 0, 0, 1, 1, 2, 3], unit=None),
    )
