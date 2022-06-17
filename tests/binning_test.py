# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import pytest
import scipp as sc
import numpy as np
from numpy.random import default_rng


@pytest.mark.parametrize('op', ['bin', 'hist', 'nanhist', 'rebin'])
def test_raises_CoordError_if_coord_not_found(op):
    table = sc.data.table_xyz(100)
    with pytest.raises(sc.CoordError):
        getattr(table, op)(abc=5)


def test_group_raises_CoordError_if_coord_not_found():
    table = sc.data.table_xyz(100)
    with pytest.raises(sc.CoordError):
        table.group('abc')


def test_many_combinations():
    table = sc.data.table_xyz(100)
    table.coords['label'] = (table.coords['x'] * 10).to(dtype='int64')
    table.group('label').hist(x=5, y=3)
    table.group('label').hist(x=5)
    table.group('label').hist()
    table.hist(x=5, y=3)
    table.hist(x=5)
    table.hist(x=5).rebin(x=3)
    table.hist(x=5, y=3).rebin(x=3, y=2)
    table.bin(x=5).bin(x=6).bin(y=6)
    table.bin(x=5).bin(y=6, x=7)
    table.bin(x=5).hist()
    table.bin(x=5).hist(x=7)
    table.bin(x=5).hist(y=7)
    table.bin(x=5).hist(x=3, y=7)
    table.bin(x=5).group('label').hist()
    table.bin(x=5).group('label').hist(y=5)


def test_hist_table_define_edges_from_bin_count():
    da = sc.data.table_xyz(100)
    histogrammed = da.hist(y=4)
    edges = histogrammed.coords['y']
    assert len(edges) == 5
    assert histogrammed.sizes['y'] == 4
    assert edges.min().value == da.coords['y'].min().value
    assert edges.max().value > da.coords['y'].max().value
    assert edges.max().value == np.nextafter(da.coords['y'].max().value, np.inf)


def test_hist_binned_define_edges_from_bin_count():
    da = sc.data.binned_x(100, 10)
    histogrammed = da.hist(y=4)
    edges = histogrammed.coords['y']
    assert len(edges) == 5
    assert histogrammed.sizes['y'] == 4
    assert edges.min().value == da.bins.coords['y'].min().value
    assert edges.max().value > da.bins.coords['y'].max().value
    assert edges.max().value == np.nextafter(da.bins.coords['y'].max().value, np.inf)


def test_hist_binned_no_additional_edges():
    da = sc.data.binned_x(100, 10)
    assert sc.identical(da.hist(), da.bins.sum())


def test_hist_table_define_edges_from_bin_size():
    da = sc.data.table_xyz(100)
    histogrammed = da.hist(y=sc.scalar(100, unit='mm'))
    edges = histogrammed.coords['y']
    assert len(edges) == 11
    assert histogrammed.sizes['y'] == 10
    assert edges.min().value == da.coords['y'].min().value
    assert edges.max().value > da.coords['y'].max().value


def test_hist_binned_define_edges_from_bin_size():
    da = sc.data.binned_x(100, 10)
    histogrammed = da.hist(y=sc.scalar(100, unit='mm'))
    edges = histogrammed.coords['y']
    assert len(edges) == 11
    assert histogrammed.sizes['y'] == 10
    assert edges.min().value == da.bins.coords['y'].min().value
    assert edges.max().value > da.bins.coords['y'].max().value


def test_hist_table_custom_edges():
    da = sc.data.table_xyz(100)
    y = sc.linspace('y', 0.2, 0.6, num=3, unit='m')
    histogrammed = da.hist(y=y)
    assert sc.identical(histogrammed.coords['y'], y)


def test_hist_binned_custom_edges():
    da = sc.data.binned_x(100, 10)
    y = sc.linspace('y', 0.2, 0.6, num=3, unit='m')
    histogrammed = da.hist(y=y)
    assert sc.identical(histogrammed.coords['y'], y)


def test_hist_x_and_edges_arg_are_position_only_and_are_ok_as_keyword_args():
    da = sc.data.table_xyz(100)
    da.coords['edges'] = da.coords['x']
    da.hist(x=4)
    da.hist(edges=4)


