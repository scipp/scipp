# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen
from collections.abc import Callable
from typing import Any

import numpy as np
import pytest

import scipp as sc
import scipp.testing


@pytest.fixture(
    params=[
        lambda x: x,
        lambda x: sc.DataArray(x),
        lambda x: sc.Dataset({'a': x}),
        lambda x: sc.DataGroup({'a': x}),
    ],
    ids=['Variable', 'DataArray', 'Dataset', 'DataGroup'],
)
def container(request: pytest.FixtureRequest) -> Callable[[object], Any]:
    return request.param  # type: ignore[no-any-return]


def test_sum(container: Callable[[object], Any]) -> None:
    x = container(
        sc.array(
            dims=['xx', 'yy'], values=[[1, 2, 3], [4, 5, 6]], unit='m', dtype='int64'
        )
    )
    assert sc.identical(sc.sum(x), container(sc.scalar(21, unit='m', dtype='int64')))
    assert sc.identical(x.sum(), container(sc.scalar(21, unit='m', dtype='int64')))


def test_sum_single_dim(container: Callable[[object], Any]) -> None:
    var = container(
        sc.array(dims=['xx', 'yy'], values=[[1, 2, 3], [4, 5, 6]], unit='m')
    )

    assert sc.identical(
        sc.sum(var, 'xx'), container(sc.array(dims=['yy'], values=[5, 7, 9], unit='m'))
    )
    assert sc.identical(
        var.sum('xx'), container(sc.array(dims=['yy'], values=[5, 7, 9], unit='m'))
    )

    assert sc.identical(
        sc.sum(var, 'yy'), container(sc.array(dims=['xx'], values=[6, 15], unit='m'))
    )
    assert sc.identical(
        var.sum('yy'), container(sc.array(dims=['xx'], values=[6, 15], unit='m'))
    )


def test_sum_dataset_with_coords() -> None:
    d = sc.Dataset(
        data={
            'a': sc.arange('a', 6, dtype='int64').fold('a', sizes={'x': 2, 'y': 3}),
        },
        coords={
            'x': sc.arange('x', 2, dtype='int64'),
            'y': sc.arange('y', 3, dtype='int64'),
            'l1': sc.arange('a', 6, dtype='int64').fold('a', sizes={'x': 2, 'y': 3}),
            'l2': sc.arange('x', 2, dtype='int64'),
        },
    )
    d_ref = sc.Dataset(
        data={
            'a': sc.array(dims=['x'], values=[3, 12], dtype='int64'),
        },
        coords={
            'x': sc.arange('x', 2, dtype='int64'),
            'l2': sc.arange('x', 2, dtype='int64'),
        },
    )

    assert sc.identical(sc.sum(d, 'y'), d_ref)


def test_sum_masked() -> None:
    d = sc.Dataset(
        data={'a': sc.array(dims=['x'], values=[1, 5, 4, 5, 1], dtype='int64')}
    )
    d['a'].masks['m1'] = sc.array(dims=['x'], values=[False, True, False, True, False])

    d_ref = sc.Dataset(data={'a': sc.scalar(np.int64(6))})

    result = sc.sum(d, 'x')['a']
    assert sc.identical(result, d_ref['a'])


def test_nansum(container: Callable[[object], Any]) -> None:
    x = container(
        sc.array(dims=['xx', 'yy'], values=[[1, np.nan, 3], [4, 5, np.nan]], unit='m')
    )
    assert sc.identical(sc.nansum(x), container(sc.scalar(13.0, unit='m')))
    assert sc.identical(x.nansum(), container(sc.scalar(13.0, unit='m')))


def test_nansum_single_dim(container: Callable[[object], Any]) -> None:
    var = container(
        sc.array(dims=['xx', 'yy'], values=[[1, np.nan, 3], [4, 5, np.nan]], unit='m')
    )

    assert sc.identical(
        sc.nansum(var, 'xx'),
        container(sc.array(dims=['yy'], values=[5.0, 5, 3], unit='m')),
    )
    assert sc.identical(
        var.nansum('xx'), container(sc.array(dims=['yy'], values=[5.0, 5, 3], unit='m'))
    )

    assert sc.identical(
        sc.nansum(var, 'yy'),
        container(sc.array(dims=['xx'], values=[4.0, 9], unit='m')),
    )
    assert sc.identical(
        var.nansum('yy'), container(sc.array(dims=['xx'], values=[4.0, 9], unit='m'))
    )


def test_nansum_masked() -> None:
    d = sc.Dataset(
        data={
            'a': sc.Variable(
                dims=['x'], values=np.array([1, 5, np.nan, np.nan, 1], dtype=np.float64)
            )
        }
    )
    d['a'].masks['m1'] = sc.Variable(
        dims=['x'], values=np.array([False, True, False, True, False])
    )

    d_ref = sc.Dataset(data={'a': sc.scalar(np.float64(2))})

    result = sc.nansum(d, 'x')['a']
    assert sc.identical(result, d_ref['a'])


def test_mean(container: Callable[[object], Any]) -> None:
    x = container(sc.array(dims=['xx', 'yy'], values=[[1, 2, 3], [4, 5, 6]], unit='m'))
    assert sc.identical(sc.mean(x), container(sc.scalar(3.5, unit='m')))
    assert sc.identical(x.mean(), container(sc.scalar(3.5, unit='m')))


