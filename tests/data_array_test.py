# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import numpy as np
import pytest

import scipp as sc


def make_dataarray(dim1='x', dim2='y', seed=None):
    if seed is not None:
        np.random.seed(seed)
    return sc.DataArray(data=sc.Variable(dims=[dim1, dim2], values=np.random.rand(2,
                                                                                  3)),
                        coords={
                            dim1: sc.Variable(dims=[dim1],
                                              values=np.arange(2.0),
                                              unit=sc.units.m),
                            dim2: sc.Variable(dims=[dim2],
                                              values=np.arange(3.0),
                                              unit=sc.units.m),
                            'aux': sc.Variable(dims=[dim2], values=np.random.rand(3))
                        },
                        attrs={'meta': sc.Variable(dims=[dim2], values=np.arange(3))})


def test_slice_init():
    orig = sc.DataArray(data=sc.Variable(dims=['x'], values=np.arange(2.0)),
                        coords={'x': sc.Variable(dims=['x'], values=np.arange(3.0))})
    a = orig['x', :].copy()
    assert sc.identical(a, orig)
    b = orig['x', 1:].copy()
    assert b.data.values[0] == orig.data.values[1:]


def test_no_default_init():
    with pytest.raises(TypeError):
        sc.DataArray()


def test_init():
    d = sc.DataArray(
        data=sc.Variable(dims=['x'], values=np.arange(3)),
        coords={
            'x': sc.Variable(dims=['x'], values=np.arange(3), unit=sc.units.m),
            'lib1': sc.Variable(dims=['x'], values=np.random.rand(3))
        },
        attrs={'met1': sc.Variable(dims=['x'], values=np.arange(3))},
        masks={'mask1': sc.Variable(dims=['x'], values=np.ones(3, dtype=bool))})
    assert len(d.meta) == 3
    assert len(d.coords) == 2
    assert len(d.attrs) == 1
    assert len(d.masks) == 1
    assert d.ndim == 1


def test_init_with_name():
    a = sc.DataArray(data=1.0 * sc.units.m, name='abc')
    assert a.name == 'abc'


def test_init_from_variable_views():
    var = sc.Variable(dims=['x'], values=np.arange(5))
    a = sc.DataArray(data=var,
                     coords={'x': var},
                     attrs={'meta': var},
                     masks={'mask1': sc.less(var, sc.scalar(3))})
    b = sc.DataArray(data=a.data,
                     coords={'x': a.coords['x']},
                     attrs={'meta': a.attrs['meta']},
                     masks={'mask1': a.masks['mask1']})
    assert sc.identical(a, b)

    # Ensure mix of Variables and Variable views work
    c = sc.DataArray(data=a.data,
                     coords={'x': var},
                     attrs={'meta': a.attrs['meta']},
                     masks={'mask1': a.masks['mask1']})

    assert sc.identical(a, c)


@pytest.mark.parametrize('coords_wrapper', (dict, lambda d: d), ids=['dict', 'Coords'])
@pytest.mark.parametrize('attrs_wrapper', (dict, lambda d: d), ids=['dict', 'Attrs'])
@pytest.mark.parametrize('masks_wrapper', (dict, lambda d: d), ids=['dict', 'Masks'])
def test_init_from_existing_metadata(coords_wrapper, attrs_wrapper, masks_wrapper):
    da1 = sc.DataArray(
        sc.arange('x', 4),
        coords={'x': sc.arange('x', 5, unit='m'), 'y': sc.scalar(12.34)},
        attrs={'a': sc.arange('x', 4, unit='s'), 'b': sc.scalar('attr-b')},
        masks={'m': sc.array(dims=['x'], values=[False, True, True, False, False])})
    da2 = sc.DataArray(-sc.arange('x', 4),
                       coords=coords_wrapper(da1.coords),
                       attrs=attrs_wrapper(da1.attrs),
                       masks=masks_wrapper(da1.masks))
    assert set(da2.coords.keys()) == {'x', 'y'}
    assert sc.identical(da2.coords['x'], da1.coords['x'])
    assert sc.identical(da2.coords['y'], da1.coords['y'])
    assert set(da2.attrs.keys()) == {'a', 'b'}
    assert sc.identical(da2.attrs['a'], da1.attrs['a'])
    assert sc.identical(da2.attrs['b'], da1.attrs['b'])
    assert set(da2.masks.keys()) == {'m'}
    assert sc.identical(da2.masks['m'], da1.masks['m'])


