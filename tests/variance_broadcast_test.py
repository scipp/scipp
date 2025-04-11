# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import pytest

import scipp as sc


def test_op_with_implicit_variance_broadcast_raises() -> None:
    var = sc.linspace('x', 0.0, 1.0, 4)
    scalar = sc.scalar(1.0, variance=0.1)
    with pytest.raises(sc.VariancesError):
        var + scalar


def test_op_with_two_implicit_variance_broadcasts_raises() -> None:
    v1 = sc.ones(sizes={'x': 5}, with_variances=True)
    v2 = sc.ones(sizes={'y': 4}, with_variances=True)
    with pytest.raises(sc.VariancesError):
        v1 * v2


def test_op_with_length_zero_dim_and_matching_shape_works() -> None:
    # This leads to a stride of 0, which might lead the implementation to assume a
    # broadcast is happening.
    var = sc.ones(dims=['x', 'y'], shape=(1, 0), with_variances=True)
    var + var


def test_op_with_length_zero_dim_and_mismatching_shape_fails() -> None:
    var = sc.ones(dims=['x', 'y'], shape=(1, 0), with_variances=True)
    with pytest.raises(sc.VariancesError):
        var + var['x', 0]


def test_inplace_op_with_implicit_variance_broadcast_raises() -> None:
    var = sc.ones(dims=['x'], shape=[4], with_variances=True)
    scalar = sc.scalar(1.0, variance=0.1)
    with pytest.raises(sc.VariancesError):
        var += scalar


def test_op_with_implicit_variance_broadcast_to_bins_raises() -> None:
    var = sc.ones(dims=['x'], shape=[4], with_variances=True)
    da = sc.data.binned_x(100, 4)
    with pytest.raises(sc.VariancesError):
        da * var


def test_inplace_op_with_implicit_variance_broadcast_to_bins_raises() -> None:
    var = sc.ones(dims=['x'], shape=[4], with_variances=True)
    table = sc.data.table_xyz(100)
    table.variances = table.values
    da = table.bin(x=4)
    with pytest.raises(sc.VariancesError):
        da *= var


def test_op_with_implicit_variance_broadcast_of_bins_raises() -> None:
    var = sc.ones(dims=['y'], shape=[4])
    table = sc.data.table_xyz(100)
    table.variances = table.values
    da = table.bin(x=2)
    with pytest.raises(sc.VariancesError):
        da * var


def test_inplace_op_with_implicit_variance_broadcast_of_bins_raises() -> None:
    table = sc.data.table_xyz(100)
    table.variances = table.values
    x = table.bin(x=4)
    xy = x.broadcast(sizes={'x': 4, 'y': 2}).copy()
    with pytest.raises(sc.VariancesError):
        xy + x


def test_broadcast_of_dense_without_variances_to_bins_works() -> None:
    table = sc.data.table_xyz(100)
    table.variances = table.values
    binned = table.bin(x=4)
    dense = sc.full(sizes=binned.sizes, value=2.0)
    assert sc.identical(dense * binned, binned * 2.0)


def test_op_with_explicit_variance_broadcast_raises() -> None:
    var = sc.linspace('x', 0.0, 1.0, 4)
    scalar = sc.scalar(1.0, variance=0.1)
    scalar = scalar.broadcast(sizes=var.sizes)
    with pytest.raises(sc.VariancesError):
        var + scalar


def test_inplace_op_with_explicit_variance_broadcast_raises() -> None:
    var = sc.ones(dims=['x'], shape=[4], with_variances=True)
    scalar = sc.scalar(1.0, variance=0.1)
    scalar = scalar.broadcast(sizes=var.sizes)
    with pytest.raises(sc.VariancesError):
        var += scalar


def test_can_copy_broadcast_with_variances() -> None:
    var = sc.scalar(1.0, variance=0.1).broadcast(sizes={'x': 4})
    assert sc.identical(var.copy(), var)


def test_can_extract_values_of_broadcast_with_variances() -> None:
    var = sc.scalar(1.0, variance=0.1).broadcast(sizes={'x': 4})
    assert sc.identical(sc.values(var), sc.ones(dims=['x'], shape=[4]))


def test_can_extract_variances_of_broadcast_with_variances() -> None:
    var = sc.scalar(1.0, variance=4.0).broadcast(sizes={'x': 4})
    assert sc.identical(sc.variances(var), sc.full(dims=['x'], shape=[4], value=4.0))


def test_can_extract_stddevs_of_broadcast_with_variances() -> None:
    var = sc.scalar(1.0, variance=4.0).broadcast(sizes={'x': 4})
    assert sc.identical(sc.stddevs(var), sc.full(dims=['x'], shape=[4], value=2.0))


def test_scale_binned_via_lookup_with_variances_raises() -> None:
    table = sc.data.table_xyz(100)
    da = table.bin(x=2)
    hist = table.hist(x=7)
    hist.variances = hist.values
    func = sc.lookup(func=hist, dim='x')
    with pytest.raises(sc.VariancesError):
        da.bins /= func  # type: ignore[assignment, operator]


def test_lookup_with_variances_raises() -> None:
    table = sc.data.table_xyz(100)
    hist = table.hist(x=7)
    hist.variances = hist.values
    func = sc.lookup(func=hist, dim='x')
    with pytest.raises(sc.VariancesError):
        func(table.coords['x'])