def test_hist_raises_if_edges_specified_positional_and_as_kwarg():
    da = sc.data.table_xyz(100)
    with pytest.raises(TypeError):
        da.hist({'y': 4}, y=4)


def test_hist_edges_from_positional_arg():
    da = sc.data.table_xyz(100)
    assert sc.identical(da.hist({'y': 4}), da.hist(y=4))


@pytest.mark.parametrize('dtype', ['int32', 'int64'])
def test_bin_integer_coord_by_int(dtype):
    table = sc.data.table_xyz(100)
    table.coords['label'] = (table.coords['x'] * 10).to(dtype=dtype)
    coord = table.bin(label=2).coords['label']
    expected = sc.array(dims=['label'], unit='m', dtype=dtype, values=[0, 5, 10])
    assert sc.identical(coord, expected)


@pytest.mark.parametrize('dtype', ['int32', 'int64'])
def test_bin_integer_coord_by_stepsize(dtype):
    table = sc.data.table_xyz(100)
    table.coords['label'] = (table.coords['x'] * 10).to(dtype=dtype)
    coord = table.bin(label=sc.scalar(5, unit='m')).coords['label']
    expected = sc.array(dims=['label'], unit='m', dtype=dtype, values=[0, 5, 10])
    assert sc.identical(coord, expected)


@pytest.mark.parametrize('dtype', ['int32', 'int64'])
def test_bin_integer_coord_by_stepsize_in_different_unit(dtype):
    table = sc.data.table_xyz(100)
    table.coords['label'] = (table.coords['x'] * 10).to(dtype=dtype)
    coord = table.bin(label=sc.scalar(500, unit='cm')).coords['label']
    expected = sc.array(dims=['label'], unit='m', dtype=dtype, values=[0, 5, 10])
    assert sc.identical(coord, expected)


@pytest.mark.parametrize('dtype', ['int32', 'int64'])
def test_bin_integer_coord_by_float_stepsize(dtype):
    table = sc.data.table_xyz(100)
    table.coords['label'] = (table.coords['x'] * 10).to(dtype=dtype)
    coord = table.bin(label=sc.scalar(3.3, unit='m')).coords['label']
    expected = sc.array(dims=['label'], unit='m', dtype=dtype, values=[0, 3, 6, 9, 12])
    assert sc.identical(coord, expected)


@pytest.mark.parametrize('dtype', ['int32', 'int64'])
def test_bin_integer_coord_by_fractional_stepsize_raises(dtype):
    table = sc.data.table_xyz(100)
    table.coords['label'] = (table.coords['x'] * 10).to(dtype=dtype)
    with pytest.raises(ZeroDivisionError):
        table.bin(label=sc.scalar(0.5, unit='m')).coords['label']


def test_group_after_bin_considers_event_value():
    table = sc.data.table_xyz(100)
    table.coords['label'] = (table.coords['x'] * 10).to(dtype='int64')
    da = table.bin(label=2).group('label')
    assert da.sizes == {'label': 10}


def test_bin_erases_dims_automatically_if_labels_for_same_dim():
    table = sc.data.table_xyz(100)
    table.coords['x2'] = table.coords['x'] * 2
    da = table.bin(x=10)
    assert da.bin(x2=100).dims == ('x', 'x2')
    # With a bin-coord for x2 we can deduce that the new binning is by different
    # 'labels' for the same data dimension.
    da.coords['x2'] = da.coords['x'] * 2
    assert da.bin(x2=100).dims == ('x2', )


def test_bin_by_two_coords_depending_on_same_dim():
    table = sc.data.table_xyz(100)
    da = table.bin(x=10, y=12)
    assert da.dims == ('x', 'y')


def test_bin_by_2d_erases_2_input_dims():
    table = sc.data.table_xyz(100)
    table.coords['xy'] = table.coords['x'] + table.coords['y']
    da = table.bin(x=10, y=12)
    # Note: These are bin edges, but `bin` still works
    da.coords['xy'] = da.coords['x'][1:] + da.coords['y']
    assert da.bin(xy=20).dims == ('xy', )