def test_init_from_iterable_of_tuples():
    da = sc.DataArray(
        sc.arange('x', 4),
        coords=[('x', sc.arange('x', 5, unit='m')), ('y', sc.scalar(12.34))],
        attrs=(('a', sc.arange('x', 4, unit='s')), ('b', sc.scalar('attr-b'))),
        masks={'m': sc.array(dims=['x'], values=[False, True, True, False,
                                                 False])}.items())
    assert set(da.coords.keys()) == {'x', 'y'}
    assert sc.identical(da.coords['x'], sc.arange('x', 5, unit='m'))
    assert sc.identical(da.coords['y'], sc.scalar(12.34))
    assert set(da.attrs.keys()) == {'a', 'b'}
    assert sc.identical(da.attrs['a'], sc.arange('x', 4, unit='s'))
    assert sc.identical(da.attrs['b'], sc.scalar('attr-b'))
    assert set(da.masks.keys()) == {'m'}
    assert sc.identical(da.masks['m'],
                        sc.array(dims=['x'], values=[False, True, True, False, False]))


@pytest.mark.parametrize("make", [lambda x: x, sc.DataArray])
def test_builtin_len(make):
    var = sc.empty(dims=['x', 'y'], shape=[3, 2])
    obj = make(var)
    assert obj.ndim == 2
    assert len(obj) == 3
    assert len(obj['x', 0]) == 2
    assert len(obj['y', 0]) == 3
    with pytest.raises(TypeError):
        len(obj['x', 0]['y', 0])


def test_coords():
    da = make_dataarray()
    assert len(dict(da.meta)) == len(da.meta) == 4
    assert len(dict(da.coords)) == len(da.coords) == 3
    assert len(dict(da.attrs)) == len(da.attrs) == 1
    assert 'x' in da.coords
    assert 'y' in da.coords
    assert 'aux' in da.coords
    assert 'meta' in da.meta
    assert 'meta' in da.attrs


def test_masks():
    da = make_dataarray()
    mask = sc.Variable(dims=['x'], values=np.array([False, True], dtype=bool))
    da.masks['mask1'] = mask
    assert len(dict(da.masks)) == len(da.masks) == 1
    assert 'mask1' in da.masks
    assert sc.identical(da.masks.pop('mask1'), mask)
    assert len(dict(da.masks)) == len(da.masks) == 0


def test_masks_delitem():
    da = make_dataarray()
    mask = sc.Variable(dims=['x'], values=np.array([False, True], dtype=bool))
    ref = da.copy()
    da.masks['masks'] = mask
    assert not sc.identical(da, ref)
    del da.masks['masks']
    assert sc.identical(da, ref)


def test_ipython_key_completion():
    da = make_dataarray()
    mask = sc.Variable(dims=['x'], values=np.array([False, True], dtype=bool))
    da.masks['mask1'] = mask
    assert set(da.coords._ipython_key_completions_()) == set(da.coords.keys())
    assert set(da.attrs._ipython_key_completions_()) == set(da.attrs.keys())
    assert set(da.meta._ipython_key_completions_()) == set(da.meta.keys())
    assert set(da.masks._ipython_key_completions_()) == set(da.masks.keys())


def test_name():
    a = sc.DataArray(data=1.0 * sc.units.m)
    assert a.name == ''
    a.name = 'abc'
    assert a.name == 'abc'


def test_eq():
    da = make_dataarray()
    assert sc.identical(da['x', :], da)
    assert sc.identical(da['y', :], da)
    assert sc.identical(da['y', :]['x', :], da)
    assert not sc.identical(da['y', 1:], da)
    assert not sc.identical(da['x', 1:], da)
    assert not sc.identical(da['y', 1:]['x', :], da)
    assert not sc.identical(da['y', :]['x', 1:], da)


def _is_copy_of(orig, copy):
    assert sc.identical(orig, copy)
    assert not id(orig) == id(copy)
    orig += 1.0
    assert sc.identical(orig, copy)


def _is_deep_copy_of(orig, copy):
    assert sc.identical(orig, copy)
    assert not id(orig) == id(copy)
    orig += 1.0
    assert not sc.identical(orig, copy)


def test_copy():
    import copy
    da = make_dataarray()
    _is_copy_of(da, da.copy(deep=False))
    _is_deep_copy_of(da, da.copy())
    _is_copy_of(da, copy.copy(da))
    _is_deep_copy_of(da, copy.deepcopy(da))


def test_in_place_binary_with_variable():
    a = sc.DataArray(data=sc.Variable(dims=['x'], values=np.arange(10.0)),
                     coords={'x': sc.Variable(dims=['x'], values=np.arange(10.0))})
    copy = a.copy()

    a += 2.0 * sc.units.dimensionless
    a *= 2.0 * sc.units.m
    a -= 4.0 * sc.units.m
    a /= 2.0 * sc.units.m
    assert sc.identical(a, copy)