def test_mean_single_dim(container: Callable[[object], Any]) -> None:
    var = container(
        sc.array(dims=['xx', 'yy'], values=[[1, 2, 3], [4, 5, 6]], unit='m')
    )

    assert sc.identical(
        sc.mean(var, 'xx'),
        container(sc.array(dims=['yy'], values=[2.5, 3.5, 4.5], unit='m')),
    )
    assert sc.identical(
        var.mean('xx'),
        container(sc.array(dims=['yy'], values=[2.5, 3.5, 4.5], unit='m')),
    )

    assert sc.identical(
        sc.mean(var, 'yy'), container(sc.array(dims=['xx'], values=[2.0, 5], unit='m'))
    )
    assert sc.identical(
        var.mean('yy'), container(sc.array(dims=['xx'], values=[2.0, 5], unit='m'))
    )


def test_mean_dataset_with_coords() -> None:
    d = sc.Dataset(
        data={
            'a': sc.arange('aux', 6, dtype='int64').fold('aux', sizes={'x': 2, 'y': 3}),
        },
        coords={
            'x': sc.arange('x', 2, dtype='int64'),
            'y': sc.arange('y', 3, dtype='int64'),
            'l1': sc.arange('aux', 6, dtype='int64').fold(
                'aux', sizes={'x': 2, 'y': 3}
            ),
            'l2': 2 * sc.arange('x', 2, dtype='int64'),
        },
    )
    expected = sc.Dataset(
        data={'a': sc.array(dims=['x'], values=[1.0, 4.0])},
        coords={
            'x': sc.arange('x', 2, dtype='int64'),
            'l2': 2 * sc.arange('x', 2, dtype='int64'),
        },
    )
    sc.testing.assert_identical(sc.mean(d, 'y'), expected)
    sc.testing.assert_identical(d.mean('y'), expected)


def test_mean_masked() -> None:
    d = sc.Dataset(
        data={
            'a': sc.Variable(
                dims=['x'], values=np.array([1, 5, 4, 5, 1]), dtype=sc.DType.float64
            )
        }
    )
    d['a'].masks['m1'] = sc.Variable(
        dims=['x'], values=np.array([False, True, False, True, False])
    )
    d_ref = sc.Dataset(data={'a': sc.scalar(2.0)})
    assert sc.identical(sc.mean(d, 'x')['a'], d_ref['a'])
    assert sc.identical(sc.nanmean(d, 'x')['a'], d_ref['a'])


def test_nanmean(container: Callable[[object], Any]) -> None:
    x = container(
        sc.array(dims=['xx', 'yy'], values=[[1, np.nan, 3], [4, 5, np.nan]], unit='m')
    )
    assert sc.identical(sc.nanmean(x), container(sc.scalar(3.25, unit='m')))
    assert sc.identical(x.nanmean(), container(sc.scalar(3.25, unit='m')))


def test_nanmean_single_dim(container: Callable[[object], Any]) -> None:
    var = container(
        sc.array(dims=['xx', 'yy'], values=[[1, np.nan, 3], [4, 5, np.nan]], unit='m')
    )

    assert sc.identical(
        sc.nanmean(var, 'xx'),
        container(sc.array(dims=['yy'], values=[2.5, 5, 3], unit='m')),
    )
    assert sc.identical(
        var.nanmean('xx'),
        container(sc.array(dims=['yy'], values=[2.5, 5, 3], unit='m')),
    )

    assert sc.identical(
        sc.nanmean(var, 'yy'),
        container(sc.array(dims=['xx'], values=[2.0, 4.5], unit='m')),
    )
    assert sc.identical(
        var.nanmean('yy'), container(sc.array(dims=['xx'], values=[2.0, 4.5], unit='m'))
    )


def test_median_even(container: Callable[[object], Any]) -> None:
    x = container(sc.array(dims=['xx', 'yy'], values=[[6, 3, 2], [4, 5, 1]], unit='m'))
    sc.testing.assert_identical(sc.median(x), container(sc.scalar(3.5, unit='m')))
    sc.testing.assert_identical(x.median(), container(sc.scalar(3.5, unit='m')))
    sc.testing.assert_identical(
        sc.median(x, dim=['xx', 'yy']), container(sc.scalar(3.5, unit='m'))
    )
    sc.testing.assert_identical(
        x.median(dim=['xx', 'yy']), container(sc.scalar(3.5, unit='m'))
    )
    sc.testing.assert_identical(
        sc.median(x, dim=['yy', 'xx']), container(sc.scalar(3.5, unit='m'))
    )
    sc.testing.assert_identical(
        x.median(dim=['yy', 'xx']), container(sc.scalar(3.5, unit='m'))
    )


def test_median_odd(container: Callable[[object], Any]) -> None:
    x = container(sc.array(dims=['xx'], values=[7, 4, 5, 2, 1], unit='m'))
    sc.testing.assert_identical(sc.median(x), container(sc.scalar(4.0, unit='m')))
    sc.testing.assert_identical(x.median(), container(sc.scalar(4.0, unit='m')))
    sc.testing.assert_identical(
        sc.median(x, dim=['xx']), container(sc.scalar(4.0, unit='m'))
    )
    sc.testing.assert_identical(
        x.median(dim=['xx']), container(sc.scalar(4.0, unit='m'))
    )


