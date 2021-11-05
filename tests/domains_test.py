# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
import scipp as sc
from scipp.core.domains import find_domains
import pytest

x = sc.arange(dim='x', start=0, stop=10)


def test_find_domains_fail_no_edges():
    data = sc.ones(dims=['x'], shape=[9], dtype='int64')
    da = sc.DataArray(data=data, coords={'x': x['x', 1:]})

    with pytest.raises(sc.DimensionError):
        find_domains(da)
    with pytest.raises(sc.DimensionError):
        find_domains(da, 'x')


def check_1d(da, expected):
    assert sc.identical(find_domains(da), expected)
    assert sc.identical(find_domains(da, 'x'), expected)
    with pytest.raises(sc.NotFoundError):
        find_domains(da, 'y')


def test_find_domains_1d_constant():
    data = sc.ones(dims=['x'], shape=[9], dtype='int64')
    da = sc.DataArray(data=data, coords={'x': x})

    expected = sc.DataArray(data=sc.array(dims=['x'], values=[1]),
                            coords={'x': sc.array(dims=['x'], values=[0, 9])})
    check_1d(da, expected)


def test_find_domains_1d_2_domains():
    data = sc.ones(dims=['x'], shape=[9], dtype='int64')
    da = sc.DataArray(data=data, coords={'x': x})
    expected = sc.DataArray(data=sc.array(dims=['x'], values=[0, 1]))

    da.data = sc.array(dims=['x'], values=[0, 1, 1, 1, 1, 1, 1, 1, 1])
    expected.coords['x'] = sc.array(dims=['x'], values=[0, 1, 9])
    check_1d(da, expected)

    da.data = sc.array(dims=['x'], values=[0, 0, 1, 1, 1, 1, 1, 1, 1])
    expected.coords['x'] = sc.array(dims=['x'], values=[0, 2, 9])
    check_1d(da, expected)

    da.data = sc.array(dims=['x'], values=[0, 0, 0, 0, 0, 0, 0, 0, 1])
    expected.coords['x'] = sc.array(dims=['x'], values=[0, 8, 9])
    check_1d(da, expected)


def test_find_domains_1d_3_domains():
    data = sc.ones(dims=['x'], shape=[9], dtype='int64')
    da = sc.DataArray(data=data, coords={'x': x})
    expected = sc.DataArray(data=sc.array(dims=['x'], values=[0, 1, 0]))

    da.data = sc.array(dims=['x'], values=[0, 1, 0, 0, 0, 0, 0, 0, 0])
    expected.coords['x'] = sc.array(dims=['x'], values=[0, 1, 2, 9])
    check_1d(da, expected)

    da.data = sc.array(dims=['x'], values=[0, 1, 1, 0, 0, 0, 0, 0, 0])
    expected.coords['x'] = sc.array(dims=['x'], values=[0, 1, 3, 9])
    check_1d(da, expected)

    da.data = sc.array(dims=['x'], values=[0, 1, 1, 1, 1, 1, 1, 1, 0])
    expected.coords['x'] = sc.array(dims=['x'], values=[0, 1, 8, 9])
    check_1d(da, expected)

    da.data = sc.array(dims=['x'], values=[0, 0, 1, 1, 1, 1, 1, 1, 0])
    expected.coords['x'] = sc.array(dims=['x'], values=[0, 2, 8, 9])
    check_1d(da, expected)


def test_find_domains_1d_4_domains():
    data = sc.ones(dims=['x'], shape=[9], dtype='int64')
    da = sc.DataArray(data=data, coords={'x': x})
    expected = sc.DataArray(data=sc.array(dims=['x'], values=[0, 1, 0, 1]))

    da.data = sc.array(dims=['x'], values=[0, 1, 0, 0, 0, 0, 0, 0, 1])
    expected.coords['x'] = sc.array(dims=['x'], values=[0, 1, 2, 8, 9])
    check_1d(da, expected)

    da.data = sc.array(dims=['x'], values=[0, 0, 1, 0, 0, 0, 0, 1, 1])
    expected.coords['x'] = sc.array(dims=['x'], values=[0, 2, 3, 7, 9])
    check_1d(da, expected)


def test_find_domains_1d_multicolor():
    data = sc.ones(dims=['x'], shape=[9], dtype='int64')
    da = sc.DataArray(data=data, coords={'x': x})
    expected = sc.DataArray(data=sc.array(dims=['x'], values=[0, 1, 2, 1]))

    da.data = sc.array(dims=['x'], values=[0, 1, 1, 2, 2, 2, 2, 2, 1])
    expected.coords['x'] = sc.array(dims=['x'], values=[0, 1, 3, 8, 9])
    check_1d(da, expected)

    da.data = sc.array(dims=['x'], values=[0, 2, 2, 2, 2, 2, 0, 2, 2])
    expected.data = sc.array(dims=['x'], values=[0, 2, 0, 2])
    expected.coords['x'] = sc.array(dims=['x'], values=[0, 1, 6, 7, 9])
    check_1d(da, expected)
