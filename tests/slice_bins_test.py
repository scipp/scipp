# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import pytest

import scipp as sc


def test_slice_bins_by_int_label() -> None:
    table = sc.data.table_xyz(100)
    table.coords['param'] = (table.coords.pop('y') * 10).to(dtype='int64')
    da = table.bin(x=10)
    param = sc.scalar(4, unit='m')
    result = da.bins['param', param]  # type: ignore[index]
    assert result.dims == da.dims
    assert sc.identical(result.coords['param'], param)
    assert not result.coords['param'].aligned
    assert sc.identical(
        result,
        da.group(
            sc.array(dims=['param'], values=[4], dtype='int64', unit='m')
        ).squeeze(),
    )


def test_slice_bins_by_int_label_range() -> None:
    table = sc.data.table_xyz(100)
    table.coords['param'] = sc.arange(dim='row', start=0, stop=100, unit='s') // 10
    da = table.bin(x=10)
    start = sc.scalar(4, unit='s')
    stop = sc.scalar(6, unit='s')
    result = da.bins['param', start:stop]  # type: ignore[index]
    assert result.dims == da.dims
    assert sc.identical(
        result.coords['param'],
        sc.array(dims=['param'], values=[4, 6], unit='s', dtype='int64'),
    )
    assert not result.coords['param'].aligned
    assert (
        result.bins.size().sum().value == 20
    )  # 2x10 events  # type: ignore[index, union-attr]
    assert sc.identical(
        result,
        da.bin(
            param=sc.array(dims=['param'], values=[4, 6], unit='s', dtype='int64')
        ).squeeze(),
    )


def test_slice_bins_by_float_label_range() -> None:
    table = sc.data.table_xyz(100)
    da = table.bin(x=10)
    start = sc.scalar(0.1, unit='m')
    stop = sc.scalar(0.2, unit='m')
    result = da.bins['z', start:stop]
    assert result.dims == da.dims
    assert sc.identical(
        result.coords['z'], sc.array(dims=['z'], values=[0.1, 0.2], unit='m')
    )
    assert not result.coords['z'].aligned
    assert sc.identical(
        result, da.bin(z=sc.array(dims=['z'], values=[0.1, 0.2], unit='m')).squeeze()
    )


def test_slice_bins_by_open_range_includes_everything() -> None:
    table = sc.data.table_xyz(100)
    da = table.bin(x=10)
    result = da.bins['z', :]  # type: ignore[index]
    assert result.bins.size().sum().value == 100


def test_slice_bins_by_half_open_float_range_splits_without_duplication() -> None:
    table = sc.data.table_xyz(100)
    da = table.bin(x=10)
    split = sc.scalar(0.4, unit='m')
    left = da.bins['z', :split]  # type: ignore[index]
    right = da.bins['z', split:]  # type: ignore[index]
    assert left.bins.size().sum().value + right.bins.size().sum().value == 100  # type: ignore[index]
    assert sc.identical(
        left.coords['z'], sc.concat([da.bins.coords['z'].min(), split], 'z')
    )
    import numpy as np

    expected_stop = np.nextafter(da.bins.coords['z'].max().value, np.inf) * sc.Unit('m')
    assert sc.identical(right.coords['z'], sc.concat([split, expected_stop], 'z'))


def test_slice_bins_by_half_open_int_range_splits_without_duplication() -> None:
    table = sc.data.table_xyz(100)
    table.coords['param'] = sc.arange(dim='row', start=0, stop=100, unit='s') // 10
    da = table.bin(x=10)
    split = sc.scalar(4, unit='s')
    left = da.bins['param', :split]
    right = da.bins['param', split:]
    assert left.bins.size().sum().value + right.bins.size().sum().value == 100
    assert sc.identical(
        left.coords['param'], sc.concat([da.bins.coords['param'].min(), split], 'param')
    )
    expected_stop = da.bins.coords['param'].max() + 1 * sc.Unit('s')
    assert sc.identical(
        right.coords['param'], sc.concat([split, expected_stop], 'param')
    )


def test_slice_bins_with_step_raises() -> None:
    da = sc.data.table_xyz(100).bin(x=10)
    start = sc.scalar(0.1, unit='m')
    stop = sc.scalar(0.4, unit='m')
    step = sc.scalar(0.1, unit='m')
    with pytest.raises(ValueError, match='Label-based indexing with step'):
        da.bins['z', start:stop:step]


def test_slice_bins_with_int_index_raises() -> None:
    da = sc.data.table_xyz(100).bin(x=10)
    with pytest.raises(
        ValueError, match='Bins can only by sliced using label-based indexing'
    ):
        da.bins['z', 1:4]
    with pytest.raises(ValueError, match='Unsupported key'):
        da.bins['z', 1]


def test_bins_slicing_open_start_too_small_stop_given() -> None:
    table = sc.data.table_xyz(nrow=100)
    da = table.bin(x=7)
    too_small_stop = da.bins.coords['x'].min() - 0.001 * sc.Unit('m')
    left = da.bins['x', :too_small_stop]
    assert left.bins.size().sum().value == 0
    assert sc.identical(
        left.coords['x'], sc.concat([too_small_stop, too_small_stop], 'x')
    )


def test_bins_slicing_open_stop_too_big_stop_given() -> None:
    table = sc.data.table_xyz(nrow=100)
    da = table.bin(x=7)
    too_big_start = da.bins.coords['x'].max() + 0.001 * sc.Unit('m')
    right = da.bins['x', too_big_start:]
    assert right.bins.size().sum().value == 0
    assert sc.identical(
        right.coords['x'], sc.concat([too_big_start, too_big_start], 'x')
    )
