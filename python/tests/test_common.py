# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import pytest
import scipp as sc


def test_bool_raises():
    # Truth values of arrays are undefined
    var = sc.Variable(value=True)
    with pytest.raises(RuntimeError):
        var and var
    da = sc.DataArray(var)
    with pytest.raises(RuntimeError):
        da and da
    ds = sc.Dataset({'a': da})
    with pytest.raises(RuntimeError):
        ds and ds