def test_in_place_binary_with_dataarray():
    da = sc.DataArray(
        data=sc.Variable(dims=['x'], values=np.arange(1.0, 10.0)),
        coords={'x': sc.Variable(dims=['x'], values=np.arange(1.0, 10.0))})
    orig = da.copy()
    da += orig
    da -= orig
    da *= orig
    da /= orig
    assert sc.identical(da, orig)


def test_in_place_binary_with_scalar():
    a = sc.DataArray(data=sc.Variable(dims=['x'], values=[10.0]),
                     coords={'x': sc.Variable(dims=['x'], values=[10])})
    copy = a.copy()

    a += 2
    a *= 2
    a -= 4
    a /= 2
    assert sc.identical(a, copy)


def test_binary_with_broadcast():
    da = sc.DataArray(data=sc.Variable(dims=['x', 'y'],
                                       values=np.arange(20).reshape(5, 4)),
                      coords={
                          'x': sc.Variable(dims=['x'], values=np.arange(0.0, 0.6, 0.1)),
                          'y': sc.Variable(dims=['y'], values=np.arange(0.0, 0.5, 0.1))
                      })
    d2 = da - da['x', 0]
    da -= da['x', 0]
    assert sc.identical(da, d2)


def test_view_in_place_binary_with_scalar():
    d = sc.Dataset(data={'data': sc.Variable(dims=['x'], values=[10.0])},
                   coords={'x': sc.Variable(dims=['x'], values=[10])})
    copy = d.copy()

    d['x', :] += 2
    d['x', :] *= 2
    d['x', :] -= 4
    d['x', :] /= 2
    assert sc.identical(d, copy)


def test_coord_setitem_can_change_dtype():
    a = np.arange(3)
    v1 = sc.array(dims=['x'], values=a)
    v2 = v1.astype(sc.DType.int32)
    data = sc.DataArray(data=v1, coords={'x': v1})
    data.coords['x'] = v2


def test_setitem_works_for_view_and_array():
    a = make_dataarray('x', 'y', seed=0)
    a['x', :]['x', 0] = a['x', 1]
    a['x', 0] = a['x', 1]


def test_astype():
    a = sc.DataArray(data=sc.Variable(dims=['x'],
                                      values=np.arange(10.0, dtype=np.int64)),
                     coords={'x': sc.Variable(dims=['x'], values=np.arange(10.0))})
    assert a.dtype == sc.DType.int64

    a_as_float = a.astype(sc.DType.float32)
    assert a_as_float.dtype == sc.DType.float32


def test_astype_bad_conversion():
    a = sc.DataArray(data=sc.Variable(dims=['x'],
                                      values=np.arange(10.0, dtype=np.int64)),
                     coords={'x': sc.Variable(dims=['x'], values=np.arange(10.0))})
    assert a.dtype == sc.DType.int64

    with pytest.raises(sc.DTypeError):
        a.astype(sc.DType.string)


def test_reciprocal():
    a = sc.DataArray(data=sc.Variable(dims=['x'], values=np.array([5.0])))
    r = sc.reciprocal(a)
    assert r.values[0] == 1.0 / 5.0


def test_sizes():
    a = sc.DataArray(data=sc.scalar(value=1))
    assert a.sizes == {}
    a = sc.DataArray(data=sc.Variable(dims=['x'], values=np.ones(2)))
    assert a.sizes == {'x': 2}
    a = sc.DataArray(data=sc.Variable(dims=['x', 'z'], values=np.ones((2, 4))))
    assert a.sizes == {'x': 2, 'z': 4}


def test_size():
    a = sc.DataArray(data=sc.scalar(value=1))
    assert a.size == 1
    a = sc.DataArray(data=sc.Variable(dims=['x'], values=np.ones(2)))
    assert a.size == 2
    a = sc.DataArray(data=sc.Variable(dims=['x', 'z'], values=np.ones((2, 4))))
    assert a.size == 8
    a = sc.DataArray(data=sc.Variable(dims=['x', 'z'], values=np.ones((0, 4))))
    assert a.size == 0


def test_to():
    da = sc.DataArray(data=sc.scalar(value=1, dtype="int32", unit="m"))

    assert sc.identical(
        da.to(unit="mm", dtype="int64"),
        sc.DataArray(data=sc.scalar(value=1000, dtype="int64", unit="mm")))


def test_zeros_like():
    a = make_dataarray()
    a.masks['m'] = sc.array(dims=['x'], values=[True, False])
    b = sc.zeros_like(a)
    a.data *= 0.
    assert sc.identical(a, b)


def test_ones_like():
    a = make_dataarray()
    a.masks['m'] = sc.array(dims=['x'], values=[True, False])
    b = sc.ones_like(a)
    a.data *= 0.
    a.data += 1.
    assert sc.identical(a, b)


