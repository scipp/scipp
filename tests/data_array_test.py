# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
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
            dim1: sc.Variable(dims=[dim1], values=np.arange(2.0), unit=sc.units.m),
            dim2: sc.Variable(dims=[dim2], values=np.arange(3.0), unit=sc.units.m),
            'aux': sc.Variable(dims=[dim2], values=np.random.rand(3)),
        },
    )


def test_slice_init() -> None:
    orig = sc.DataArray(
        data=sc.Variable(dims=['x'], values=np.arange(2.0)),
        coords={'x': sc.Variable(dims=['x'], values=np.arange(3.0))},
    )
    a = orig['x', :].copy()
    assert sc.identical(a, orig)
    b = orig['x', 1:].copy()
    assert b.data.values[0] == orig.data.values[1:]


def test_no_default_init() -> None:
    with pytest.raises(TypeError):
        sc.DataArray()


def test_init() -> None:
    d = sc.DataArray(
        data=sc.Variable(dims=['x'], values=np.arange(3)),
        coords={
            'x': sc.Variable(dims=['x'], values=np.arange(3), unit=sc.units.m),
            'lib1': sc.Variable(dims=['x'], values=np.random.rand(3)),
        },
        masks={'mask1': sc.Variable(dims=['x'], values=np.ones(3, dtype=bool))},
    )
    assert len(d.coords) == 2
    assert len(d.masks) == 1
    assert d.ndim == 1


def test_init_with_name() -> None:
    a = sc.DataArray(data=1.0 * sc.units.m, name='abc')
    assert a.name == 'abc'


def test_init_from_variable_views() -> None:
    var = sc.Variable(dims=['x'], values=np.arange(5))
    a = sc.DataArray(
        data=var,
        coords={'x': var},
        masks={'mask1': sc.less(var, sc.scalar(3))},
    )
    b = sc.DataArray(
        data=a.data,
        coords={'x': a.coords['x']},
        masks={'mask1': a.masks['mask1']},
    )
    assert sc.identical(a, b)

    # Ensure mix of Variables and Variable views work
    c = sc.DataArray(
        data=a.data,
        coords={'x': var},
        masks={'mask1': a.masks['mask1']},
    )

    assert sc.identical(a, c)


@pytest.mark.parametrize('coords_wrapper', [dict, lambda d: d], ids=['dict', 'Coords'])
@pytest.mark.parametrize('attrs_wrapper', [dict, lambda d: d], ids=['dict', 'Attrs'])
@pytest.mark.parametrize('masks_wrapper', [dict, lambda d: d], ids=['dict', 'Masks'])
def test_init_from_existing_metadata(
    coords_wrapper, attrs_wrapper, masks_wrapper
) -> None:
    da1 = sc.DataArray(
        sc.arange('x', 4),
        coords={'x': sc.arange('x', 5, unit='m'), 'y': sc.scalar(12.34)},
        masks={'m': sc.array(dims=['x'], values=[False, True, True, False, False])},
    )
    da2 = sc.DataArray(
        -sc.arange('x', 4),
        coords=coords_wrapper(da1.coords),
        masks=masks_wrapper(da1.masks),
    )
    assert set(da2.coords.keys()) == {'x', 'y'}
    assert sc.identical(da2.coords['x'], da1.coords['x'])
    assert sc.identical(da2.coords['y'], da1.coords['y'])
    assert set(da2.masks.keys()) == {'m'}
    assert sc.identical(da2.masks['m'], da1.masks['m'])


def test_init_from_iterable_of_tuples() -> None:
    da = sc.DataArray(
        sc.arange('x', 4),
        coords=[('x', sc.arange('x', 5, unit='m')), ('y', sc.scalar(12.34))],
        masks={
            'm': sc.array(dims=['x'], values=[False, True, True, False, False])
        }.items(),
    )
    assert set(da.coords.keys()) == {'x', 'y'}
    assert sc.identical(da.coords['x'], sc.arange('x', 5, unit='m'))
    assert sc.identical(da.coords['y'], sc.scalar(12.34))
    assert set(da.masks.keys()) == {'m'}
    assert sc.identical(
        da.masks['m'], sc.array(dims=['x'], values=[False, True, True, False, False])
    )


