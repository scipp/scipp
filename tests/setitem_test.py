# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import scipp as sc


def test_setitem_required_for_inplace_ops() -> None:
    # Test that all required __setitem__ overloads for in-place operations
    # are available.

    var = sc.empty(dims=['x', 'y'], shape=[2, 3])
    var *= 1.5  # not setitem, just assigns python variable
    var['x', 1:] *= 1.5  # Variable.__setitem__
    var['x', 1:]['y', 1:] *= 1.5  # VariableView.__setitem__

    a = sc.DataArray(data=var)
    a *= 1.5  # not setitem, just assigns python variable
    a['x', 1:] *= 1.5  # DataArray.__setitem__
    a['x', 1:]['y', 1:] *= 1.5  # DataArrayView.__setitem__

    d = sc.Dataset(data={'a': var})
    d *= 1.5  # not setitem, just assigns python variable
    d['a'] *= 1.5  # Dataset.__setitem__(string)
    d['x', 1:] *= 1.5  # Dataset.__setitem__(slice)
    d['x', 1:]['a'] *= 1.5  # DatasetView.__setitem__(string)
    d['a']['x', 1:] *= 1.5  # DatasetView.__setitem__(slice)
    d['x', 1:]['y', 1:] *= 1.5  # DatasetView.__setitem__(slice)


def test_setitem_coords_required_for_inplace_ops() -> None:
    var = sc.zeros(dims=['x'], shape=(4,), dtype=sc.DType.int64)
    da = sc.DataArray(data=var)
    da.coords['x'] = var
    da.coords['x']['x', 2:] += 1
    assert sc.identical(
        da.coords['x'], sc.array(dims=['x'], dtype=sc.DType.int64, values=[0, 0, 1, 1])
    )
    ds = sc.Dataset(data={'a': da})
    ds.coords['x']['x', 2:] += 1
    assert sc.identical(
        ds.coords['x'], sc.array(dims=['x'], dtype=sc.DType.int64, values=[0, 0, 2, 2])
    )