def test_empty_like():
    a = make_dataarray()
    a.masks['m'] = sc.array(dims=['x'], values=[True, False])
    b = sc.empty_like(a)
    assert a.dims == b.dims
    assert a.shape == b.shape
    assert a.unit == b.unit
    assert a.dtype == b.dtype
    assert (a.variances is None) == (b.variances is None)


def test_full_like():
    a = make_dataarray()
    a.masks['m'] = sc.array(dims=['x'], values=[True, False])
    b = sc.full_like(a, 2.)
    a.data *= 0.
    a.data += 2.
    assert sc.identical(a, b)


def test_zeros_like_deep_copy_masks():
    a = make_dataarray()
    a.masks['m'] = sc.array(dims=['x'], values=[True, False])
    c = sc.scalar(33., unit='m')
    b = sc.zeros_like(a)
    a.coords['x'][0] = c
    a.masks['m'][0] = False
    assert sc.identical(b.coords['x'][0], c)
    assert sc.identical(b.masks['m'][0], sc.scalar(True))


def test_drop_coords():
    data = sc.array(dims=['x', 'y', 'z'], values=np.random.rand(4, 3, 5))
    coord0 = sc.linspace('x', start=0.2, stop=1.61, num=4)
    coord1 = sc.linspace('y', start=1, stop=4, num=3)
    coord2 = sc.linspace('z', start=-0.1, stop=0.1, num=5)
    da = sc.DataArray(data,
                      coords={'coord0': coord0, 'coord1': coord1, 'coord2': coord2})

    assert 'coord0' not in da.drop_coords(['coord0']).coords
    assert 'coord0' not in da.drop_coords('coord0').coords
    assert 'coord1' in da.drop_coords('coord0').coords
    assert 'coord2' in da.drop_coords('coord0').coords
    assert 'coord0' not in da.drop_coords(['coord0', 'coord1']).coords
    assert 'coord1' not in da.drop_coords(['coord0', 'coord1']).coords
    assert 'coord2' in da.drop_coords(['coord0', 'coord1']).coords


def test_drop_masks():
    data = sc.array(dims=['x', 'y', 'z'], values=np.random.rand(4, 3, 5))
    mask0 = sc.array(dims=['x'], values=[False, True, True, False])
    mask1 = sc.array(dims=['y'], values=[1, 2, 4])
    mask2 = sc.array(dims=['z'], values=[False, False, True, True, True])
    da = sc.DataArray(data, masks={'mask0': mask0, 'mask1': mask1, 'mask2': mask2})

    assert 'mask0' not in da.drop_masks('mask0').masks
    assert 'mask1' in da.drop_masks('mask0').masks
    assert 'mask2' in da.drop_masks(['mask0']).masks
    assert 'mask0' not in da.drop_masks(['mask0', 'mask1']).masks
    assert 'mask1' not in da.drop_masks(['mask0', 'mask1']).masks
    assert 'mask2' in da.drop_masks(['mask0', 'mask1']).masks


def test_drop_attrs():
    data = sc.array(dims=['x', 'y', 'z'], values=np.random.rand(4, 3, 5))
    attr0 = sc.scalar('attribute_0')
    attr1 = sc.linspace('y', start=0.2, stop=1.61, num=4)
    attr2 = sc.linspace('z', start=-10, stop=10, num=5)
    da = sc.DataArray(data, attrs={'attr0': attr0, 'attr1': attr1, 'attr2': attr2})

    assert 'attr0' not in da.drop_attrs('attr0').attrs
    assert 'attr1' in da.drop_attrs('attr0').attrs
    assert 'attr2' in da.drop_attrs(['attr0']).attrs
    assert 'attr0' not in da.drop_attrs(['attr0', 'attr1']).attrs
    assert 'attr1' not in da.drop_attrs(['attr0', 'attr1']).attrs
    assert 'attr2' in da.drop_attrs(['attr0', 'attr1']).attrs


def test_attrs_update_from_dict_adds_items():
    da = sc.DataArray(sc.scalar(0.0), attrs={'a': sc.scalar(1.0)})
    da.attrs.update({'b': sc.scalar(2.0)})
    assert sc.identical(da.attrs['a'], sc.scalar(1.0))
    assert sc.identical(da.attrs['b'], sc.scalar(2.0))


def test_masks_update_from_dict_adds_items():
    da = sc.DataArray(sc.scalar(0.0), masks={'a': sc.scalar(True)})
    da.masks.update({'b': sc.scalar(False)})
    assert sc.identical(da.masks['a'], sc.scalar(True))
    assert sc.identical(da.masks['b'], sc.scalar(False))