@pytest.mark.parametrize("make", [lambda x: x, sc.DataArray])
def test_builtin_len(make) -> None:
    var = sc.empty(dims=['x', 'y'], shape=[3, 2])
    obj = make(var)
    assert obj.ndim == 2
    assert len(obj) == 3
    assert len(obj['x', 0]) == 2
    assert len(obj['y', 0]) == 3
    with pytest.raises(TypeError):
        len(obj['x', 0]['y', 0])


def test_coords() -> None:
    da = make_dataarray()
    assert len(dict(da.coords)) == len(da.coords) == 3
    assert 'x' in da.coords
    assert 'y' in da.coords
    assert 'aux' in da.coords


def test_masks() -> None:
    da = make_dataarray()
    mask = sc.Variable(dims=['x'], values=np.array([False, True], dtype=bool))
    da.masks['mask1'] = mask
    assert len(dict(da.masks)) == len(da.masks) == 1
    assert 'mask1' in da.masks
    assert sc.identical(da.masks.pop('mask1'), mask)
    assert len(dict(da.masks)) == len(da.masks) == 0


def test_masks_delitem() -> None:
    da = make_dataarray()
    mask = sc.Variable(dims=['x'], values=np.array([False, True], dtype=bool))
    ref = da.copy()
    da.masks['masks'] = mask
    assert not sc.identical(da, ref)
    del da.masks['masks']
    assert sc.identical(da, ref)


def test_ipython_key_completion() -> None:
    da = make_dataarray()
    mask = sc.Variable(dims=['x'], values=np.array([False, True], dtype=bool))
    da.masks['mask1'] = mask
    assert set(da.coords._ipython_key_completions_()) == set(da.coords.keys())
    assert set(da.masks._ipython_key_completions_()) == set(da.masks.keys())


def test_name() -> None:
    a = sc.DataArray(data=1.0 * sc.units.m)
    assert a.name == ''
    a.name = 'abc'
    assert a.name == 'abc'


def test_eq() -> None:
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


def test_copy() -> None:
    import copy

    da = make_dataarray()
    _is_copy_of(da, da.copy(deep=False))
    _is_deep_copy_of(da, da.copy())
    _is_copy_of(da, copy.copy(da))
    _is_deep_copy_of(da, copy.deepcopy(da))


def test_in_place_binary_with_variable() -> None:
    a = sc.DataArray(
        data=sc.Variable(dims=['x'], values=np.arange(10.0)),
        coords={'x': sc.Variable(dims=['x'], values=np.arange(10.0))},
    )
    copy = a.copy()

    a += 2.0 * sc.units.dimensionless
    a *= 2.0 * sc.units.m
    a -= 4.0 * sc.units.m
    a /= 2.0 * sc.units.m
    assert sc.identical(a, copy)


def test_in_place_binary_with_dataarray() -> None:
    da = sc.DataArray(
        data=sc.Variable(dims=['x'], values=np.arange(1.0, 10.0)),
        coords={'x': sc.Variable(dims=['x'], values=np.arange(1.0, 10.0))},
    )
    orig = da.copy()
    da += orig
    da -= orig
    da *= orig
    da /= orig
    assert sc.identical(da, orig)


def test_in_place_binary_with_scalar() -> None:
    a = sc.DataArray(
        data=sc.Variable(dims=['x'], values=[10.0]),
        coords={'x': sc.Variable(dims=['x'], values=[10])},
    )
    copy = a.copy()

    a += 2
    a *= 2
    a -= 4
    a /= 2
    assert sc.identical(a, copy)


def test_binary_with_broadcast() -> None:
    da = sc.DataArray(
        data=sc.Variable(dims=['x', 'y'], values=np.arange(20).reshape(5, 4)),
        coords={
            'x': sc.Variable(dims=['x'], values=np.arange(0.0, 0.6, 0.1)),
            'y': sc.Variable(dims=['y'], values=np.arange(0.0, 0.5, 0.1)),
        },
    )
    d2 = da - da['x', 0]
    da -= da['x', 0]
    assert sc.identical(da, d2)


