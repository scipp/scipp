# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import numpy as np
import pytest

import scipp as sc


def assert_variable_writeable(var):
    assert var.values.flags['WRITEABLE']
    if var.variances is not None:
        assert var.variances.flags['WRITEABLE']
    with pytest.raises(sc.VariancesError):
        var['x', 1].variances = None
    with pytest.raises(sc.UnitError):
        var['x', 1].unit = 'm'  # unit of slice is readonly
    assert var['x', 1].values.flags['WRITEABLE']
    if var.variances is not None:
        assert var['x', 1].variances.flags['WRITEABLE']
    var['x', 1] = var['x', 0]  # slice is writable
    assert sc.identical(var['x', 1], var['x', 0])


def assert_variable_readonly(var):
    original = var.copy()
    assert not var.values.flags['WRITEABLE']
    if var.variances is not None:
        assert not var.variances.flags['WRITEABLE']
    with pytest.raises(sc.VariableError):
        var.variances = None
    with pytest.raises(sc.VariableError):
        var.unit = 'm'
    assert not var['x', 1].values.flags['WRITEABLE']
    if var.variances is not None:
        assert not var['x', 1].variances.flags['WRITEABLE']
    with pytest.raises(sc.VariableError):
        var['x', 1] = var['x', 0]
    assert sc.identical(var, original)


def assert_readonly_data_array(da, readonly_data: bool):
    da2 = da['x', 1].copy(deep=False)
    var = sc.array(dims=['x'], values=np.arange(4))
    with pytest.raises(sc.DataArrayError):
        da['x', 1].data = var['x', 1]  # slice is readonly
    if readonly_data:
        assert_variable_readonly(da.data)
    else:
        da['x', 1].data += var['x', 1]  # slice is readonly but self-assign ok
        assert sc.identical(da.data, sc.array(dims=['x'], values=[0, 2, 2, 3]))
        da['x', 1].values = 1  # slice is readonly, but not the slice values
        assert sc.identical(da.data, sc.array(dims=['x'], values=[0, 1, 2, 3]))
        da2.values = 2  # values reference original
        assert sc.identical(da.data, sc.array(dims=['x'], values=[0, 2, 2, 3]))
        da2.data = var['x', 0]  # shallow-copy clears readonly flag...
        # ... but data setter sets new data, rather than overwriting original
        assert sc.identical(da.data, sc.array(dims=['x'], values=[0, 2, 2, 3]))
        assert_variable_writeable(da.data)


def test_readonly_variable_unit_and_variances():
    var = sc.array(dims=['x'], values=np.arange(4.), variances=np.arange(4.))
    assert_variable_writeable(var)


def test_readonly_variable():
    var = sc.broadcast(sc.scalar(value=1., variance=1.), dims=['x'], shape=[4])
    assert_variable_readonly(var)
    assert_variable_readonly(var.copy(deep=False))
    assert_variable_writeable(var.copy())


def _make_data_array():
    var = sc.array(dims=['x'], values=np.arange(4))
    scalar = sc.scalar(value=4)
    return sc.DataArray(data=var.copy(),
                        coords={
                            'x': var.copy(),
                            'scalar': scalar
                        },
                        masks={
                            'm': var.copy(),
                            'scalar': scalar
                        },
                        attrs={
                            'a': var.copy(),
                            'scalar_attr': scalar
                        })


def test_readonly_data_array():
    assert_readonly_data_array(_make_data_array(), readonly_data=False)


class TestReadonlyMetadata:
    def test_coords(self):
        da = _make_data_array()
        with pytest.raises(sc.DataArrayError):
            da['x', 1].coords['new'] = sc.scalar(4)
        assert 'new' not in da.coords
        with pytest.raises(sc.DataArrayError):
            del da['x', 1].coords['x']
        assert 'x' in da.coords

    def test_masks(self):
        da = _make_data_array()
        with pytest.raises(sc.DataArrayError):
            da['x', 1].masks['new'] = sc.scalar(4)
        assert 'new' not in da.masks
        with pytest.raises(sc.DataArrayError):
            del da['x', 1].masks['m']
        assert 'm' in da.masks

    def test_attrs(self):
        da = _make_data_array()
        with pytest.raises(sc.DataArrayError):
            da['x', 1].attrs['new'] = sc.scalar(4)
        assert 'new' not in da.attrs
        with pytest.raises(sc.DataArrayError):
            del da['x', 1].attrs['a']
        assert 'a' in da.attrs

    def test_broadcast_sets_readonly_flag(self):
        da = _make_data_array()
        da = sc.concatenate(da, da, 'y')
        assert_variable_readonly(da['y', 1].coords['x'])
        assert_variable_readonly(da['y', 1].masks['m'])
        assert_variable_readonly(da['y', 1].attrs['a'])


def test_readonly_dataset():
    ds = sc.Dataset({
        'a': _make_data_array(),
        'scalar': _make_data_array()['x', 0].copy()
    })
    with pytest.raises(sc.DatasetError):
        del ds['x', 0]['a']
    assert 'a' in ds
    with pytest.raises(sc.DatasetError):
        ds['x', 0]['b'] = ds['scalar'].copy()
    assert 'b' not in ds
    ds['b'] = sc.broadcast(ds['a'].data, dims=['y', 'x'], shape=[2, 4]).copy()
    assert_readonly_data_array(ds['y', 0]['b'], readonly_data=False)
    assert_readonly_data_array(ds['y', 0]['a'], readonly_data=True)
