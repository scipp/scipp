# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import numpy as np
import pytest

import scipp as sc


def make_dataarray(dim1='x', dim2='y', seed=None):
    if seed is not None:
        np.random.seed(seed)
    return sc.DataArray(
        data=sc.Variable(dims=[dim1, dim2], values=np.random.rand(2, 3)),
        coords={
            dim1: sc.Variable([dim1], values=np.arange(2.0), unit=sc.units.m),
            dim2: sc.Variable([dim2], values=np.arange(3.0), unit=sc.units.m),
            'aux': sc.Variable([dim2], values=np.random.rand(3))
        },
        attrs={'meta': sc.Variable([dim2], values=np.arange(3))})


def test_init():
    d = sc.DataArray(
        data=sc.Variable(dims=['x'], values=np.arange(3)),
        coords={
            'x': sc.Variable(['x'], values=np.arange(3), unit=sc.units.m),
            'lib1': sc.Variable(['x'], values=np.random.rand(3))
        },
        attrs={'met1': sc.Variable(['x'], values=np.arange(3))},
        masks={'mask1': sc.Variable(['x'], values=np.ones(3, dtype=np.bool))})
    assert len(d.attrs) == 1
    assert len(d.coords) == 2
    assert len(d.masks) == 1


def _is_deep_copy_of(orig, copy):
    assert orig == copy
    assert not id(orig) == id(copy)


def test_copy():
    import copy
    da = make_dataarray()
    _is_deep_copy_of(da, da.copy())
    _is_deep_copy_of(da, copy.copy(da))
    _is_deep_copy_of(da, copy.deepcopy(da))


def test_in_place_binary_with_variable():
    a = sc.DataArray(data=sc.Variable(['x'], values=np.arange(10.0)),
                     coords={'x': sc.Variable(['x'], values=np.arange(10.0))})
    copy = a.copy()

    a += 2.0 * sc.units.dimensionless
    a *= 2.0 * sc.units.m
    a -= 4.0 * sc.units.m
    a /= 2.0 * sc.units.m
    assert a == copy


def test_in_place_binary_with_scalar():
    a = sc.DataArray(data=sc.Variable(['x'], values=[10]),
                     coords={'x': sc.Variable(['x'], values=[10])})
    copy = a.copy()

    a += 2
    a *= 2
    a -= 4
    a /= 2
    assert a == copy


def test_rename_dims():
    d = make_dataarray('x', 'y', seed=0)
    d.rename_dims({'y': 'z'})
    assert d == make_dataarray('x', 'z', seed=0)
    d.rename_dims(dims_dict={'x': 'y', 'z': 'x'})
    assert d == make_dataarray('y', 'x', seed=0)


def test_setitem_works_for_view_and_array():
    a = make_dataarray('x', 'y', seed=0)
    a['x', :]['x', 0] = a['x', 1]
    a['x', 0] = a['x', 1]


def test_astype():
    a = sc.DataArray(data=sc.Variable(['x'],
                                      values=np.arange(10.0, dtype=np.int64)),
                     coords={'x': sc.Variable(['x'], values=np.arange(10.0))})
    assert a.dtype == sc.dtype.int64

    a_as_float = a.astype(sc.dtype.float32)
    assert a_as_float.dtype == sc.dtype.float32


def test_astype_bad_conversion():
    a = sc.DataArray(data=sc.Variable(['x'],
                                      values=np.arange(10.0, dtype=np.int64)),
                     coords={'x': sc.Variable(['x'], values=np.arange(10.0))})
    assert a.dtype == sc.dtype.int64

    with pytest.raises(RuntimeError):
        a.astype(sc.dtype.string)


def test_reciprocal():
    a = sc.DataArray(data=sc.Variable(['x'], values=np.array([5.0])))
    r = sc.reciprocal(a)
    assert r.values[0] == 1.0 / 5.0


def test_realign():
    co = sc.Variable(['x'], shape=[1], dtype=sc.dtype.event_list_float64)
    co.values[0].append(1.0)
    co.values[0].append(2.0)
    co.values[0].append(2.0)
    data = sc.Variable(['y'],
                       dtype=sc.dtype.float64,
                       values=np.array([1]),
                       variances=np.array([1]))
    da = sc.DataArray(data=data, coords={'x': co})
    da_r = sc.realign(
        da, {'x': sc.Variable(['x'], values=np.array([0.0, 1.0, 3.0]))})
    assert da_r.shape == [1, 2]
    assert da_r.unaligned == da
    assert np.allclose(sc.histogram(da_r).values, np.array([0, 3]), atol=1e-9)