def test_median_single_dim(container: Callable[[object], Any]) -> None:
    x = container(sc.array(dims=['xx', 'yy'], values=[[6, 3, 2], [4, 5, 1]], unit='m'))
    sc.testing.assert_identical(
        sc.median(x, dim='xx'),
        container(sc.array(dims=['yy'], values=[5, 4, 1.5], unit='m')),
    )
    sc.testing.assert_identical(
        x.median(dim='xx'),
        container(sc.array(dims=['yy'], values=[5, 4, 1.5], unit='m')),
    )
    sc.testing.assert_identical(
        sc.median(x, dim=['xx']),
        container(sc.array(dims=['yy'], values=[5, 4, 1.5], unit='m')),
    )
    sc.testing.assert_identical(
        x.median(dim=['xx']),
        container(sc.array(dims=['yy'], values=[5, 4, 1.5], unit='m')),
    )

    sc.testing.assert_identical(
        sc.median(x, dim='yy'),
        container(sc.array(dims=['xx'], values=[3.0, 4], unit='m')),
    )
    sc.testing.assert_identical(
        x.median(dim='yy'), container(sc.array(dims=['xx'], values=[3.0, 4], unit='m'))
    )
    sc.testing.assert_identical(
        sc.median(x, dim=['yy']),
        container(sc.array(dims=['xx'], values=[3.0, 4], unit='m')),
    )
    sc.testing.assert_identical(
        x.median(dim=['yy']),
        container(sc.array(dims=['xx'], values=[3.0, 4], unit='m')),
    )


def test_median_raises_for_variances(container: Callable[[object], Any]) -> None:
    x = container(sc.array(dims=['x'], values=[1.2, 3.4], variances=[0.1, 1.3]))
    with pytest.raises(sc.VariancesError):
        sc.median(x)
    with pytest.raises(sc.VariancesError):
        x.median()


def test_median_dataset_with_coords() -> None:
    d = sc.Dataset(
        data={
            'a': sc.arange('aux', 6, dtype='int64').fold('aux', sizes={'x': 2, 'y': 3}),
        },
        coords={
            'x': sc.arange('x', 2, dtype='int64'),
            'y': sc.arange('y', 3, dtype='int64'),
            'l1': sc.arange('aux', 6, dtype='int64').fold(
                'aux', sizes={'x': 2, 'y': 3}
            ),
            'l2': 2 * sc.arange('x', 2, dtype='int64'),
        },
    )
    expected = sc.Dataset(
        data={'a': sc.array(dims=['x'], values=[1.0, 4.0])},
        coords={
            'x': sc.arange('x', 2, dtype='int64'),
            'l2': 2 * sc.arange('x', 2, dtype='int64'),
        },
    )
    sc.testing.assert_identical(sc.median(d, 'y'), expected)
    sc.testing.assert_identical(d.median('y'), expected)


def test_median_single_mask() -> None:
    d = sc.Dataset(
        data={
            'a': sc.array(dims=['x'], values=[1, 5, 4, 5, 1], dtype=sc.DType.float64)
        },
    )
    d['a'].masks['m1'] = sc.array(dims=['x'], values=[False, True, True, False, False])
    d_ref = sc.Dataset(data={'a': sc.scalar(1.0)})
    sc.testing.assert_identical(sc.median(d, 'x'), d_ref)


def test_median_two_masks_1d() -> None:
    d = sc.Dataset(
        data={
            'a': sc.array(dims=['x'], values=[1, 5, 4, 5, 1], dtype=sc.DType.float64)
        },
    )
    d['a'].masks['m1'] = sc.array(dims=['x'], values=[False, False, True, False, False])
    d['a'].masks['m2'] = sc.array(dims=['x'], values=[False, True, False, False, True])
    d_ref = sc.Dataset(data={'a': sc.scalar(3.0)})
    sc.testing.assert_identical(sc.median(d, 'x'), d_ref)


def test_median_mask_along_reduced_dim() -> None:
    d = sc.Dataset(
        data={'a': sc.array(dims=['x', 'y'], values=[[1, 5], [4, 5], [1, 3]])},
    )
    d['a'].masks['m1'] = sc.array(dims=['x'], values=[False, False, True])
    d_ref = sc.Dataset(data={'a': sc.array(dims=['y'], values=[2.5, 5.0])})
    sc.testing.assert_identical(sc.median(d, 'x'), d_ref)


def test_median_mask_along_other_dim() -> None:
    d = sc.Dataset(
        data={'a': sc.array(dims=['x', 'y'], values=[[1, 5], [4, 5], [1, 3]])},
    )
    d['a'].masks['m1'] = sc.array(dims=['x'], values=[False, False, True])
    d_ref = sc.Dataset(data={'a': sc.array(dims=['x'], values=[3.0, 4.5, 2.0])})
    d_ref['a'].masks['m1'] = sc.array(dims=['x'], values=[False, False, True])
    sc.testing.assert_identical(sc.median(d, 'y'), d_ref)


def test_median_mask_along_multiple_dims() -> None:
    d = sc.Dataset(
        data={'a': sc.array(dims=['x', 'y'], values=[[1, 5], [4, 5], [1, 3]])},
    )
    d['a'].masks['m1'] = sc.array(dims=['x'], values=[False, False, True])
    d['a'].masks['m2'] = sc.array(dims=['y'], values=[False, True])
    d_ref = sc.Dataset(data={'a': sc.scalar(2.5)})
    sc.testing.assert_identical(sc.median(d), d_ref)


