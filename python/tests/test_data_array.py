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


def test_slice_init():
    orig = sc.DataArray(
        data=sc.Variable(['x'], values=np.arange(2.0)),
        coords={'x': sc.Variable(['x'], values=np.arange(3.0))})
    a = sc.DataArray(orig['x', :])
    assert a == orig
    b = sc.DataArray(orig['x', 1:])
    assert b.data.values[0] == orig.data.values[1:]


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


def test_init_from_variable_views():
    var = sc.Variable(['x'], values=np.arange(5))
    a = sc.DataArray(data=var,
                     coords={'x': var},
                     attrs={'meta': var},
                     masks={'mask1': sc.less(var, sc.Variable(value=3))})
    b = sc.DataArray(data=a.data,
                     coords={'x': a.coords['x']},
                     attrs={'meta': a.attrs['meta']},
                     masks={'mask1': a.masks['mask1']})
    assert a == b

    # Ensure mix of Variables and Variable views work
    c = sc.DataArray(data=a.data,
                     coords={'x': var},
                     attrs={'meta': a.attrs['meta']},
                     masks={'mask1': a.masks['mask1']})

    assert a == c


def test_coords():
    da = make_dataarray()
    assert len(dict(da.coords)) == 3
    assert 'x' in da.coords
    assert 'y' in da.coords
    assert 'aux' in da.coords


def test_attrs():
    da = make_dataarray()
    assert len(dict(da.attrs)) == 1
    assert 'meta' in da.attrs


def test_masks():
    da = make_dataarray()
    da.masks['mask1'] = sc.Variable(['x'],
                                    values=np.array([False, True],
                                                    dtype=np.bool))
    assert len(dict(da.masks)) == 1
    assert 'mask1' in da.masks


def test_labels():
    da = make_dataarray()
    # Deprecated at point of use
    with pytest.raises(RuntimeError):
        da.labels


def test_eq():
    da = make_dataarray()
    assert da['x', :] == da
    assert da['y', :] == da
    assert da['y', :]['x', :] == da
    assert not da['y', 1:] == da
    assert not da['x', 1:] == da
    assert not da['y', 1:]['x', :] == da
    assert not da['y', :]['x', 1:] == da


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


def test_in_place_binary_with_dataarray():
    da = sc.DataArray(
        data=sc.Variable(['x'], values=np.arange(1.0, 10.0)),
        coords={'x': sc.Variable(['x'], values=np.arange(1.0, 10.0))})
    orig = da.copy()
    da += orig
    da -= orig
    da *= orig
    da /= orig
    assert da == orig


def test_in_place_binary_with_scalar():
    a = sc.DataArray(data=sc.Variable(['x'], values=[10]),
                     coords={'x': sc.Variable(['x'], values=[10])})
    copy = a.copy()

    a += 2
    a *= 2
    a -= 4
    a /= 2
    assert a == copy


def test_binary_with_broadcast():
    da = sc.DataArray(data=sc.Variable(['x', 'y'],
                                       values=np.arange(20).reshape(5, 4)),
                      coords={
                          'x': sc.Variable(['x'],
                                           values=np.arange(0.0, 0.6, 0.1)),
                          'y': sc.Variable(['y'],
                                           values=np.arange(0.0, 0.5, 0.1))
                      })
    d2 = da - da['x', 0]
    da -= da['x', 0]
    assert da == d2


def test_view_in_place_binary_with_scalar():
    d = sc.Dataset({'data': sc.Variable(dims=['x'], values=[10])},
                   coords={'x': sc.Variable(dims=['x'], values=[10])})
    copy = d.copy()

    d['x', :] += 2
    d['x', :] *= 2
    d['x', :] -= 4
    d['x', :] /= 2
    assert d == copy


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
    assert not da.unaligned
    da_r = sc.realign(
        da, {'x': sc.Variable(['x'], values=np.array([0.0, 1.0, 3.0]))})
    assert da_r.shape == [1, 2]
    assert da_r.unaligned == da
    assert not da_r.data
    assert np.allclose(sc.histogram(da_r).values, np.array([0, 3]), atol=1e-9)
    da.realign({'x': sc.Variable(['x'], values=np.array([0.0, 1.0, 3.0]))})
    assert da.shape == [1, 2]