def test_view_in_place_binary_with_scalar() -> None:
    d = sc.Dataset(
        data={'data': sc.Variable(dims=['x'], values=[10.0])},
        coords={'x': sc.Variable(dims=['x'], values=[10])},
    )
    copy = d.copy()

    d['x', :] += 2
    d['x', :] *= 2
    d['x', :] -= 4
    d['x', :] /= 2
    assert sc.identical(d, copy)


def test_coord_setitem_can_change_dtype() -> None:
    a = np.arange(3)
    v1 = sc.array(dims=['x'], values=a)
    v2 = v1.astype(sc.DType.int32)
    data = sc.DataArray(data=v1, coords={'x': v1})
    data.coords['x'] = v2


def test_setitem_works_for_view_and_array() -> None:
    a = make_dataarray('x', 'y', seed=0)
    a['x', :]['x', 0] = a['x', 1]
    a['x', 0] = a['x', 1]


def test_astype() -> None:
    a = sc.DataArray(
        data=sc.Variable(dims=['x'], values=np.arange(10.0, dtype=np.int64)),
        coords={'x': sc.Variable(dims=['x'], values=np.arange(10.0))},
    )
    assert a.dtype == sc.DType.int64

    a_as_float = a.astype(sc.DType.float32)
    assert a_as_float.dtype == sc.DType.float32


def test_astype_bad_conversion() -> None:
    a = sc.DataArray(
        data=sc.Variable(dims=['x'], values=np.arange(10.0, dtype=np.int64)),
        coords={'x': sc.Variable(dims=['x'], values=np.arange(10.0))},
    )
    assert a.dtype == sc.DType.int64

    with pytest.raises(sc.DTypeError):
        a.astype(sc.DType.string)


def test_reciprocal() -> None:
    a = sc.DataArray(data=sc.Variable(dims=['x'], values=np.array([5.0])))
    r = sc.reciprocal(a)
    assert r.values[0] == 1.0 / 5.0


def test_sizes() -> None:
    a = sc.DataArray(data=sc.scalar(value=1))
    assert a.sizes == {}
    a = sc.DataArray(data=sc.Variable(dims=['x'], values=np.ones(2)))
    assert a.sizes == {'x': 2}
    a = sc.DataArray(data=sc.Variable(dims=['x', 'z'], values=np.ones((2, 4))))
    assert a.sizes == {'x': 2, 'z': 4}


def test_size() -> None:
    a = sc.DataArray(data=sc.scalar(value=1))
    assert a.size == 1
    a = sc.DataArray(data=sc.Variable(dims=['x'], values=np.ones(2)))
    assert a.size == 2
    a = sc.DataArray(data=sc.Variable(dims=['x', 'z'], values=np.ones((2, 4))))
    assert a.size == 8
    a = sc.DataArray(data=sc.Variable(dims=['x', 'z'], values=np.ones((0, 4))))
    assert a.size == 0


def test_to() -> None:
    da = sc.DataArray(data=sc.scalar(value=1, dtype="int32", unit="m"))

    assert sc.identical(
        da.to(unit="mm", dtype="int64"),
        sc.DataArray(data=sc.scalar(value=1000, dtype="int64", unit="mm")),
    )


def test_zeros_like() -> None:
    a = make_dataarray()
    a.masks['m'] = sc.array(dims=['x'], values=[True, False])
    b = sc.zeros_like(a)
    a.data *= 0.0
    assert sc.identical(a, b)


def test_ones_like() -> None:
    a = make_dataarray()
    a.masks['m'] = sc.array(dims=['x'], values=[True, False])
    b = sc.ones_like(a)
    a.data *= 0.0
    a.data += 1.0
    assert sc.identical(a, b)


def test_empty_like() -> None:
    a = make_dataarray()
    a.masks['m'] = sc.array(dims=['x'], values=[True, False])
    b = sc.empty_like(a)
    assert a.dims == b.dims
    assert a.shape == b.shape
    assert a.unit == b.unit
    assert a.dtype == b.dtype
    assert (a.variances is None) == (b.variances is None)


def test_full_like() -> None:
    a = make_dataarray()
    a.masks['m'] = sc.array(dims=['x'], values=[True, False])
    b = sc.full_like(a, 2.0)
    a.data *= 0.0
    a.data += 2.0
    assert sc.identical(a, b)