def test_median_multi_dim_mask() -> None:
    d = sc.Dataset(
        data={'a': sc.array(dims=['x', 'y'], values=[[1, 5], [4, 5], [1, 3]])},
    )
    d['a'].masks['m1'] = sc.array(
        dims=['x', 'y'], values=[[False, True], [True, False], [True, True]]
    )
    d_ref = sc.Dataset(data={'a': sc.scalar(3.0)})
    sc.testing.assert_identical(sc.median(d), d_ref)

    d_ref = sc.Dataset(data={'a': sc.array(dims=['y'], values=[1.0, 5.0])})
    sc.testing.assert_identical(sc.median(d, 'x'), d_ref)

    d_ref = sc.Dataset(data={'a': sc.array(dims=['x'], values=[1.0, 5.0, 0.0])})
    sc.testing.assert_identical(sc.median(d, 'y'), d_ref)


def test_median_all_masked() -> None:
    d = sc.Dataset(
        data={'a': sc.array(dims=['x'], values=[1, 5, 4, 5, 1])},
    )
    d['a'].masks['m1'] = sc.full(value=True, sizes=d.sizes)
    d_ref = sc.Dataset(data={'a': sc.scalar(0.0)})
    sc.testing.assert_identical(sc.median(d), d_ref)


def test_median_binned_not_supported() -> None:
    buffer = sc.DataArray(
        sc.ones(sizes={'event': 10}), coords={'x': sc.arange('event', 10)}
    )
    da = buffer.bin(x=2)
    with pytest.raises(sc.DTypeError, match='binned'):
        sc.median(da)
    with pytest.raises(sc.DTypeError, match='binned'):
        da.median()


def test_nanmedian_even(container: Callable[[object], Any]) -> None:
    x = container(
        sc.array(dims=['xx', 'yy'], values=[[np.nan, 3, 2], [4, np.nan, 1]], unit='m')
    )
    sc.testing.assert_identical(sc.nanmedian(x), container(sc.scalar(2.5, unit='m')))
    sc.testing.assert_identical(x.nanmedian(), container(sc.scalar(2.5, unit='m')))
    sc.testing.assert_identical(
        sc.nanmedian(x, dim=['xx', 'yy']), container(sc.scalar(2.5, unit='m'))
    )
    sc.testing.assert_identical(
        x.nanmedian(dim=['xx', 'yy']), container(sc.scalar(2.5, unit='m'))
    )
    sc.testing.assert_identical(
        sc.nanmedian(x, dim=['yy', 'xx']), container(sc.scalar(2.5, unit='m'))
    )
    sc.testing.assert_identical(
        x.nanmedian(dim=['yy', 'xx']), container(sc.scalar(2.5, unit='m'))
    )


def test_nanmedian_odd(container: Callable[[object], Any]) -> None:
    x = container(sc.array(dims=['xx'], values=[7, np.nan, 5, np.nan, 1], unit='m'))
    sc.testing.assert_identical(sc.nanmedian(x), container(sc.scalar(5.0, unit='m')))
    sc.testing.assert_identical(x.nanmedian(), container(sc.scalar(5.0, unit='m')))
    sc.testing.assert_identical(
        sc.nanmedian(x, dim=['xx']), container(sc.scalar(5.0, unit='m'))
    )
    sc.testing.assert_identical(
        x.nanmedian(dim=['xx']), container(sc.scalar(5.0, unit='m'))
    )


def test_nanmedian_single_dim(container: Callable[[object], Any]) -> None:
    x = container(
        sc.array(dims=['xx', 'yy'], values=[[6, np.nan, 2], [np.nan, 5, 1]], unit='m')
    )
    sc.testing.assert_identical(
        sc.nanmedian(x, dim='xx'),
        container(sc.array(dims=['yy'], values=[6, 5, 1.5], unit='m')),
    )
    sc.testing.assert_identical(
        x.nanmedian(dim='xx'),
        container(sc.array(dims=['yy'], values=[6, 5, 1.5], unit='m')),
    )
    sc.testing.assert_identical(
        sc.nanmedian(x, dim=['xx']),
        container(sc.array(dims=['yy'], values=[6, 5, 1.5], unit='m')),
    )
    sc.testing.assert_identical(
        x.nanmedian(dim=['xx']),
        container(sc.array(dims=['yy'], values=[6, 5, 1.5], unit='m')),
    )

    sc.testing.assert_identical(
        sc.nanmedian(x, dim='yy'),
        container(sc.array(dims=['xx'], values=[4.0, 3.0], unit='m')),
    )
    sc.testing.assert_identical(
        x.nanmedian(dim='yy'),
        container(sc.array(dims=['xx'], values=[4.0, 3.0], unit='m')),
    )
    sc.testing.assert_identical(
        sc.nanmedian(x, dim=['yy']),
        container(sc.array(dims=['xx'], values=[4.0, 3.0], unit='m')),
    )
    sc.testing.assert_identical(
        x.nanmedian(dim=['yy']),
        container(sc.array(dims=['xx'], values=[4.0, 3.0], unit='m')),
    )


