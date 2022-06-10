# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import pytest
import scipp as sc
import numpy as np


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
    assert edges.min().value == da.coords['y'].min().value
    assert edges.max().value > da.coords['y'].max().value
    assert edges.max().value == np.nextafter(da.coords['y'].max().value, np.inf)


def test_hist_binned_define_edges_from_bin_count():
    da = sc.data.binned_x(100, 10)
    histogrammed = da.hist(y=4)
    edges = histogrammed.coords['y']
    assert len(edges) == 5
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
    assert edges.min().value == da.coords['y'].min().value
    assert edges.max().value > da.coords['y'].max().value


def test_hist_binned_define_edges_from_bin_size():
    da = sc.data.binned_x(100, 10)
    histogrammed = da.hist(y=sc.scalar(100, unit='mm'))
    edges = histogrammed.coords['y']
    assert len(edges) == 11
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


def test_group_after_bin():
    table = sc.data.table_xyz(100)
    table.coords['label'] = (table.coords['x'] * 10).to(dtype='int64')
    table.bin(label=2).group('label')
