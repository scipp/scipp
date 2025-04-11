# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
import pytest

import scipp as sc
from scipp.core.domains import merge_equal_adjacent

x = sc.arange(dim='x', start=0, stop=10)


def test_merge_equal_adjacent_fail_not_1d() -> None:
    data = sc.ones(dims=['y', 'x'], shape=[2, 9], dtype='int64')
    da = sc.DataArray(data=data, coords={'x': sc.concat([x, x], 'y')})
    with pytest.raises(sc.DimensionError):
        merge_equal_adjacent(da)


def test_merge_equal_adjacent_fail_no_edges() -> None:
    data = sc.ones(dims=['x'], shape=[9], dtype='int64')
    da = sc.DataArray(data=data, coords={'x': x['x', 1:]})
    with pytest.raises(sc.DimensionError):
        merge_equal_adjacent(da)


def test_merge_equal_adjacent_length_1() -> None:
    data = sc.ones(dims=['x'], shape=[1], dtype='int64')
    da = sc.DataArray(data=data, coords={'x': sc.array(dims=['x'], values=[0, 2])})
    assert sc.identical(merge_equal_adjacent(da), da)


def test_merge_equal_adjacent_length_2() -> None:
    data = sc.ones(dims=['x'], shape=[2], dtype='int64')
    da = sc.DataArray(data=data, coords={'x': sc.array(dims=['x'], values=[0, 1, 2])})
    expected = sc.DataArray(
        data=sc.array(dims=['x'], values=[1], dtype='int64'),
        coords={'x': sc.array(dims=['x'], values=[0, 2])},
    )
    assert sc.identical(merge_equal_adjacent(da), expected)


def test_merge_equal_adjacent_2_domains_length_2() -> None:
    data = sc.array(dims=['x'], values=[1, 0])
    da = sc.DataArray(data=data, coords={'x': sc.array(dims=['x'], values=[0, 1, 2])})
    assert sc.identical(merge_equal_adjacent(da), da)


def test_merge_equal_adjacent_constant() -> None:
    data = sc.ones(dims=['x'], shape=[9], dtype='int64')
    da = sc.DataArray(data=data, coords={'x': x})

    expected = sc.DataArray(
        data=sc.array(dims=['x'], values=[1], dtype='int64'),
        coords={'x': sc.array(dims=['x'], values=[0, 9])},
    )
    assert sc.identical(merge_equal_adjacent(da), expected)


def test_merge_equal_adjacent_2_domains() -> None:
    data = sc.ones(dims=['x'], shape=[9], dtype='int64')
    da = sc.DataArray(data=data, coords={'x': x})
    expected = sc.DataArray(data=sc.array(dims=['x'], values=[0, 1]))

    da.data = sc.array(dims=['x'], values=[0, 1, 1, 1, 1, 1, 1, 1, 1])
    expected.coords['x'] = sc.array(dims=['x'], values=[0, 1, 9])
    assert sc.identical(merge_equal_adjacent(da), expected)

    da.data = sc.array(dims=['x'], values=[0, 0, 1, 1, 1, 1, 1, 1, 1])
    expected.coords['x'] = sc.array(dims=['x'], values=[0, 2, 9])
    assert sc.identical(merge_equal_adjacent(da), expected)

    da.data = sc.array(dims=['x'], values=[0, 0, 0, 0, 0, 0, 0, 0, 1])
    expected.coords['x'] = sc.array(dims=['x'], values=[0, 8, 9])
    assert sc.identical(merge_equal_adjacent(da), expected)


def test_merge_equal_adjacent_3_domains() -> None:
    data = sc.ones(dims=['x'], shape=[9], dtype='int64')
    da = sc.DataArray(data=data, coords={'x': x})
    expected = sc.DataArray(data=sc.array(dims=['x'], values=[0, 1, 0]))

    da.data = sc.array(dims=['x'], values=[0, 1, 0, 0, 0, 0, 0, 0, 0])
    expected.coords['x'] = sc.array(dims=['x'], values=[0, 1, 2, 9])
    assert sc.identical(merge_equal_adjacent(da), expected)

    da.data = sc.array(dims=['x'], values=[0, 1, 1, 0, 0, 0, 0, 0, 0])
    expected.coords['x'] = sc.array(dims=['x'], values=[0, 1, 3, 9])
    assert sc.identical(merge_equal_adjacent(da), expected)

    da.data = sc.array(dims=['x'], values=[0, 1, 1, 1, 1, 1, 1, 1, 0])
    expected.coords['x'] = sc.array(dims=['x'], values=[0, 1, 8, 9])
    assert sc.identical(merge_equal_adjacent(da), expected)

    da.data = sc.array(dims=['x'], values=[0, 0, 1, 1, 1, 1, 1, 1, 0])
    expected.coords['x'] = sc.array(dims=['x'], values=[0, 2, 8, 9])
    assert sc.identical(merge_equal_adjacent(da), expected)


def test_merge_equal_adjacent_4_domains() -> None:
    data = sc.ones(dims=['x'], shape=[9], dtype='int64')
    da = sc.DataArray(data=data, coords={'x': x})
    expected = sc.DataArray(data=sc.array(dims=['x'], values=[0, 1, 0, 1]))

    da.data = sc.array(dims=['x'], values=[0, 1, 0, 0, 0, 0, 0, 0, 1])
    expected.coords['x'] = sc.array(dims=['x'], values=[0, 1, 2, 8, 9])
    assert sc.identical(merge_equal_adjacent(da), expected)

    da.data = sc.array(dims=['x'], values=[0, 0, 1, 0, 0, 0, 0, 1, 1])
    expected.coords['x'] = sc.array(dims=['x'], values=[0, 2, 3, 7, 9])
    assert sc.identical(merge_equal_adjacent(da), expected)


def test_merge_equal_adjacent_multicolor() -> None:
    data = sc.ones(dims=['x'], shape=[9], dtype='int64')
    da = sc.DataArray(data=data, coords={'x': x})
    expected = sc.DataArray(data=sc.array(dims=['x'], values=[0, 1, 2, 1]))

    da.data = sc.array(dims=['x'], values=[0, 1, 1, 2, 2, 2, 2, 2, 1])
    expected.coords['x'] = sc.array(dims=['x'], values=[0, 1, 3, 8, 9])
    assert sc.identical(merge_equal_adjacent(da), expected)

    da.data = sc.array(dims=['x'], values=[0, 2, 2, 2, 2, 2, 0, 2, 2])
    expected.data = sc.array(dims=['x'], values=[0, 2, 0, 2])
    expected.coords['x'] = sc.array(dims=['x'], values=[0, 1, 6, 7, 9])
    assert sc.identical(merge_equal_adjacent(da), expected)
