# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import numpy as np
import pytest

import scipp as sc


def test_readonly_variable_unit_and_variances():
    var = sc.array(dims=['x'], values=np.arange(4.), variances=np.arange(4.))
    assert var.values.flags['WRITEABLE']
    assert var.variances.flags['WRITEABLE']
    with pytest.raises(sc.VariancesError):
        var['x', 1].variances = None
    with pytest.raises(sc.UnitError):
        var['x', 1].unit = 'm'  # unit of slice is readonly
    assert var['x', 1].values.flags['WRITEABLE']
    assert var['x', 1].variances.flags['WRITEABLE']
    var['x', 1] = var['x', 0]  # slice is writable
    assert sc.identical(var['x', 1], var['x', 0])


def test_readonly_variable():
    var = sc.broadcast(sc.scalar(value=1., variance=1.), dims=['x'], shape=[4])
    original = var.copy()
    assert not var.values.flags['WRITEABLE']
    assert not var.variances.flags['WRITEABLE']
    with pytest.raises(sc.VariableError):
        var['x', 1].variances = None
    with pytest.raises(sc.VariableError):
        var['x', 1].unit = 'm'
    assert not var['x', 1].values.flags['WRITEABLE']
    assert not var['x', 1].variances.flags['WRITEABLE']
    with pytest.raises(sc.VariableError):
        var['x', 1] = var['x', 0]
    assert sc.identical(var, original)
    shallow = var.copy(deep=False)
    assert not shallow.values.flags['WRITEABLE']
    assert not shallow.variances.flags['WRITEABLE']
    deep = var.copy()
    assert deep.values.flags['WRITEABLE']
    assert deep.variances.flags['WRITEABLE']


def test_readonly():
    var = sc.array(dims=['x'], values=np.arange(4))
    da = sc.DataArray(data=var.copy(),
                      coords={'x': var.copy()},
                      masks={'m': var.copy()},
                      attrs={'a': var.copy()})
    with pytest.raises(sc.DataArrayError):
        da['x', 1].data = var['x', 1]  # slice is readonly
    da['x', 1].data += var['x', 1]  # slice is readonly but self-assign ok
    assert sc.identical(da.data, sc.array(dims=['x'], values=[0, 2, 2, 3]))
    da['x', 1].values = 1  # slice is readonly, but not the slice values
    assert sc.identical(da.data, sc.array(dims=['x'], values=[0, 1, 2, 3]))
    da2 = da['x', 1].copy(deep=False)
    da2.values = 2  # values reference original
    assert sc.identical(da.data, sc.array(dims=['x'], values=[0, 2, 2, 3]))
    da2.data = var['x', 0]  # shallow-copy clears readonly flag...
    # ... but data setter sets new data, rather than overwriting original
    assert sc.identical(da.data, sc.array(dims=['x'], values=[0, 2, 2, 3]))
