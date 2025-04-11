# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from collections.abc import MutableMapping

import numpy as np
import pytest

import scipp as sc


def assert_variable_writable(var: sc.Variable) -> None:
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


def assert_variable_readonly(var: sc.Variable) -> None:
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


def assert_dict_writable(d: MutableMapping[str, sc.Variable]) -> None:
    key = next(iter(d.keys()))
    del d[key]
    assert key not in d
    d['new'] = sc.scalar(4)
    assert 'new' in d


def assert_dict_readonly(
    d: MutableMapping[str, sc.Variable], error: type[RuntimeError] = sc.DataArrayError
) -> None:
    with pytest.raises(error):
        d['new'] = sc.scalar(4)
    assert 'new' not in d
    key = next(iter(d.keys()))
    with pytest.raises(error):
        del d[key]
    assert key in d


def assert_readonly_data_array(da: sc.DataArray, readonly_data: bool) -> None:
    N = da.sizes['x']
    da2 = da.copy(deep=False)
    var = sc.array(dims=['x'], values=np.arange(N))
    with pytest.raises(sc.DataArrayError):
        da.data = var  # slice is readonly
    assert_dict_readonly(da.coords)
    assert_dict_readonly(da.masks)
    if readonly_data:
        assert_variable_readonly(da.data)
        assert_variable_readonly(da.copy(deep=False).data)
        assert_variable_writable(da.copy().data)
    else:
        expected = da.data + var
        da.data += var  # slice is readonly but self-assign ok
        assert sc.identical(da.data, expected)
        vals = np.arange(1, N + 1)
        da.values = vals  # slice is readonly, but not values
        assert sc.identical(da.data, sc.array(dims=['x'], values=vals))
        vals = np.arange(2, N + 2)
        da2.values = vals  # values reference original
        assert sc.identical(da.data, sc.array(dims=['x'], values=vals))
        assert_variable_writable(da.data)
    da2.data = var + var  # shallow-copy clears readonly flag...
    # ... but data setter sets new data, rather than overwriting original
    assert not sc.identical(da2.data, da.data)


def test_readonly_variable_unit_and_variances() -> None:
    var = sc.array(dims=['x'], values=np.arange(4.0), variances=np.arange(4.0))
    assert_variable_writable(var)


def test_readonly_variable() -> None:
    var = sc.broadcast(sc.scalar(value=1.0, variance=1.0), dims=['x'], shape=[4])
    assert_variable_readonly(var)
    assert_variable_readonly(var.copy(deep=False))
    assert_variable_writable(var.copy())


def _make_data_array() -> sc.DataArray:
    var = sc.array(dims=['x'], values=np.arange(4))
    return sc.DataArray(
        data=var.copy(),
        coords={'x': var.copy()},
        masks={'m': var.copy()},
    )


def _make_dataset() -> sc.Dataset:
    return sc.Dataset(data={'a': _make_data_array()})


def test_readonly_data_array_slice() -> None:
    assert_readonly_data_array(_make_data_array()['x', 1:4], readonly_data=False)


def test_readonly_metadata() -> None:
    da = _make_data_array()
    assert_dict_readonly(da['x', 1:2].coords)
    assert_dict_readonly(da['x', 1:2].masks)
    # Shallow copy makes dict writable
    assert_dict_writable(da['x', 1:2].copy(deep=False).coords)
    assert_dict_writable(da['x', 1:2].copy(deep=False).masks)


def test_readonly_metadata_broadcast_sets_readonly_flag() -> None:
    da = _make_data_array()
    da = sc.concat([da, da], 'y')
    assert_variable_readonly(da['y', 1].coords['x'])
    assert_variable_readonly(da['y', 1].masks['m'])
    # Shallow copy makes dict writable but not the items (buffers)
    assert_variable_readonly(da['y', 1].copy(deep=False).coords['x'])
    assert_variable_readonly(da['y', 1].copy(deep=False).masks['m'])
    # Deep copy copies buffers
    assert_variable_writable(da['y', 1].copy().coords['x'])
    assert_variable_writable(da['y', 1].copy().masks['m'])


def test_dataset_readonly_metadata_dicts() -> None:
    ds = _make_dataset()
    # Coords dicts are shared and thus readonly
    assert_dict_readonly(ds['a'].coords)
    assert_dict_writable(ds['a'].masks)


def test_dataset_readonly_metadata_items() -> None:
    ds = _make_dataset()
    # Coords are shared and thus readonly
    assert_variable_readonly(ds['a'].coords['x'])
    assert_variable_writable(ds['a'].masks['m'])


def test_readonly_dataset_slice() -> None:
    ds = _make_dataset()
    assert_dict_readonly(ds['x', 0], sc.DatasetError)
    assert_dict_writable(ds['x', 0].copy(deep=False))


def test_readonly_dataset_slice_items() -> None:
    ds = sc.Dataset(
        {
            key: val.broadcast(dims=['x', 'y'], shape=[4, 3])
            for key, val in _make_dataset().items()
        }
    )
    assert_readonly_data_array(ds['y', 0]['a'], readonly_data=True)