def test_var(container: Callable[[object], Any]) -> None:
    x = container(sc.array(dims=['xx', 'yy'], values=[[2, 5, 3], [2, 2, 4]], unit='m'))
    # Yes, using identical with floats.
    # It works and allclose doesn't support datasets and data groups.
    sc.testing.assert_identical(
        sc.var(x, ddof=0), container(sc.scalar(4 / 3, unit='m**2'))
    )
    sc.testing.assert_identical(x.var(ddof=0), container(sc.scalar(4 / 3, unit='m**2')))
    sc.testing.assert_identical(
        sc.var(x, ddof=0, dim=['xx', 'yy']), container(sc.scalar(4 / 3, unit='m**2'))
    )
    sc.testing.assert_identical(
        x.var(ddof=0, dim=['xx', 'yy']), container(sc.scalar(4 / 3, unit='m**2'))
    )

    sc.testing.assert_identical(
        sc.var(x, ddof=1), container(sc.scalar(1.6, unit='m**2'))
    )
    sc.testing.assert_identical(x.var(ddof=1), container(sc.scalar(1.6, unit='m**2')))
    sc.testing.assert_identical(
        sc.var(x, ddof=1, dim=['xx', 'yy']), container(sc.scalar(1.6, unit='m**2'))
    )
    sc.testing.assert_identical(
        x.var(ddof=1, dim=['xx', 'yy']), container(sc.scalar(1.6, unit='m**2'))
    )


def test_var_single_dim(container: Callable[[object], Any]) -> None:
    x = container(sc.array(dims=['xx', 'yy'], values=[[2, 5, 3], [2, 2, 4]], unit='m'))
    # Yes, using identical with floats.
    # It works and allclose doesn't support datasets and data groups.
    sc.testing.assert_identical(
        sc.var(x, 'xx', ddof=0),
        container(sc.array(dims=['yy'], values=[0.0, 2.25, 0.25], unit='m**2')),
    )
    sc.testing.assert_identical(
        x.var('xx', ddof=0),
        container(sc.array(dims=['yy'], values=[0.0, 2.25, 0.25], unit='m**2')),
    )
    sc.testing.assert_identical(
        sc.var(x, dim=['yy'], ddof=0),
        container(sc.array(dims=['xx'], values=[14 / 9, 8 / 9], unit='m**2')),
    )
    sc.testing.assert_identical(
        x.var(dim=['yy'], ddof=0),
        container(sc.array(dims=['xx'], values=[14 / 9, 8 / 9], unit='m**2')),
    )

    sc.testing.assert_identical(
        sc.var(x, 'xx', ddof=1),
        container(sc.array(dims=['yy'], values=[0.0, 4.5, 0.5], unit='m**2')),
    )
    sc.testing.assert_identical(
        x.var('xx', ddof=1),
        container(sc.array(dims=['yy'], values=[0.0, 4.5, 0.5], unit='m**2')),
    )
    sc.testing.assert_identical(
        sc.var(x, dim=['yy'], ddof=1),
        container(sc.array(dims=['xx'], values=[7 / 3, 4 / 3], unit='m**2')),
    )
    sc.testing.assert_identical(
        x.var(dim=['yy'], ddof=1),
        container(sc.array(dims=['xx'], values=[7 / 3, 4 / 3], unit='m**2')),
    )


def test_nanvar(container: Callable[[object], Any]) -> None:
    x = container(
        sc.array(dims=['xx', 'yy'], values=[[2, np.nan, 3], [2, 2, np.nan]], unit='m')
    )
    # Yes, using identical with floats.
    # It works and allclose doesn't support datasets and data groups.
    sc.testing.assert_identical(
        sc.nanvar(x, ddof=0), container(sc.scalar(0.1875, unit='m**2'))
    )
    sc.testing.assert_identical(
        x.nanvar(ddof=0), container(sc.scalar(0.1875, unit='m**2'))
    )
    sc.testing.assert_identical(
        sc.nanvar(x, ddof=0, dim=['xx', 'yy']),
        container(sc.scalar(0.1875, unit='m**2')),
    )
    sc.testing.assert_identical(
        x.nanvar(ddof=0, dim=['xx', 'yy']), container(sc.scalar(0.1875, unit='m**2'))
    )


def test_nanvar_single_dim(container: Callable[[object], Any]) -> None:
    x = container(
        sc.array(dims=['xx', 'yy'], values=[[2, np.nan, 3], [2, 2, np.nan]], unit='m')
    )
    # Yes, using identical with floats.
    # It works and allclose doesn't support datasets and data groups.
    sc.testing.assert_identical(
        sc.nanvar(x, 'xx', ddof=0),
        container(sc.array(dims=['yy'], values=[0.0, 0.0, 0.0], unit='m**2')),
    )
    sc.testing.assert_identical(
        x.nanvar('xx', ddof=0),
        container(sc.array(dims=['yy'], values=[0.0, 0.0, 0.0], unit='m**2')),
    )
    sc.testing.assert_identical(
        sc.nanvar(x, dim=['yy'], ddof=0),
        container(sc.array(dims=['xx'], values=[0.25, 0.0], unit='m**2')),
    )
    sc.testing.assert_identical(
        x.nanvar(dim=['yy'], ddof=0),
        container(sc.array(dims=['xx'], values=[0.25, 0.0], unit='m**2')),
    )


