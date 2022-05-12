# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen
import numpy as np
import pytest

import scipp as sc


@pytest.fixture(
    params=[lambda x: x, lambda x: sc.DataArray(x), lambda x: sc.Dataset({'a': x})],
    ids=['Variable', 'DataArray', 'Dataset'])
def container(request):
    return request.param


def test_sum(container):
    x = container(sc.array(dims=['xx', 'yy'], values=[[1, 2, 3], [4, 5, 6]], unit='m'))
    assert sc.identical(sc.sum(x), container(sc.scalar(21, unit='m')))
    assert sc.identical(x.sum(), container(sc.scalar(21, unit='m')))


def test_sum_single_dim(container):
    var = container(sc.array(dims=['xx', 'yy'], values=[[1, 2, 3], [4, 5, 6]],
                             unit='m'))

    assert sc.identical(sc.sum(var, 'xx'),
                        container(sc.array(dims=['yy'], values=[5, 7, 9], unit='m')))
    assert sc.identical(var.sum('xx'),
                        container(sc.array(dims=['yy'], values=[5, 7, 9], unit='m')))

    assert sc.identical(sc.sum(var, 'yy'),
                        container(sc.array(dims=['xx'], values=[6, 15], unit='m')))
    assert sc.identical(var.sum('yy'),
                        container(sc.array(dims=['xx'], values=[6, 15], unit='m')))


def test_nansum(container):
    x = container(
        sc.array(dims=['xx', 'yy'], values=[[1, np.nan, 3], [4, 5, np.nan]], unit='m'))
    assert sc.identical(sc.nansum(x), container(sc.scalar(13., unit='m')))
    assert sc.identical(x.nansum(), container(sc.scalar(13., unit='m')))


def test_nansum_single_dim(container):
    var = container(
        sc.array(dims=['xx', 'yy'], values=[[1, np.nan, 3], [4, 5, np.nan]], unit='m'))

    assert sc.identical(sc.nansum(var, 'xx'),
                        container(sc.array(dims=['yy'], values=[5., 5, 3], unit='m')))
    assert sc.identical(var.nansum('xx'),
                        container(sc.array(dims=['yy'], values=[5., 5, 3], unit='m')))

    assert sc.identical(sc.nansum(var, 'yy'),
                        container(sc.array(dims=['xx'], values=[4., 9], unit='m')))
    assert sc.identical(var.nansum('yy'),
                        container(sc.array(dims=['xx'], values=[4., 9], unit='m')))


def test_mean(container):
    x = container(sc.array(dims=['xx', 'yy'], values=[[1, 2, 3], [4, 5, 6]], unit='m'))
    assert sc.identical(sc.mean(x), container(sc.scalar(3.5, unit='m')))
    assert sc.identical(x.mean(), container(sc.scalar(3.5, unit='m')))


def test_mean_single_dim(container):
    var = container(sc.array(dims=['xx', 'yy'], values=[[1, 2, 3], [4, 5, 6]],
                             unit='m'))

    assert sc.identical(
        sc.mean(var, 'xx'),
        container(sc.array(dims=['yy'], values=[2.5, 3.5, 4.5], unit='m')))
    assert sc.identical(
        var.mean('xx'),
        container(sc.array(dims=['yy'], values=[2.5, 3.5, 4.5], unit='m')))

    assert sc.identical(sc.mean(var, 'yy'),
                        container(sc.array(dims=['xx'], values=[2., 5], unit='m')))
    assert sc.identical(var.mean('yy'),
                        container(sc.array(dims=['xx'], values=[2., 5], unit='m')))


def test_nanmean(container):
    x = container(
        sc.array(dims=['xx', 'yy'], values=[[1, np.nan, 3], [4, 5, np.nan]], unit='m'))
    assert sc.identical(sc.nanmean(x), container(sc.scalar(3.25, unit='m')))
    assert sc.identical(x.nanmean(), container(sc.scalar(3.25, unit='m')))


def test_nanmean_single_dim(container):
    var = container(
        sc.array(dims=['xx', 'yy'], values=[[1, np.nan, 3], [4, 5, np.nan]], unit='m'))

    assert sc.identical(sc.nanmean(var, 'xx'),
                        container(sc.array(dims=['yy'], values=[2.5, 5, 3], unit='m')))
    assert sc.identical(var.nanmean('xx'),
                        container(sc.array(dims=['yy'], values=[2.5, 5, 3], unit='m')))

    assert sc.identical(sc.nanmean(var, 'yy'),
                        container(sc.array(dims=['xx'], values=[2., 4.5], unit='m')))
    assert sc.identical(var.nanmean('yy'),
                        container(sc.array(dims=['xx'], values=[2., 4.5], unit='m')))


