# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import pytest
import scipp as sc


def make_variable() -> sc.Variable:
    v = sc.arange('dummy', 12, dtype='int64')
    return v.fold(dim='dummy', sizes={'xx': 4, 'yy': 3})


def make_array() -> sc.DataArray:
    da = sc.DataArray(make_variable())
    da.coords['xx'] = sc.arange('xx', 4, dtype='int64')
    da.coords['yy'] = sc.arange('yy', 3, dtype='int64')
    return da


def make_dataset() -> sc.Dataset:
    ds = sc.Dataset()
    ds['xy'] = make_array()
    ds['x'] = ds.coords['xx']
    return ds


@pytest.fixture
def variable_xx4_yy3():
    return make_variable()


@pytest.fixture
def data_array_xx4_yy3():
    return make_array()


@pytest.fixture
def dataset_xx4_yy3():
    return make_dataset()


@pytest.fixture(params=[make_variable(), make_array(),
                        make_dataset()],
                ids=['Variable', 'DataArray', 'Dataset'])
def sliceable(request):
    return request.param


@pytest.fixture(params=[make_array(), make_dataset()], ids=['DataArray', 'Dataset'])
def labeled_sliceable(request):
    return request.param
