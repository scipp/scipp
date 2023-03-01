# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
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
    x = container(
        sc.array(dims=['xx', 'yy'],
                 values=[[1, 2, 3], [4, 5, 6]],
                 unit='m',
                 dtype='int64'))
    assert sc.identical(sc.sum(x), container(sc.scalar(21, unit='m', dtype='int64')))
    assert sc.identical(x.sum(), container(sc.scalar(21, unit='m', dtype='int64')))


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


def test_sum_dataset_with_coords():
    d = sc.Dataset(data={
        'a': sc.arange('a', 6, dtype='int64').fold('a', {'x': 2, 'y': 3}),
        'b': sc.arange('y', 3, dtype='int64'),
    },
                   coords={
                       'x': sc.arange('x', 2, dtype='int64'),
                       'y': sc.arange('y', 3, dtype='int64'),
                       'l1': sc.arange('a', 6,
                                       dtype='int64').fold('a', {'x': 2, 'y': 3}),
                       'l2': sc.arange('x', 2, dtype='int64'),
                   })
    d_ref = sc.Dataset(data={
        'a': sc.array(dims=['x'], values=[3, 12], dtype='int64'), 'b': sc.scalar(3)
    },
                       coords={
                           'x': sc.arange('x', 2, dtype='int64'),
                           'l2': sc.arange('x', 2, dtype='int64'),
                       })

    assert sc.identical(sc.sum(d, 'y'), d_ref)


def test_sum_masked():
    d = sc.Dataset(
        data={'a': sc.array(dims=['x'], values=[1, 5, 4, 5, 1], dtype='int64')})
    d['a'].masks['m1'] = sc.array(dims=['x'], values=[False, True, False, True, False])

    d_ref = sc.Dataset(data={'a': sc.scalar(np.int64(6))})

    result = sc.sum(d, 'x')['a']
    assert sc.identical(result, d_ref['a'])


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


def test_nansum_masked():
    d = sc.Dataset(
        data={
            'a': sc.Variable(dims=['x'],
                             values=np.array([1, 5, np.nan, np.nan, 1],
                                             dtype=np.float64))
        })
    d['a'].masks['m1'] = sc.Variable(dims=['x'],
                                     values=np.array([False, True, False, True, False]))

    d_ref = sc.Dataset(data={'a': sc.scalar(np.float64(2))})

    result = sc.nansum(d, 'x')['a']
    assert sc.identical(result, d_ref['a'])


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


def test_mean_dataset_with_coords():
    d = sc.Dataset(data={
        'a': sc.Variable(dims=['x', 'y'],
                         values=np.arange(6, dtype=np.int64).reshape(2, 3)),
        'b': sc.Variable(dims=['y'], values=np.arange(3, dtype=np.int64))
    },
                   coords={
                       'x': sc.Variable(dims=['x'], values=np.arange(2,
                                                                     dtype=np.int64)),
                       'y': sc.Variable(dims=['y'], values=np.arange(3,
                                                                     dtype=np.int64)),
                       'l1': sc.Variable(dims=['x', 'y'],
                                         values=np.arange(6,
                                                          dtype=np.int64).reshape(2,
                                                                                  3)),
                       'l2': sc.Variable(dims=['x'],
                                         values=np.arange(2, dtype=np.int64))
                   })

    assert (sc.mean(d, 'y')['a'].values == [1.0, 4.0]).all()
    assert sc.mean(d, 'y')['b'].value == 1.0


def test_mean_masked():
    d = sc.Dataset(
        data={
            'a': sc.Variable(
                dims=['x'], values=np.array([1, 5, 4, 5, 1]), dtype=sc.DType.float64)
        })
    d['a'].masks['m1'] = sc.Variable(dims=['x'],
                                     values=np.array([False, True, False, True, False]))
    d_ref = sc.Dataset(data={'a': sc.scalar(2.0)})
    assert sc.identical(sc.mean(d, 'x')['a'], d_ref['a'])
    assert sc.identical(sc.nanmean(d, 'x')['a'], d_ref['a'])


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
    x = container(
        sc.array(dims=['xx', 'yy'],
                 values=[[1, 2, 3], [4, 5, 6]],
                 unit='m',
                 dtype='int64'))
    assert sc.identical(sc.max(x), container(sc.scalar(6, unit='m', dtype='int64')))
    assert sc.identical(x.max(), container(sc.scalar(6, unit='m', dtype='int64')))


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
    x = container(
        sc.array(dims=['xx', 'yy'],
                 values=[[1, 2, 3], [4, 5, 6]],
                 unit='m',
                 dtype='int64'))
    assert sc.identical(sc.min(x), container(sc.scalar(1, unit='m', dtype='int64')))
    assert sc.identical(x.min(), container(sc.scalar(1, unit='m', dtype='int64')))


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


def test_all(container):
    x = container(
        sc.array(dims=['xx', 'yy'], values=[[True, False, False], [True, True, False]]))

    assert sc.identical(sc.all(x), container(sc.scalar(False)))
    assert sc.identical(x.all(), container(sc.scalar(False)))


def test_all_single_dim(container):
    x = container(
        sc.array(dims=['xx', 'yy'], values=[[True, False, False], [True, True, False]]))

    assert sc.identical(sc.all(x, 'xx'),
                        container(sc.array(dims=['yy'], values=[True, False, False])))
    assert sc.identical(x.all('xx'),
                        container(sc.array(dims=['yy'], values=[True, False, False])))

    assert sc.identical(sc.all(x, 'yy'),
                        container(sc.array(dims=['xx'], values=[False, False])))
    assert sc.identical(x.all('yy'),
                        container(sc.array(dims=['xx'], values=[False, False])))


def test_any(container):
    x = container(
        sc.array(dims=['xx', 'yy'], values=[[True, False, False], [True, True, False]]))

    assert sc.identical(sc.any(x), container(sc.scalar(True)))
    assert sc.identical(x.any(), container(sc.scalar(True)))


def test_any_single_dim(container):
    x = container(
        sc.array(dims=['xx', 'yy'], values=[[True, False, False], [True, True, False]]))

    assert sc.identical(sc.any(x, 'xx'),
                        container(sc.array(dims=['yy'], values=[True, True, False])))
    assert sc.identical(x.any('xx'),
                        container(sc.array(dims=['yy'], values=[True, True, False])))

    assert sc.identical(sc.any(x, 'yy'),
                        container(sc.array(dims=['xx'], values=[True, True])))
    assert sc.identical(x.any('yy'),
                        container(sc.array(dims=['xx'], values=[True, True])))