def test_std_variable() -> None:
    x = sc.array(dims=['xx', 'yy'], values=[[2, 5, 3], [2, 2, 4]], unit='m')
    sc.testing.assert_allclose(
        sc.std(x, ddof=0), sc.scalar(1.1547005383792515, unit='m')
    )
    sc.testing.assert_allclose(x.std(ddof=0), sc.scalar(1.1547005383792515, unit='m'))
    sc.testing.assert_allclose(
        sc.std(x, ddof=0, dim=['xx', 'yy']), sc.scalar(1.1547005383792515, unit='m')
    )
    sc.testing.assert_allclose(
        x.std(ddof=0, dim=['xx', 'yy']), sc.scalar(1.1547005383792515, unit='m')
    )

    sc.testing.assert_allclose(
        sc.std(x, ddof=1), sc.scalar(1.2649110640673518, unit='m')
    )
    sc.testing.assert_allclose(x.std(ddof=1), sc.scalar(1.2649110640673518, unit='m'))
    sc.testing.assert_allclose(
        sc.std(x, ddof=1, dim=['xx', 'yy']), sc.scalar(1.2649110640673518, unit='m')
    )
    sc.testing.assert_allclose(
        x.std(ddof=1, dim=['xx', 'yy']), sc.scalar(1.2649110640673518, unit='m')
    )


def test_std_data_array() -> None:
    x = sc.DataArray(
        sc.array(dims=['xx', 'yy'], values=[[2, 5, 3], [2, 2, 4]], unit='m')
    )
    sc.testing.assert_allclose(
        sc.std(x, ddof=0), sc.DataArray(sc.scalar(1.1547005383792515, unit='m'))
    )
    sc.testing.assert_allclose(
        x.std(ddof=0), sc.DataArray(sc.scalar(1.1547005383792515, unit='m'))
    )


def test_std_dataset() -> None:
    x = sc.Dataset(
        {'a': sc.array(dims=['xx', 'yy'], values=[[2, 5, 3], [2, 2, 4]], unit='m')}
    )
    sc.testing.assert_allclose(
        sc.std(x, ddof=0)['a'], sc.DataArray(sc.scalar(1.1547005383792515, unit='m'))
    )
    sc.testing.assert_allclose(
        x.std(ddof=0)['a'], sc.DataArray(sc.scalar(1.1547005383792515, unit='m'))
    )


def test_std_data_group() -> None:
    x = sc.DataGroup(
        {'a': sc.array(dims=['xx', 'yy'], values=[[2, 5, 3], [2, 2, 4]], unit='m')}
    )
    sc.testing.assert_allclose(
        sc.std(x, ddof=0)['a'], sc.scalar(1.1547005383792515, unit='m')
    )
    sc.testing.assert_allclose(
        x.std(ddof=0)['a'], sc.scalar(1.1547005383792515, unit='m')
    )


def test_std_single_dim(container: Callable[[object], Any]) -> None:
    x = container(sc.array(dims=['xx', 'yy'], values=[[2, 5, 3], [2, 2, 4]], unit='m'))
    # Yes, using identical with floats.
    # It works and allclose doesn't support datasets and data groups.
    sc.testing.assert_identical(
        sc.std(x, 'xx', ddof=0),
        container(sc.array(dims=['yy'], values=[0.0, 1.5, 0.5], unit='m')),
    )
    sc.testing.assert_identical(
        x.std('xx', ddof=0),
        container(sc.array(dims=['yy'], values=[0.0, 1.5, 0.5], unit='m')),
    )


def test_std_single_dim_variable() -> None:
    x = sc.array(dims=['xx', 'yy'], values=[[2, 5, 3], [2, 2, 4]], unit='m')
    sc.testing.assert_allclose(
        sc.std(x, dim=['yy'], ddof=0),
        sc.array(dims=['xx'], values=[1.24721913, 0.94280904], unit='m'),
    )
    sc.testing.assert_allclose(
        x.std(dim=['yy'], ddof=0),
        sc.array(dims=['xx'], values=[1.24721913, 0.94280904], unit='m'),
    )

    sc.testing.assert_allclose(
        sc.std(x, 'xx', ddof=1),
        sc.array(dims=['yy'], values=[0.0, 2.12132034, 0.70710678], unit='m'),
    )
    sc.testing.assert_allclose(
        x.std('xx', ddof=1),
        sc.array(dims=['yy'], values=[0.0, 2.12132034, 0.70710678], unit='m'),
    )
    sc.testing.assert_allclose(
        sc.std(x, dim=['yy'], ddof=1),
        sc.array(dims=['xx'], values=[1.52752523, 1.15470054], unit='m'),
    )
    sc.testing.assert_allclose(
        x.std(dim=['yy'], ddof=1),
        sc.array(dims=['xx'], values=[1.52752523, 1.15470054], unit='m'),
    )


