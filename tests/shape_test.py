# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @file

import numpy as np
import scipp as sc
from .common import assert_export


def test_concat():
    var = sc.scalar(1.0)
    assert sc.identical(sc.concat([var, var + var, 3 * var], 'x'),
                        sc.array(dims=['x'], values=[1.0, 2.0, 3.0]))


def test_fold():
    x = sc.array(dims=['x'], values=np.arange(6.0))
    da = sc.DataArray(x)
    assert_export(sc.fold, x=x, sizes={'x': 2, 'y': 3})
    assert_export(sc.fold, x=x, dims=['x', 'y'], shape=[2, 3])
    assert_export(sc.fold, x=da, sizes={'x': 2, 'y': 3})
    assert_export(sc.fold, x=da, dims=['x', 'y'], shape=[2, 3])


def test_flatten():
    x = sc.array(dims=['x', 'y'], values=np.arange(6.0).reshape(2, 3))
    da = sc.DataArray(x)
    assert_export(sc.flatten, x=x, dims=['x', 'y'], dim='z')
    assert_export(sc.flatten, x=x, dim='z')
    assert_export(sc.flatten, x=da, dims=['x', 'y'], dim='z')
    assert_export(sc.flatten, x=da, dim='z')
