# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock

import numpy as np
import scipp as sc

dim = 'yy'
var = sc.array(dims=[dim, 'xx'], values=np.random.rand(4, 12), unit='m')


def _slices(obj):
    return [obj[dim, i] for i in range(obj.sizes[dim])]


def test_reduce_logical():
    v = var < var.mean()
    args = _slices(v)
    assert sc.identical(sc.reduce(args).all(), sc.all(v, 'yy'))
    assert sc.identical(sc.reduce(args).any(), sc.any(v, 'yy'))


def test_reduce():
    args = _slices(var)
    assert sc.identical(sc.reduce(args).max(), sc.max(var, 'yy'))
    assert sc.identical(sc.reduce(args).min(), sc.min(var, 'yy'))
    assert sc.identical(sc.reduce(args).sum(), sc.sum(var, 'yy'))
    assert sc.identical(sc.reduce(args).mean(), sc.mean(var, 'yy'))


def test_reduce_nan():
    var.values[1, 1] = np.NAN
    args = _slices(var)
    assert sc.identical(sc.reduce(args).nanmax(), sc.nanmax(var, 'yy'))
    assert sc.identical(sc.reduce(args).nanmin(), sc.nanmin(var, 'yy'))
    assert sc.identical(sc.reduce(args).nansum(), sc.nansum(var, 'yy'))
    assert sc.identical(sc.reduce(args).nanmean(), sc.nanmean(var, 'yy'))