def test_max(container):
    x = container(sc.array(dims=['xx', 'yy'], values=[[1, 2, 3], [4, 5, 6]], unit='m'))
    assert sc.identical(sc.max(x), container(sc.scalar(6, unit='m')))
    assert sc.identical(x.max(), container(sc.scalar(6, unit='m')))


def test_max_single_dim(container):
    var = container(sc.array(dims=['xx', 'yy'], values=[[1, 2, 3], [4, 5, 6]],
                             unit='m'))

    assert sc.identical(sc.max(var, 'xx'),
                        container(sc.array(dims=['yy'], values=[4, 5, 6], unit='m')))
    assert sc.identical(var.max('xx'),
                        container(sc.array(dims=['yy'], values=[4, 5, 6], unit='m')))

    assert sc.identical(sc.max(var, 'yy'),
                        container(sc.array(dims=['xx'], values=[3, 6], unit='m')))
    assert sc.identical(var.max('yy'),
                        container(sc.array(dims=['xx'], values=[3, 6], unit='m')))


def test_nanmax(container):
    x = container(
        sc.array(dims=['xx', 'yy'], values=[[1, np.nan, 3], [4, 5, np.nan]], unit='m'))
    assert sc.identical(sc.nanmax(x), container(sc.scalar(5., unit='m')))
    assert sc.identical(x.nanmax(), container(sc.scalar(5., unit='m')))


def test_nanmax_single_dim(container):
    var = container(
        sc.array(dims=['xx', 'yy'], values=[[1, np.nan, 3], [4, 5, np.nan]], unit='m'))

    assert sc.identical(sc.nanmax(var, 'xx'),
                        container(sc.array(dims=['yy'], values=[4., 5, 3], unit='m')))
    assert sc.identical(var.nanmax('xx'),
                        container(sc.array(dims=['yy'], values=[4., 5, 3], unit='m')))

    assert sc.identical(sc.nanmax(var, 'yy'),
                        container(sc.array(dims=['xx'], values=[3., 5], unit='m')))
    assert sc.identical(var.nanmax('yy'),
                        container(sc.array(dims=['xx'], values=[3., 5], unit='m')))


def test_min(container):
    x = container(sc.array(dims=['xx', 'yy'], values=[[1, 2, 3], [4, 5, 6]], unit='m'))
    assert sc.identical(sc.min(x), container(sc.scalar(1, unit='m')))
    assert sc.identical(x.min(), container(sc.scalar(1, unit='m')))


def test_min_single_dim(container):
    var = container(sc.array(dims=['xx', 'yy'], values=[[1, 2, 3], [4, 5, 6]],
                             unit='m'))

    assert sc.identical(sc.min(var, 'xx'),
                        container(sc.array(dims=['yy'], values=[1, 2, 3], unit='m')))
    assert sc.identical(var.min('xx'),
                        container(sc.array(dims=['yy'], values=[1, 2, 3], unit='m')))

    assert sc.identical(sc.min(var, 'yy'),
                        container(sc.array(dims=['xx'], values=[1, 4], unit='m')))
    assert sc.identical(var.min('yy'),
                        container(sc.array(dims=['xx'], values=[1, 4], unit='m')))


def test_nanmin(container):
    x = container(
        sc.array(dims=['xx', 'yy'], values=[[1, np.nan, 3], [4, 5, np.nan]], unit='m'))
    assert sc.identical(sc.nanmin(x), container(sc.scalar(1., unit='m')))
    assert sc.identical(x.nanmin(), container(sc.scalar(1., unit='m')))


def test_nanmin_single_dim(container):
    var = container(
        sc.array(dims=['xx', 'yy'], values=[[1, np.nan, 3], [4, 5, np.nan]], unit='m'))

    assert sc.identical(sc.nanmin(var, 'xx'),
                        container(sc.array(dims=['yy'], values=[1., 5, 3], unit='m')))
    assert sc.identical(var.nanmin('xx'),
                        container(sc.array(dims=['yy'], values=[1., 5, 3], unit='m')))

    assert sc.identical(sc.nanmin(var, 'yy'),
                        container(sc.array(dims=['xx'], values=[1., 4], unit='m')))
    assert sc.identical(var.nanmin('yy'),
                        container(sc.array(dims=['xx'], values=[1., 4], unit='m')))
