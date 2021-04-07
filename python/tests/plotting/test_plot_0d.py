# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Owen Arnold

from scipp import plot
import scipp as sc
import pytest


def test_plot_0d_variable():
    with pytest.raises(ValueError):
        plot(sc.Variable(value=1)).close()


def test_plot_0d_dataarray():
    da = sc.DataArray(data=sc.Variable(value=1))
    with pytest.raises(ValueError):
        plot(da).close()


def test_plot_0d_dataset():
    ds = sc.Dataset(data={'a': sc.Variable(value=1)})
    with pytest.raises(ValueError):
        plot(ds).close()


def test_plot_not_all_0d_dataset():
    ds = sc.Dataset(data={
        'a': sc.Variable(value=1),
        'b': sc.Variable(['x'], values=[1, 2])
    })
    plot(ds).close()  # Throw nothing


def test_dict_of_0d_variables():
    with pytest.raises(ValueError):
        plot({'a': sc.Variable(value=1)}).close()


def test_dict_of_0d_data_array():
    with pytest.raises(ValueError):
        plot({'a': sc.DataArray(data=sc.Variable(value=1))}).close()


def test_dict_of_0d_dataset():
    with pytest.raises(ValueError):
        plot({'a': sc.Dataset(data={'x': sc.Variable(value=1)})}).close()


def test_dict_of_nd_variables():
    # 0D and 1D is OK
    plot({
        'a': sc.Variable(value=1),
        'b': sc.Variable(['x'], values=[1, 2])
    }).close()


def test_dict_of_nd_data_array():
    # 0D and 1D is OK
    plot({
        'a': sc.DataArray(data=sc.Variable(value=1)),
        'b': sc.DataArray(data=sc.Variable(['x'], values=[1, 2]))
    }).close()


def test_dict_of_nd_dataset():
    # 0D and 1D is OK
    ds1 = sc.Dataset(data={'a': sc.Variable(['x'], values=[1, 2])})
    ds2 = sc.Dataset(data={
        'a': sc.Variable(value=1),
    })
    plot({'x': ds1, 'y': ds2}).close()