def test_bin_by_2d_dimension_coord_does_not_erase_extra_dim():
    table = sc.data.table_xyz(100)
    table.coords['xy'] = table.coords['x'] + table.coords['y']
    da = table.bin(x=10, y=12)
    da.coords['xy'] = da.coords['x'][1:] + da.coords['y']
    da = da.rename_dims({'y': 'xy'})
    # The call to `bin` here "aligns" all bins, so we generally do not want ot erase.
    assert da.bin(xy=20).dims == ('x', 'xy')


def test_hist_on_dense_data_without_edges_raisesTypeError():
    table = sc.data.table_xyz(10)
    with pytest.raises(TypeError):
        table.hist()
    with pytest.raises(TypeError):
        table.nanhist()


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


def test_bin_without_coord_keeps_dim():
    da = date_month_day_table_grouped_by_date()
    # Binning is by event-coord
    assert da.bin(month=12).dims == ('date', 'month')


def test_bin_with_1d_dimcoord_keeps_dim():
    da = date_month_day_table_grouped_by_date()
    assert da.bin(date=4).dims == ('date', )


def test_bin_with_2d_dimcoord_keeps_dims():
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    da.coords['day'] = da.coords['date'] % 30
    da2d = da.group('month', 'day')
    da2d.coords['date'] = da2d.coords['day'] + 30 * da2d.coords['month']
    da2d = da2d.rename_dims({'month': 'date'})
    assert da2d.bin(date=4).dims == ('date', 'day')


def test_bin_with_nondimcoord_removes_dim():
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    assert da.bin(month=12).dims == ('month', )


def test_bin_with_multiple_nondimcoords_removes_dim():
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    da.coords['day'] = da.coords['date'] % 30
    assert da.bin(month=12, day=30).dims == ('month', 'day')


def test_bin_with_dimcoord_and_nondimcoord_keeps_dim():
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    # 'date' exists, so new coord is inner dim
    assert da.bin(month=12, date=4).dims == ('date', 'month')


def test_bin_with_nondimcoord_removes_multiple_input_dims():
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    da.coords['day'] = da.coords['date'] % 30
    da2d = da.group('month', 'day')
    da2d.coords['date'] = da2d.coords['day'] + 30 * da2d.coords['month']
    assert da2d.bin(date=4).dims == ('date', )


# 2. group


def test_group_without_coord_keeps_dim():
    da = date_month_day_table_grouped_by_date()
    # Grouping is by event-coord
    assert da.group('month').dims == ('date', 'month')


def test_group_with_1d_dimcoord_keeps_dim():
    da = date_month_day_table_grouped_by_date()
    assert da.group(sc.array(dims=['date'], values=[1, 3, 5],
                             dtype='int64')).dims == ('date', )


def test_group_with_2d_dimcoord_keeps_dims():
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    da.coords['day'] = da.coords['date'] % 30
    da2d = da.group('month', 'day')
    da2d.coords['date'] = da2d.coords['day'] + 30 * da2d.coords['month']
    da2d = da2d.rename_dims({'month': 'date'})
    assert da2d.group('date').dims == ('date', 'day')


def test_group_with_nondimcoord_removes_dim():
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    assert da.group('month').dims == ('month', )


def test_group_with_multiple_nondimcoords_removes_dim():
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    da.coords['day'] = da.coords['date'] % 30
    assert da.group('month', 'day').dims == ('month', 'day')


def test_group_with_dimcoord_and_nondimcoord_keeps_dim():
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    # 'date' exists, so new coord is inner dim
    assert da.group('month', 'date').dims == ('date', 'month')


def test_group_with_nondimcoord_removes_multiple_input_dims():
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    da.coords['day'] = da.coords['date'] % 30
    da2d = da.group('month', 'day')
    da2d.coords['date'] = da2d.coords['day'] + 30 * da2d.coords['month']
    assert da2d.group('date').dims == ('date', )


# 3. hist


def test_hist_without_coord_keeps_dim():
    da = date_month_day_table_grouped_by_date()
    # Histogramming is by event-coord
    assert da.hist(month=12).dims == ('date', 'month')


def test_hist_with_1d_dimcoord_keeps_dim():
    da = date_month_day_table_grouped_by_date()
    assert da.hist(date=4).dims == ('date', )


