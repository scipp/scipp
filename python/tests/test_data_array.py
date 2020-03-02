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


@pytest.mark.parametrize("dims, lengths",
                         ((['x'], (sc.Dimensions.Sparse, )),
                          (['x', 'y'], (10, sc.Dimensions.Sparse)),
                          (['x', 'y', 'z'], (10, 10, sc.Dimensions.Sparse)),
                          (['x', 'y', 'z', 'spectrum'],
                           (10, 10, 10, sc.Dimensions.Sparse))))
def test_sparse_dim_has_none_shape(dims, lengths):
    da = sc.DataArray(sc.Variable(dims, shape=lengths))

    assert da.shape[-1] is None


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
