# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import pytest
import scipp as sc


def test_op_with_implicit_variance_broadcast_raises():
    var = sc.linspace('x', 0.0, 1.0, 4)
    scalar = sc.scalar(1.0, variance=0.1)
    with pytest.raises(sc.VariancesError):
        var + scalar


def test_op_with_implicit_variance_broadcast_to_bins_raises():
    var = sc.ones(dims=['x'], shape=[4], with_variances=True)
    da = sc.data.binned_x(100, 4)
    with pytest.raises(sc.VariancesError):
        da + var


def test_op_with_explicit_variance_broadcast_raises():
    var = sc.linspace('x', 0.0, 1.0, 4)
    scalar = sc.scalar(1.0, variance=0.1)
    scalar = scalar.broadcast(sizes=var.sizes)
    with pytest.raises(sc.VariancesError):
        var + scalar


def test_can_copy_broadcast_with_variances():
    var = sc.scalar(1.0, variance=0.1).broadcast(sizes={'x': 4})
    assert sc.identical(var.copy(), var)