def test_hist_with_2d_dimcoord_keeps_dims():
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    da.coords['day'] = da.coords['date'] % 30
    da2d = da.group('month', 'day')
    da2d.coords['date'] = da2d.coords['day'] + 30 * da2d.coords['month']
    da2d = da2d.rename_dims({'month': 'date'})
    # Note current slightly inconsistent behavior by `histogram`: The histogrammed dim
    # alsways turns into the inner dimension.
    assert da2d.hist(date=4).dims == ('day', 'date')


def test_hist_with_nondimcoord_removes_dim():
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    assert da.hist(month=12).dims == ('month', )


def test_hist_with_multiple_nondimcoords_removes_dim():
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    da.coords['day'] = da.coords['date'] % 30
    assert da.hist(month=12, day=30).dims == ('month', 'day')


def test_hist_with_dimcoord_and_nondimcoord_keeps_dim():
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    assert da.hist(month=12, date=4).dims == ('month', 'date')


def test_hist_with_nondimcoord_removes_multiple_input_dims():
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    da.coords['day'] = da.coords['date'] % 30
    da2d = da.group('month', 'day')
    da2d.coords['date'] = da2d.coords['day'] + 30 * da2d.coords['month']
    assert da2d.hist(date=4).dims == ('date', )


# 4. nanhist


def test_nanhist_without_coord_keeps_dim():
    da = date_month_day_table_grouped_by_date()
    # Histogramming is by event-coord
    assert da.nanhist(month=12).dims == ('date', 'month')


def test_nanhist_with_1d_dimcoord_keeps_dim():
    da = date_month_day_table_grouped_by_date()
    assert da.nanhist(date=4).dims == ('date', )


def test_nanhist_with_2d_dimcoord_keeps_dims():
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    da.coords['day'] = da.coords['date'] % 30
    da2d = da.group('month', 'day')
    da2d.coords['date'] = da2d.coords['day'] + 30 * da2d.coords['month']
    da2d = da2d.rename_dims({'month': 'date'})
    assert da2d.nanhist(date=4).dims == ('date', 'day')


def test_nanhist_with_nondimcoord_removes_dim():
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    assert da.nanhist(month=12).dims == ('month', )


def test_nanhist_with_multiple_nondimcoords_removes_dim():
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    da.coords['day'] = da.coords['date'] % 30
    assert da.nanhist(month=12, day=30).dims == ('month', 'day')


def test_nanhist_with_dimcoord_and_nondimcoord_keeps_dim():
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    assert da.nanhist(month=12, date=4).dims == ('date', 'month')


def test_nanhist_with_nondimcoord_removes_multiple_input_dims():
    da = date_month_day_table_grouped_by_date()
    da.coords['month'] = da.coords['date'] // 30
    da.coords['day'] = da.coords['date'] % 30
    da2d = da.group('month', 'day')
    da2d.coords['date'] = da2d.coords['day'] + 30 * da2d.coords['month']
    assert da2d.nanhist(date=4).dims == ('date', )


def test_binning_low_level_functions_exist():
    from scipp import binning
    binning.make_binned
    binning.make_histogrammed


def test_histogram_deprecated_name():
    da = sc.data.table_xyz(100)
    x = sc.linspace('x', 0, 1, num=10, unit='m')
    with pytest.warns(UserWarning):
        result = sc.histogram(da, bins=x)
    assert sc.identical(result, da.hist(x=x))


def test_bin_deprecated_arguments():
    da = sc.data.table_xyz(100)
    x = sc.linspace('x', 0, 1, num=10, unit='m')
    with pytest.warns(UserWarning):
        result = sc.bin(da, edges=[x])
    assert sc.identical(result, da.bin(x=x))


def test_rebin_deprecated_keyword_arguments():
    da = sc.data.table_xyz(100).hist(x=100)
    x = sc.linspace('x', 0, 1, num=10, unit='m')
    with pytest.warns(UserWarning):
        result = sc.rebin(da, 'x', bins=x)
    assert sc.identical(result, da.rebin(x=x))


def test_rebin_deprecated_positional_arguments():
    da = sc.data.table_xyz(100).hist(x=100)
    x = sc.linspace('x', 0, 1, num=10, unit='m')
    with pytest.warns(UserWarning):
        result = sc.rebin(da, 'x', x)
    assert sc.identical(result, da.rebin(x=x))
