# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import numpy as np

import scipp as sc


def test_all() -> None:
    var = sc.array(dims=['x', 'y'], values=[[True, True], [True, False]])
    assert sc.identical(sc.all(var), sc.scalar(False))


def test_all_with_dim() -> None:
    var = sc.array(dims=['x', 'y'], values=[[True, True], [True, False]])
    assert sc.identical(sc.all(var, 'x'), sc.array(dims=['y'], values=[True, False]))
    assert sc.identical(sc.all(var, 'y'), sc.array(dims=['x'], values=[True, False]))


def test_any() -> None:
    var = sc.array(dims=['x', 'y'], values=[[True, True], [True, False]])
    assert sc.identical(sc.any(var), sc.scalar(True))


def test_any_with_dim() -> None:
    var = sc.array(dims=['x', 'y'], values=[[True, True], [False, False]])
    assert sc.identical(sc.any(var, 'x'), sc.array(dims=['y'], values=[True, True]))
    assert sc.identical(sc.any(var, 'y'), sc.array(dims=['x'], values=[True, False]))


def test_min() -> None:
    var = sc.array(dims=['x'], values=[1.0, 2.0, 3.0])
    assert sc.identical(sc.min(var, 'x'), sc.scalar(1.0))


def test_max() -> None:
    var = sc.array(dims=['x'], values=[1.0, 2.0, 3.0])
    assert sc.identical(sc.max(var, 'x'), sc.scalar(3.0))


def test_nanmin() -> None:
    var = sc.array(dims=['x'], values=[np.nan, 2.0, 3.0])
    assert sc.identical(sc.nanmin(var, 'x'), sc.scalar(2.0))


def test_nanmax() -> None:
    var = sc.array(dims=['x'], values=[1.0, 2.0, np.nan])
    assert sc.identical(sc.nanmax(var, 'x'), sc.scalar(2.0))


def test_sum() -> None:
    var = sc.array(dims=['x', 'y'], values=np.arange(4.0).reshape(2, 2))
    assert sc.identical(sc.sum(var), sc.scalar(6.0))
    assert sc.identical(sc.sum(var, 'x'), sc.array(dims=['y'], values=[2.0, 4.0]))


def test_nansum() -> None:
    var = sc.array(dims=['x', 'y'], values=[[1.0, 1.0], [1.0, np.nan]])
    assert sc.identical(sc.nansum(var), sc.scalar(3.0))
    assert sc.identical(sc.nansum(var, 'x'), sc.array(dims=['y'], values=[2.0, 1.0]))


def test_mean() -> None:
    var = sc.arange('r', 4.0).fold('r', sizes={'x': 2, 'y': 2})
    assert sc.identical(sc.mean(var), sc.scalar(6.0 / 4))
    assert sc.identical(sc.mean(var, 'x'), sc.array(dims=['y'], values=[1.0, 2.0]))


def test_nanmean() -> None:
    var = sc.array(dims=['x', 'y'], values=[[1.0, 1.0], [1.0, 1.0]])
    assert sc.identical(sc.nanmean(var), sc.scalar(3.0 / 3))
    assert sc.identical(sc.nanmean(var, 'x'), sc.array(dims=['y'], values=[1.0, 1.0]))


def test_sum_multiple_dims() -> None:
    values = np.arange(24).reshape(2, 3, 4)
    var = sc.array(dims=['x', 'y', 'z'], values=values)
    assert sc.identical(
        var.sum(('x', 'y')), sc.array(dims=['z'], values=values.sum(axis=(0, 1)))
    )
    assert sc.identical(
        var.sum(('x', 'z')), sc.array(dims=['y'], values=values.sum(axis=(0, 2)))
    )
    assert sc.identical(
        var.sum(('z', 'x')), sc.array(dims=['y'], values=values.sum(axis=(0, 2)))
    )
    assert sc.identical(var.sum(()), var)
    assert sc.identical(var.sum(('x',)), var.sum('x'))
    assert sc.identical(var.sum(('x', 'y', 'z')), var.sum())
