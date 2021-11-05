# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
import scipp as sc
from scipp.core.domains import find_domains
import pytest

x = sc.arange(dim='x', start=0, stop=10)


def test_find_domains_fail_not_1d():
    data = sc.ones(dims=['y', 'x'], shape=[2, 9], dtype='int64')
    da = sc.DataArray(data=data, coords={'x': sc.concat([x, x], 'y')})
    with pytest.raises(sc.DimensionError):
        find_domains(da)


def test_find_domains_fail_no_edges():
    data = sc.ones(dims=['x'], shape=[9], dtype='int64')
    da = sc.DataArray(data=data, coords={'x': x['x', 1:]})
    with pytest.raises(sc.DimensionError):
        find_domains(da)


def test_find_domains_length_1():
    data = sc.ones(dims=['x'], shape=[1], dtype='int64')
    da = sc.DataArray(data=data, coords={'x': sc.array(dims=['x'], values=[0, 2])})
    assert sc.identical(find_domains(da), da)


def test_find_domains_length_2():
    data = sc.ones(dims=['x'], shape=[2], dtype='int64')
    da = sc.DataArray(data=data, coords={'x': sc.array(dims=['x'], values=[0, 1, 2])})
    expected = sc.DataArray(data=sc.array(dims=['x'], values=[1], dtype='int64'),
                            coords={'x': sc.array(dims=['x'], values=[0, 2])})
    assert sc.identical(find_domains(da), expected)


def test_find_domains_2_domains_length_2():
    data = sc.array(dims=['x'], values=[1, 0])
    da = sc.DataArray(data=data, coords={'x': sc.array(dims=['x'], values=[0, 1, 2])})
    assert sc.identical(find_domains(da), da)


def test_find_domains_constant():
    data = sc.ones(dims=['x'], shape=[9], dtype='int64')
    da = sc.DataArray(data=data, coords={'x': x})

    expected = sc.DataArray(data=sc.array(dims=['x'], values=[1], dtype='int64'),
                            coords={'x': sc.array(dims=['x'], values=[0, 9])})
    assert sc.identical(find_domains(da), expected)


def test_find_domains_2_domains():
    data = sc.ones(dims=['x'], shape=[9], dtype='int64')
    da = sc.DataArray(data=data, coords={'x': x})
    expected = sc.DataArray(data=sc.array(dims=['x'], values=[0, 1]))

    da.data = sc.array(dims=['x'], values=[0, 1, 1, 1, 1, 1, 1, 1, 1])
    expected.coords['x'] = sc.array(dims=['x'], values=[0, 1, 9])
    assert sc.identical(find_domains(da), expected)

    da.data = sc.array(dims=['x'], values=[0, 0, 1, 1, 1, 1, 1, 1, 1])
    expected.coords['x'] = sc.array(dims=['x'], values=[0, 2, 9])
    assert sc.identical(find_domains(da), expected)

    da.data = sc.array(dims=['x'], values=[0, 0, 0, 0, 0, 0, 0, 0, 1])
    expected.coords['x'] = sc.array(dims=['x'], values=[0, 8, 9])
    assert sc.identical(find_domains(da), expected)


def test_find_domains_3_domains():
    data = sc.ones(dims=['x'], shape=[9], dtype='int64')
    da = sc.DataArray(data=data, coords={'x': x})
    expected = sc.DataArray(data=sc.array(dims=['x'], values=[0, 1, 0]))

    da.data = sc.array(dims=['x'], values=[0, 1, 0, 0, 0, 0, 0, 0, 0])
    expected.coords['x'] = sc.array(dims=['x'], values=[0, 1, 2, 9])
    assert sc.identical(find_domains(da), expected)

    da.data = sc.array(dims=['x'], values=[0, 1, 1, 0, 0, 0, 0, 0, 0])
    expected.coords['x'] = sc.array(dims=['x'], values=[0, 1, 3, 9])
    assert sc.identical(find_domains(da), expected)

    da.data = sc.array(dims=['x'], values=[0, 1, 1, 1, 1, 1, 1, 1, 0])
    expected.coords['x'] = sc.array(dims=['x'], values=[0, 1, 8, 9])
    assert sc.identical(find_domains(da), expected)

    da.data = sc.array(dims=['x'], values=[0, 0, 1, 1, 1, 1, 1, 1, 0])
    expected.coords['x'] = sc.array(dims=['x'], values=[0, 2, 8, 9])
    assert sc.identical(find_domains(da), expected)


def test_find_domains_4_domains():
    data = sc.ones(dims=['x'], shape=[9], dtype='int64')
    da = sc.DataArray(data=data, coords={'x': x})
    expected = sc.DataArray(data=sc.array(dims=['x'], values=[0, 1, 0, 1]))

    da.data = sc.array(dims=['x'], values=[0, 1, 0, 0, 0, 0, 0, 0, 1])
    expected.coords['x'] = sc.array(dims=['x'], values=[0, 1, 2, 8, 9])
    assert sc.identical(find_domains(da), expected)

    da.data = sc.array(dims=['x'], values=[0, 0, 1, 0, 0, 0, 0, 1, 1])
    expected.coords['x'] = sc.array(dims=['x'], values=[0, 2, 3, 7, 9])
    assert sc.identical(find_domains(da), expected)


def test_find_domains_multicolor():
    data = sc.ones(dims=['x'], shape=[9], dtype='int64')
    da = sc.DataArray(data=data, coords={'x': x})
    expected = sc.DataArray(data=sc.array(dims=['x'], values=[0, 1, 2, 1]))

    da.data = sc.array(dims=['x'], values=[0, 1, 1, 2, 2, 2, 2, 2, 1])
    expected.coords['x'] = sc.array(dims=['x'], values=[0, 1, 3, 8, 9])
    assert sc.identical(find_domains(da), expected)

    da.data = sc.array(dims=['x'], values=[0, 2, 2, 2, 2, 2, 0, 2, 2])
    expected.data = sc.array(dims=['x'], values=[0, 2, 0, 2])
    expected.coords['x'] = sc.array(dims=['x'], values=[0, 1, 6, 7, 9])
    assert sc.identical(find_domains(da), expected)