def test_zeros_like_deep_copy_masks() -> None:
    a = make_dataarray()
    a.masks['m'] = sc.array(dims=['x'], values=[True, False])
    c = sc.scalar(33.0, unit='m')
    b = sc.zeros_like(a)
    a.coords['x'][0] = c
    a.masks['m'][0] = False
    assert sc.identical(b.coords['x'][0], c)
    assert sc.identical(b.masks['m'][0], sc.scalar(True))


def test_drop_coords() -> None:
    data = sc.array(dims=['x', 'y', 'z'], values=np.random.rand(4, 3, 5))
    coord0 = sc.linspace('x', start=0.2, stop=1.61, num=4)
    coord1 = sc.linspace('y', start=1, stop=4, num=3)
    coord2 = sc.linspace('z', start=-0.1, stop=0.1, num=5)
    da = sc.DataArray(
        data, coords={'coord0': coord0, 'coord1': coord1, 'coord2': coord2}
    )

    assert 'coord0' not in da.drop_coords(['coord0']).coords
    assert 'coord0' not in da.drop_coords('coord0').coords
    assert 'coord1' in da.drop_coords('coord0').coords
    assert 'coord2' in da.drop_coords('coord0').coords
    assert 'coord0' not in da.drop_coords(['coord0', 'coord1']).coords
    assert 'coord1' not in da.drop_coords(['coord0', 'coord1']).coords
    assert 'coord2' in da.drop_coords(['coord0', 'coord1']).coords


def test_drop_masks() -> None:
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


def test_masks_update_from_dict_adds_items() -> None:
    da = sc.DataArray(sc.scalar(0.0), masks={'a': sc.scalar(True)})
    da.masks.update({'b': sc.scalar(False)})
    assert sc.identical(da.masks['a'], sc.scalar(True))
    assert sc.identical(da.masks['b'], sc.scalar(False))


def test_assign() -> None:
    data = sc.array(dims=['x', 'y'], values=np.random.rand(4, 3))
    coord = sc.linspace('x', start=0.2, stop=1.61, num=4)
    da_o = sc.DataArray(data.copy(), coords={'x': coord})
    da_n = da_o.assign(2 * data)
    assert sc.identical(da_o, sc.DataArray(data, coords={'x': coord}))
    assert sc.identical(da_n, sc.DataArray(2 * data, coords={'x': coord}))


def test_assign_coords() -> None:
    data = sc.array(dims=['x', 'y', 'z'], values=np.random.rand(4, 3, 5))
    da_o = sc.DataArray(data)
    coord0 = sc.linspace('x', start=0.2, stop=1.61, num=4)
    coord1 = sc.linspace('y', start=1, stop=4, num=3)
    da_n = da_o.assign_coords({'coord0': coord0, 'coord1': coord1})
    assert sc.identical(da_o, sc.DataArray(data))
    assert sc.identical(
        da_n, sc.DataArray(data, coords={'coord0': coord0, 'coord1': coord1})
    )


def test_assign_coords_kwargs() -> None:
    data = sc.array(dims=['x', 'y', 'z'], values=np.random.rand(4, 3, 5))
    da_o = sc.DataArray(data)
    coord0 = sc.linspace('x', start=0.2, stop=1.61, num=4)
    coord1 = sc.linspace('y', start=1, stop=4, num=3)
    da_n = da_o.assign_coords(coord0=coord0, coord1=coord1)
    assert sc.identical(da_o, sc.DataArray(data))
    assert sc.identical(
        da_n, sc.DataArray(data, coords={'coord0': coord0, 'coord1': coord1})
    )


def test_assign_coords_overlapping_names() -> None:
    data = sc.array(dims=['x', 'y', 'z'], values=np.random.rand(4, 3, 5))
    da = sc.DataArray(data)
    coord0 = sc.linspace('x', start=0.2, stop=1.61, num=4)
    with pytest.raises(TypeError, match='names .* distinct'):
        da.assign_coords({'coord0': coord0}, coord0=coord0)


