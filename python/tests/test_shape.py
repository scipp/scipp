# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file

import numpy as np
import scipp as sc
from .common import assert_export


def test_reshape_variable():
    x = sc.array(dims=['x'], values=np.arange(6.0))
    assert_export(sc.reshape, x=x, sizes={'x': 2, 'y': 3})
    assert_export(sc.reshape, x=x, dims=['x', 'y'], shape=[2, 3])


def test_split_data_array():
    x = sc.DataArray(sc.array(dims=['x'], values=np.arange(6.0)))
    assert_export(sc.split, x=x, sizes={'x': 2, 'y': 3})
    assert_export(sc.split, x=x, dims=['x', 'y'], shape=[2, 3])


def test_flatten_data_array():
    x = sc.DataArray(
        sc.array(dims=['x', 'y'], values=np.arange(6.0).reshape(2, 3)))
    assert_export(sc.flatten, x=x, dims=['x', 'y'], dim='z')
    assert_export(sc.flatten, x=x, dim='z')
