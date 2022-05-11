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