def test_assign_update_coords() -> None:
    data = sc.array(dims=['x', 'y', 'z'], values=np.random.rand(4, 3, 5))
    coord0_o = sc.linspace('x', start=0.2, stop=1.61, num=4)
    coord1_o = sc.linspace('y', start=1, stop=4, num=4)
    da_o = sc.DataArray(data, coords={'coord0': coord0_o, 'coord1': coord1_o})
    coord0_n = sc.linspace('x', start=0.2, stop=1.61, num=4)
    coord1_n = sc.linspace('y', start=1, stop=4, num=4)
    da_n = da_o.assign_coords({"coord0": coord0_n, "coord1": coord1_n})
    assert sc.identical(
        da_o, sc.DataArray(data, coords={'coord0': coord0_o, 'coord1': coord1_o})
    )
    assert sc.identical(
        da_n, sc.DataArray(data, coords={'coord0': coord0_n, 'coord1': coord1_n})
    )


def test_assign_masks() -> None:
    data = sc.array(dims=['x', 'y', 'z'], values=np.random.rand(4, 3, 5))
    da_o = sc.DataArray(data)
    mask0 = sc.array(dims=['x'], values=[False, True, True, False])
    mask1 = sc.array(dims=['y'], values=[1, 2, 4])
    da_n = da_o.assign_masks({'mask0': mask0, 'mask1': mask1})
    assert sc.identical(da_o, sc.DataArray(data))
    assert sc.identical(
        da_n, sc.DataArray(data, masks={'mask0': mask0, 'mask1': mask1})
    )


def test_assign_masks_kwargs() -> None:
    data = sc.array(dims=['x', 'y', 'z'], values=np.random.rand(4, 3, 5))
    da_o = sc.DataArray(data)
    mask0 = sc.array(dims=['x'], values=[False, True, True, False])
    mask1 = sc.array(dims=['y'], values=[1, 2, 4])
    da_n = da_o.assign_masks(mask0=mask0, mask1=mask1)
    assert sc.identical(da_o, sc.DataArray(data))
    assert sc.identical(
        da_n, sc.DataArray(data, masks={'mask0': mask0, 'mask1': mask1})
    )


def test_assign_masks_overlapping_names() -> None:
    data = sc.array(dims=['x', 'y', 'z'], values=np.random.rand(4, 3, 5))
    da = sc.DataArray(data)
    mask0 = sc.array(dims=['x'], values=[False, True, True, False])
    with pytest.raises(TypeError, match='names .* distinct'):
        da.assign_masks({'mask0': mask0}, mask0=mask0)


def test_assign_update_masks() -> None:
    data = sc.array(dims=['x', 'y', 'z'], values=np.random.rand(4, 3, 5))
    mask0_o = sc.array(dims=['x'], values=[False, True, True, False])
    mask1_o = sc.array(dims=['y'], values=[1, 2, 4])
    da_o = sc.DataArray(data, masks={'mask0': mask0_o, 'mask1': mask1_o})

    mask0_n = sc.array(dims=['x'], values=[True, False, False, True])
    mask1_n = sc.array(dims=['y'], values=[0.1, 0.2, 0.4])
    da_n = da_o.assign_masks({"mask0": mask0_n, "mask1": mask1_n})
    assert sc.identical(
        da_o, sc.DataArray(data, masks={'mask0': mask0_o, 'mask1': mask1_o})
    )
    assert sc.identical(
        da_n, sc.DataArray(data, masks={'mask0': mask0_n, 'mask1': mask1_n})
    )


def test_setting_binned_var_as_coord_or_mask_raises() -> None:
    da = sc.data.binned_x(1, 1)
    bad = da.bins.coords['x']
    with pytest.raises(sc.VariableError):
        da.coords['x'] = bad
    with pytest.raises(sc.VariableError):
        da.masks['x'] = bad


def test_creating_data_array_with_binned_var_as_coord_or_mask_raises() -> None:
    da = sc.data.binned_x(1, 1)
    bad = da.bins.coords['x'][0]
    with pytest.raises(sc.VariableError):
        sc.DataArray(data=sc.scalar(1), coords={'x': bad})
    with pytest.raises(sc.VariableError):
        sc.DataArray(data=sc.scalar(1), masks={'x': bad})
    with pytest.raises(sc.VariableError):
        sc.Dataset(coords={'x': bad})