def test_nanstd(container: Callable[[object], Any]) -> None:
    x = container(
        sc.array(dims=['xx', 'yy'], values=[[2, np.nan, 3], [2, 3, np.nan]], unit='m')
    )
    # Yes, using identical with floats.
    # It works and allclose doesn't support datasets and data groups.
    sc.testing.assert_identical(
        sc.nanstd(x, ddof=0), container(sc.scalar(0.5, unit='m'))
    )
    sc.testing.assert_identical(x.nanstd(ddof=0), container(sc.scalar(0.5, unit='m')))
    sc.testing.assert_identical(
        sc.nanstd(x, ddof=0, dim=['xx', 'yy']), container(sc.scalar(0.5, unit='m'))
    )
    sc.testing.assert_identical(
        x.nanstd(ddof=0, dim=['xx', 'yy']), container(sc.scalar(0.5, unit='m'))
    )


def test_test_nanstd_single_dim(container: Callable[[object], Any]) -> None:
    x = container(
        sc.array(dims=['xx', 'yy'], values=[[2, np.nan, 3], [2, 2, np.nan]], unit='m')
    )
    # Yes, using identical with floats.
    # It works and allclose doesn't support datasets and data groups.
    sc.testing.assert_identical(
        sc.nanstd(x, 'xx', ddof=0),
        container(sc.array(dims=['yy'], values=[0.0, 0.0, 0.0], unit='m')),
    )
    sc.testing.assert_identical(
        x.nanstd('xx', ddof=0),
        container(sc.array(dims=['yy'], values=[0.0, 0.0, 0.0], unit='m')),
    )
    sc.testing.assert_identical(
        sc.nanstd(x, dim=['yy'], ddof=0),
        container(sc.array(dims=['xx'], values=[0.5, 0.0], unit='m')),
    )
    sc.testing.assert_identical(
        x.nanstd(dim=['yy'], ddof=0),
        container(sc.array(dims=['xx'], values=[0.5, 0.0], unit='m')),
    )


def test_max(container: Callable[[object], Any]) -> None:
    x = container(
        sc.array(
            dims=['xx', 'yy'], values=[[1, 2, 3], [4, 5, 6]], unit='m', dtype='int64'
        )
    )
    assert sc.identical(sc.max(x), container(sc.scalar(6, unit='m', dtype='int64')))
    assert sc.identical(x.max(), container(sc.scalar(6, unit='m', dtype='int64')))


def test_max_single_dim(container: Callable[[object], Any]) -> None:
    var = container(
        sc.array(dims=['xx', 'yy'], values=[[1, 2, 3], [4, 5, 6]], unit='m')
    )

    assert sc.identical(
        sc.max(var, 'xx'), container(sc.array(dims=['yy'], values=[4, 5, 6], unit='m'))
    )
    assert sc.identical(
        var.max('xx'), container(sc.array(dims=['yy'], values=[4, 5, 6], unit='m'))
    )

    assert sc.identical(
        sc.max(var, 'yy'), container(sc.array(dims=['xx'], values=[3, 6], unit='m'))
    )
    assert sc.identical(
        var.max('yy'), container(sc.array(dims=['xx'], values=[3, 6], unit='m'))
    )


def test_nanmax(container: Callable[[object], Any]) -> None:
    x = container(
        sc.array(dims=['xx', 'yy'], values=[[1, np.nan, 3], [4, 5, np.nan]], unit='m')
    )
    assert sc.identical(sc.nanmax(x), container(sc.scalar(5.0, unit='m')))
    assert sc.identical(x.nanmax(), container(sc.scalar(5.0, unit='m')))


def test_nanmax_single_dim(container: Callable[[object], Any]) -> None:
    var = container(
        sc.array(dims=['xx', 'yy'], values=[[1, np.nan, 3], [4, 5, np.nan]], unit='m')
    )

    assert sc.identical(
        sc.nanmax(var, 'xx'),
        container(sc.array(dims=['yy'], values=[4.0, 5, 3], unit='m')),
    )
    assert sc.identical(
        var.nanmax('xx'), container(sc.array(dims=['yy'], values=[4.0, 5, 3], unit='m'))
    )

    assert sc.identical(
        sc.nanmax(var, 'yy'),
        container(sc.array(dims=['xx'], values=[3.0, 5], unit='m')),
    )
    assert sc.identical(
        var.nanmax('yy'), container(sc.array(dims=['xx'], values=[3.0, 5], unit='m'))
    )


def test_min(container: Callable[[object], Any]) -> None:
    x = container(
        sc.array(
            dims=['xx', 'yy'], values=[[1, 2, 3], [4, 5, 6]], unit='m', dtype='int64'
        )
    )
    assert sc.identical(sc.min(x), container(sc.scalar(1, unit='m', dtype='int64')))
    assert sc.identical(x.min(), container(sc.scalar(1, unit='m', dtype='int64')))


def test_min_single_dim(container: Callable[[object], Any]) -> None:
    var = container(
        sc.array(dims=['xx', 'yy'], values=[[1, 2, 3], [4, 5, 6]], unit='m')
    )

    assert sc.identical(
        sc.min(var, 'xx'), container(sc.array(dims=['yy'], values=[1, 2, 3], unit='m'))
    )
    assert sc.identical(
        var.min('xx'), container(sc.array(dims=['yy'], values=[1, 2, 3], unit='m'))
    )

    assert sc.identical(
        sc.min(var, 'yy'), container(sc.array(dims=['xx'], values=[1, 4], unit='m'))
    )
    assert sc.identical(
        var.min('yy'), container(sc.array(dims=['xx'], values=[1, 4], unit='m'))
    )


def test_nanmin(container: Callable[[object], Any]) -> None:
    x = container(
        sc.array(dims=['xx', 'yy'], values=[[1, np.nan, 3], [4, 5, np.nan]], unit='m')
    )
    assert sc.identical(sc.nanmin(x), container(sc.scalar(1.0, unit='m')))
    assert sc.identical(x.nanmin(), container(sc.scalar(1.0, unit='m')))


def test_nanmin_single_dim(container: Callable[[object], Any]) -> None:
    var = container(
        sc.array(dims=['xx', 'yy'], values=[[1, np.nan, 3], [4, 5, np.nan]], unit='m')
    )

    assert sc.identical(
        sc.nanmin(var, 'xx'),
        container(sc.array(dims=['yy'], values=[1.0, 5, 3], unit='m')),
    )
    assert sc.identical(
        var.nanmin('xx'), container(sc.array(dims=['yy'], values=[1.0, 5, 3], unit='m'))
    )

    assert sc.identical(
        sc.nanmin(var, 'yy'),
        container(sc.array(dims=['xx'], values=[1.0, 4], unit='m')),
    )
    assert sc.identical(
        var.nanmin('yy'), container(sc.array(dims=['xx'], values=[1.0, 4], unit='m'))
    )


def test_all(container: Callable[[object], Any]) -> None:
    x = container(
        sc.array(dims=['xx', 'yy'], values=[[True, False, False], [True, True, False]])
    )

    assert sc.identical(sc.all(x), container(sc.scalar(False)))
    assert sc.identical(x.all(), container(sc.scalar(False)))


def test_all_single_dim(container: Callable[[object], Any]) -> None:
    x = container(
        sc.array(dims=['xx', 'yy'], values=[[True, False, False], [True, True, False]])
    )

    assert sc.identical(
        sc.all(x, 'xx'), container(sc.array(dims=['yy'], values=[True, False, False]))
    )
    assert sc.identical(
        x.all('xx'), container(sc.array(dims=['yy'], values=[True, False, False]))
    )

    assert sc.identical(
        sc.all(x, 'yy'), container(sc.array(dims=['xx'], values=[False, False]))
    )
    assert sc.identical(
        x.all('yy'), container(sc.array(dims=['xx'], values=[False, False]))
    )


def test_any(container: Callable[[object], Any]) -> None:
    x = container(
        sc.array(dims=['xx', 'yy'], values=[[True, False, False], [True, True, False]])
    )

    assert sc.identical(sc.any(x), container(sc.scalar(True)))
    assert sc.identical(x.any(), container(sc.scalar(True)))


def test_any_single_dim(container: Callable[[object], Any]) -> None:
    x = container(
        sc.array(dims=['xx', 'yy'], values=[[True, False, False], [True, True, False]])
    )

    assert sc.identical(
        sc.any(x, 'xx'), container(sc.array(dims=['yy'], values=[True, True, False]))
    )
    assert sc.identical(
        x.any('xx'), container(sc.array(dims=['yy'], values=[True, True, False]))
    )

    assert sc.identical(
        sc.any(x, 'yy'), container(sc.array(dims=['xx'], values=[True, True]))
    )
    assert sc.identical(
        x.any('yy'), container(sc.array(dims=['xx'], values=[True, True]))
    )


def test_reduction_with_mask_works_with_vectors() -> None:
    data = sc.vectors(dims=['x'], values=np.arange(12, dtype=np.int64).reshape(4, 3))
    mask = sc.array(dims=['x'], values=[False, True, False, True])
    da = sc.DataArray(data=data, masks={'mask': mask})
    da.sum()


# See docstring of these functions about why the `ddof` parameter is required.
# This test exists to prevent accidental or intentional but not thoroughly
# thought-out changes.
@pytest.mark.parametrize('opname', ['var', 'nanvar', 'std', 'nanstd'])
def test_variance_reductions_require_ddof_param(opname: str) -> None:
    data = sc.zeros(sizes={'x': 2})

    func = getattr(sc, opname)
    with pytest.raises(TypeError, match='ddof'):
        func(data)

    meth = getattr(data, opname)
    with pytest.raises(TypeError, match='ddof'):
        meth()


@pytest.mark.parametrize(
    'opname',
    [
        'mean',
        'nanmean',
        'median',
        'nanmedian',
        'sum',
        'nansum',
        'min',
        'nanmin',
        'max',
        'nanmax',
    ],
)
def test_reduce_two_dims(container: Callable[[object], Any], opname: str) -> None:
    x = container(
        sc.array(
            dims=['xx', 'yy', 'zz'],
            values=[[[1, 2, 3], [4, 5, 6]], [[7, 8, 9], [10, 11, 12]]],
            dtype='int64',
            unit='m',
        )
    )
    func = getattr(sc, opname)
    possibles = (['xx', 'yy', 'zz'], ['yy', 'zz', 'xx'], ['zz', 'xx', 'yy'])
    for dims in possibles:
        res = func(x, dims[:2])
        last = dims[-1]
        for i in range(x.sizes[last]):
            assert sc.identical(res[last, i], func(x[last, i]))

    meth = getattr(x, opname)
    for dims in possibles:
        res = meth(dims[:2])
        last = dims[-1]
        for i in range(x.sizes[last]):
            assert sc.identical(res[last, i], getattr(x[last, i], opname)())
