# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from math import isnan

import numpy as np
import pytest
from numpy.random import default_rng

import scipp as sc


def get_coords(x):
    return x.coords


def get_masks(x):
    return x.masks


def test_dense_data_properties_are_none() -> None:
    var = sc.scalar(1)
    assert not var.is_binned


def test_bins_default_begin_end() -> None:
    data = sc.Variable(dims=['x'], values=[1, 2, 3, 4])
    var = sc.bins(dim='x', data=data)
    assert var.dims == data.dims
    assert var.shape == data.shape
    for i in range(4):
        assert sc.identical(var['x', i].value, data['x', i : i + 1])


def test_bins_default_end_uses_begin_as_offsets() -> None:
    data = sc.Variable(dims=['x'], values=[1, 2, 3, 4])
    begin = sc.Variable(dims=['y'], values=[1, 3], dtype=sc.DType.int64, unit=None)
    var = sc.bins(begin=begin, dim='x', data=data)
    assert var.dims == begin.dims
    assert var.shape == begin.shape
    assert sc.identical(var['y', 0].value, data['x', 1:3])
    assert sc.identical(var['y', 1].value, data['x', 3:4])


def test_bins_fail_only_end() -> None:
    data = sc.Variable(dims=['x'], values=[1, 2, 3, 4])
    end = sc.Variable(dims=['y'], values=[1, 3], dtype=sc.DType.int64, unit=None)
    with pytest.raises(RuntimeError):
        sc.bins(end=end, dim='x', data=data)


@pytest.mark.parametrize('begin_dtype', [sc.DType.int32, sc.DType.int64])
@pytest.mark.parametrize('end_dtype', [sc.DType.int32, sc.DType.int64])
def test_bins_works_with_int32_begin_end(begin_dtype, end_dtype) -> None:
    data = sc.array(dims=['x'], values=[1, 2, 3, 4])
    begin = sc.Variable(dims=['y'], values=[1, 3], dtype=begin_dtype, unit=None)
    end = sc.Variable(dims=['y'], values=[3, 4], dtype=end_dtype, unit=None)
    var = sc.bins(begin=begin, end=end, dim='x', data=data)
    assert var.sizes == begin.sizes
    assert sc.identical(var['y', 0].value, data['x', 1:3])
    assert sc.identical(var['y', 1].value, data['x', 3:4])


def test_bins_works_with_int32_begin() -> None:
    data = sc.array(dims=['x'], values=[1, 2, 3, 4])
    begin = sc.Variable(dims=['y'], values=[1, 3], dtype=sc.DType.int32, unit=None)
    var = sc.bins(begin=begin, dim='x', data=data)
    assert var.sizes == begin.sizes
    assert sc.identical(var['y', 0].value, data['x', 1:3])
    assert sc.identical(var['y', 1].value, data['x', 3:4])


def test_bins_constituents() -> None:
    var = sc.Variable(dims=['x'], values=[1, 2, 3, 4])
    data = sc.DataArray(data=var, coords={'coord': var}, masks={'mask': var})
    begin = sc.Variable(dims=['y'], values=[0, 2], dtype=sc.DType.int64, unit=None)
    end = sc.Variable(dims=['y'], values=[2, 4], dtype=sc.DType.int64, unit=None)
    binned = sc.bins(begin=begin, end=end, dim='x', data=data)
    events = binned.bins.constituents['data']
    assert 'coord' in events.coords
    assert 'mask' in events.masks
    del events.coords['coord']
    del events.masks['mask']
    # sc.bins makes a (shallow) copy of `data`
    assert 'coord' in data.coords
    assert 'mask' in data.masks
    # ... but when buffer is accessed we can insert/delete meta data
    assert 'coord' not in events.coords
    assert 'mask' not in events.masks
    events.coords['coord'] = var
    events.masks['mask'] = var
    assert 'coord' in events.coords
    assert 'mask' in events.masks


def test_bins() -> None:
    data = sc.Variable(dims=['x'], values=[1, 2, 3, 4])
    begin = sc.Variable(dims=['y'], values=[0, 2], dtype=sc.DType.int64, unit=None)
    end = sc.Variable(dims=['y'], values=[2, 4], dtype=sc.DType.int64, unit=None)
    var = sc.bins(begin=begin, end=end, dim='x', data=data)
    assert var.dims == begin.dims
    assert var.shape == begin.shape
    assert sc.identical(var['y', 0].value, data['x', 0:2])
    assert sc.identical(var['y', 1].value, data['x', 2:4])


def test_bins_raises_when_DataGroup_given() -> None:
    data = sc.Variable(dims=['x'], values=[1, 2, 3, 4])
    begin = sc.Variable(dims=['y'], values=[0, 2], dtype=sc.DType.int64, unit=None)
    end = sc.Variable(dims=['y'], values=[2, 4], dtype=sc.DType.int64, unit=None)
    dg = sc.DataGroup(a=data)
    with pytest.raises(ValueError, match='DataGroup argument'):
        sc.bins(begin=begin, end=end, dim='x', data=dg)


def test_bins_of_transpose() -> None:
    data = sc.Variable(dims=['row'], values=[1, 2, 3, 4])
    begin = sc.Variable(
        dims=['x', 'y'], values=[[0, 1], [2, 3]], dtype=sc.DType.int64, unit=None
    )
    end = begin + sc.index(1)
    var = sc.bins(begin=begin, end=end, dim='row', data=data)
    assert sc.identical(sc.bins(**var.transpose().bins.constituents), var.transpose())


def make_binned():
    col = sc.Variable(dims=['event'], values=[1, 2, 3, 4])
    table = sc.DataArray(
        data=col,
        coords={'time': col * 2.2},
        masks={'mask': col == col},
    )
    begin = sc.Variable(dims=['y'], values=[0, 2], dtype=sc.DType.int64, unit=None)
    end = sc.Variable(dims=['y'], values=[2, 4], dtype=sc.DType.int64, unit=None)
    return sc.bins(begin=begin, end=end, dim='event', data=table)


def test_bins_view() -> None:
    var = make_binned()
    assert 'time' in var.bins.coords
    assert 'mask' in var.bins.masks
    col = sc.Variable(dims=['event'], values=[1, 2, 3, 4])
    with pytest.raises(sc.DTypeError):
        var.bins.coords['time2'] = col  # col is not binned

    def check(a, b):
        # sc.identical does not work for us directly since we have owning and
        # non-owning views which currently never compare equal.
        assert sc.identical(1 * a, 1 * b)

    var.bins.coords['time'] = var.bins.data
    assert sc.identical(var.bins.coords['time'], var.bins.data)
    var.bins.coords['time'] = var.bins.data * 2.0
    check(var.bins.coords['time'], var.bins.data * 2.0)
    var.bins.coords['time2'] = var.bins.data
    assert sc.identical(var.bins.coords['time2'], var.bins.data)
    var.bins.coords['time3'] = var.bins.data * 2.0
    check(var.bins.coords['time3'], var.bins.data * 2.0)
    var.bins.data = var.bins.coords['time']
    assert sc.identical(var.bins.data, var.bins.coords['time'])
    var.bins.data = var.bins.data * 2.0
    del var.bins.coords['time3']
    assert 'time3' not in var.bins.coords


def test_bins_view_data_array_unit() -> None:
    var = make_binned()
    var.unit = 'mK'
    assert var.unit == 'mK'
    assert var.bins.unit == 'mK'
    var.bins.unit = 'K'
    assert var.unit == 'K'
    assert var.bins.unit == 'K'


def test_variable_bins_data_assign() -> None:
    var = make_binned()
    assert set(var.bins.coords) == {'time'}
    new = var.bins.assign(var.bins.data * 2.0)
    assert sc.identical(new.bins.data, var.bins.data * 2.0)
    assert sc.identical(new.bins.coords['time'], var.bins.coords['time'])


def test_data_array_bins_data_assign() -> None:
    da = sc.DataArray(make_binned(), coords={'x': sc.scalar(0.1)})
    assert set(da.bins.coords) == {'time'}
    new = da.bins.assign(da.bins.data * 2.0)
    assert sc.identical(new.bins.data, da.bins.data * 2.0)
    assert sc.identical(new.bins.coords['time'], da.bins.coords['time'])
    assert sc.identical(new.coords['x'], da.coords['x'])


def test_bins_view_coord_unit() -> None:
    var = make_binned()
    var.bins.coords['time'].unit = 'mK'
    assert var.bins.coords['time'].bins.unit == 'mK'
    var.bins.coords['time'].bins.unit = 'K'
    assert var.bins.coords['time'].bins.unit == 'K'


def test_bins_view_coords_iterators() -> None:
    var = make_binned()
    assert set(var.bins.coords) == {'time'}
    assert set(var.bins.coords.keys()) == {'time'}
    [value] = var.bins.coords.values()
    assert sc.identical(value, var.bins.coords['time'])
    [(key, value)] = var.bins.coords.items()
    assert key == 'time'
    assert sc.identical(value, var.bins.coords['time'])


def test_bins_view_coords_assign() -> None:
    var = make_binned()
    assert set(var.bins.coords) == {'time'}
    new = var.bins.assign_coords(
        {'a': var.bins.coords['time'] * 2.0}, b=var.bins.coords['time'] * 3.0
    )
    assert set(new.bins.coords) == {'time', 'a', 'b'}
    assert set(var.bins.coords) == {'time'}

    assert sc.identical(new.bins.coords['time'], var.bins.coords['time'])
    assert sc.identical(new.bins.coords['a'], var.bins.coords['time'] * 2.0)
    assert sc.identical(new.bins.coords['b'], var.bins.coords['time'] * 3.0)


def test_data_array_bins_view_coords_assign() -> None:
    da = sc.DataArray(make_binned())
    assert set(da.bins.coords) == {'time'}
    new = da.bins.assign_coords(
        {'a': da.bins.coords['time'] * 2.0}, b=da.bins.coords['time'] * 3.0
    )
    assert set(new.bins.coords) == {'time', 'a', 'b'}
    assert set(da.bins.coords) == {'time'}

    assert sc.identical(new.bins.coords['time'], da.bins.coords['time'])
    assert sc.identical(new.bins.coords['a'], da.bins.coords['time'] * 2.0)
    assert sc.identical(new.bins.coords['b'], da.bins.coords['time'] * 3.0)


def test_bins_view_coords_drop() -> None:
    var = make_binned()
    assert set(var.bins.coords) == {'time'}
    new = var.bins.drop_coords(('time',))
    assert set(new.bins.coords) == set()
    assert set(var.bins.coords) == {'time'}


def test_bins_view_masks_iterators() -> None:
    var = make_binned()
    assert set(var.bins.masks) == {'mask'}
    assert set(var.bins.masks.keys()) == {'mask'}
    [value] = var.bins.masks.values()
    assert sc.identical(value, var.bins.data == var.bins.data)
    [(key, value)] = var.bins.masks.items()
    assert key == 'mask'
    assert sc.identical(value, var.bins.data == var.bins.data)


@pytest.mark.parametrize('get', [get_coords, get_masks])
def test_bins_view_mapping_clear(get) -> None:
    var = make_binned()
    assert len(get(var.bins)) == 1
    get(var.bins).clear()
    assert len(get(var.bins)) == 0


@pytest.mark.parametrize('param', [(get_coords, 'time'), (get_masks, 'mask')])
def test_bins_view_mapping_delitem(param) -> None:
    get, name = param
    var = make_binned()
    del get(var.bins)[name]
    assert len(get(var.bins)) == 0


def test_bins_view_coords_update() -> None:
    var = make_binned()
    var.bins.coords.update({'extra': 2 * var.bins.coords['time']})
    assert set(var.bins.coords) == {'time', 'extra'}
    assert sc.identical(var.bins.coords['time'], make_binned().bins.coords['time'])
    assert sc.identical(var.bins.coords['extra'], 2 * make_binned().bins.coords['time'])


def test_bins_view_masks_update() -> None:
    var = make_binned()
    var.bins.masks.update({'extra': ~var.bins.masks['mask']})
    assert set(var.bins.masks) == {'mask', 'extra'}
    assert sc.identical(var.bins.masks['mask'], make_binned().bins.masks['mask'])
    assert sc.identical(var.bins.masks['extra'], ~make_binned().bins.masks['mask'])


def test_bins_view_masks_assign() -> None:
    var = make_binned()
    assert set(var.bins.masks) == {'mask'}
    new = var.bins.assign_masks(
        {'a': ~var.bins.masks['mask']}, b=var.bins.masks['mask']
    )
    assert set(new.bins.masks) == {'mask', 'a', 'b'}
    assert set(var.bins.masks) == {'mask'}

    assert sc.identical(new.bins.masks['mask'], var.bins.masks['mask'])
    assert sc.identical(new.bins.masks['a'], ~var.bins.masks['mask'])
    assert sc.identical(new.bins.masks['b'], var.bins.masks['mask'])


def test_bins_view_masks_drop() -> None:
    var = make_binned()
    assert set(var.bins.masks) == {'mask'}
    new = var.bins.drop_masks(('mask',))
    assert set(new.bins.masks) == set()
    assert set(var.bins.masks) == {'mask'}


@pytest.mark.parametrize('param', [(get_coords, 'time'), (get_masks, 'mask')])
def test_bins_view_mapping_pop(param) -> None:
    get, name = param
    var = make_binned()
    x = get(var.bins).pop(name)
    assert sc.identical(x, get(make_binned().bins)[name])


def test_bins_view_data_unit() -> None:
    var = make_binned()
    var.bins.data.unit = 'mK'
    assert var.bins.data.bins.unit == 'mK'
    var.bins.data.bins.unit = 'K'
    assert var.bins.data.bins.unit == 'K'


def test_bins_arithmetic() -> None:
    var = sc.Variable(dims=['event'], values=[1.0, 2.0, 3.0, 4.0])
    table = sc.DataArray(var, coords={'x': var})
    binned = table.bin(x=sc.array(dims=['x'], values=[1.0, 5.0]))
    hist = sc.DataArray(
        data=sc.Variable(dims=['x'], values=[1.0, 2.0]),
        coords={'x': sc.Variable(dims=['x'], values=[1.0, 3.0, 5.0])},
    )
    binned.bins *= sc.lookup(func=hist, dim='x')
    assert sc.identical(
        binned.bins.constituents['data'].data,
        sc.Variable(dims=['event'], values=[1.0, 2.0, 6.0, 8.0]),
    )


def test_bins_sum_with_masked_buffer() -> None:
    N = 5
    values = np.ones(N)
    data = sc.DataArray(
        data=sc.Variable(
            dims=['position'], unit=sc.units.counts, values=values, variances=values
        ),
        coords={
            'position': sc.Variable(
                dims=['position'], values=[f'site-{i}' for i in range(N)]
            ),
            'x': sc.Variable(dims=['position'], unit=sc.units.m, values=[0.2] * 5),
            'y': sc.Variable(dims=['position'], unit=sc.units.m, values=[0.2] * 5),
        },
        masks={
            'test-mask': sc.Variable(
                dims=['position'], values=[True, False, True, False, True]
            )
        },
    )
    xbins = sc.Variable(dims=['x'], unit=sc.units.m, values=[0.1, 0.5, 0.9])
    binned = sc.bin(data, x=xbins)
    assert binned.bins.sum().values[0] == 2


def test_bins_mean() -> None:
    data = sc.DataArray(
        data=sc.Variable(
            dims=['position'], unit=sc.units.counts, values=[0.1, 0.2, 0.3, 0.4, 0.5]
        ),
        coords={
            'x': sc.Variable(dims=['position'], unit=sc.units.m, values=[1, 2, 3, 4, 5])
        },
    )
    xbins = sc.Variable(dims=['x'], unit=sc.units.m, values=[0, 5, 6, 7])
    binned = data.bin(x=xbins)

    # Mean of [0.1, 0.2, 0.3, 0.4]
    assert binned.bins.mean().values[0] == 0.25

    # Mean of [0.5]
    assert binned.bins.mean().values[1] == 0.5

    # Mean of last (empty) bin should be NaN
    assert isnan(binned.bins.mean().values[2])

    assert binned.bins.mean().dims == ('x',)
    assert binned.bins.mean().shape == (3,)
    assert binned.bins.mean().unit == sc.units.counts


def test_bins_mean_with_masks() -> None:
    data = sc.DataArray(
        data=sc.Variable(
            dims=['position'], unit=sc.units.counts, values=[0.1, 0.2, 0.3, 0.4, 0.5]
        ),
        coords={
            'x': sc.Variable(dims=['position'], unit=sc.units.m, values=[1, 2, 3, 4, 5])
        },
        masks={
            'test-mask': sc.Variable(
                dims=['position'], values=[False, True, False, True, False]
            )
        },
    )
    xbins = sc.Variable(dims=['x'], unit=sc.units.m, values=[0, 5, 6, 7])
    binned = data.bin(x=xbins)

    # Mean of [0.1, 0.3] (0.2 and 0.4 are masked)
    assert binned.bins.mean().values[0] == 0.2

    # Mean of [0.5]
    assert binned.bins.mean().values[1] == 0.5

    # Mean of last (empty) bin should be NaN
    assert isnan(binned.bins.mean().values[2])

    assert binned.bins.mean().dims == ('x',)
    assert binned.bins.mean().shape == (3,)
    assert binned.bins.mean().unit == sc.units.counts


def test_bins_mean_using_bins() -> None:
    # Call to sc.bins gives different data structure compared to sc.bin

    buffer = sc.arange('event', 5, unit=sc.units.ns, dtype=sc.DType.float64)
    begin = sc.array(dims=['x'], values=[0, 2], dtype=sc.DType.int64, unit=None)
    end = sc.array(dims=['x'], values=[2, 5], dtype=sc.DType.int64, unit=None)
    binned = sc.bins(data=buffer, dim='event', begin=begin, end=end)
    means = binned.bins.mean()

    assert sc.identical(
        means,
        sc.array(dims=["x"], values=[0.5, 3], unit=sc.units.ns, dtype=sc.DType.float64),
    )


def test_bins_nanmean() -> None:
    data = sc.DataArray(
        data=sc.array(dims=['position'], values=[1.0, 2.0, -np.nan, 5.0, np.nan]),
        coords={'group': sc.array(dims=['position'], values=[0, 1, 1, 0, 2])},
    )
    groups = sc.array(dims=['group'], values=[0, 1, 2])
    binned = data.group(groups)

    assert binned.bins.nanmean().values[0] == 3.0
    assert binned.bins.nanmean().values[1] == 2.0
    # The last bin only contains NaN and is thus effectively empty,
    # so even nanmean returns NaN.
    assert isnan(binned.bins.nanmean().values[2])


def test_bins_nansum() -> None:
    data = sc.DataArray(
        data=sc.array(dims=['position'], values=[1.0, 2.0, -np.nan, 5.0, np.nan]),
        coords={'group': sc.array(dims=['position'], values=[0, 1, 1, 0, 2])},
    )
    groups = sc.array(dims=['group'], values=[0, 1, 2])
    binned = data.group(groups)
    expected = sc.DataArray(
        sc.array(dims=['group'], values=[6.0, 2.0, 0.0]), coords={'group': groups}
    )
    assert sc.identical(binned.bins.nansum(), expected)


def test_bins_max() -> None:
    data = sc.DataArray(
        data=sc.array(dims=['position'], values=[1.0, 2.0, 3.0, 5.0, 6.0]),
        coords={'group': sc.array(dims=['position'], values=[0, 1, 1, 0, 2])},
    )
    groups = sc.array(dims=['group'], values=[0, 1, 2])
    binned = data.group(groups)
    expected = sc.DataArray(
        sc.array(dims=['group'], values=[5.0, 3.0, 6.0]), coords={'group': groups}
    )
    assert sc.identical(binned.bins.max(), expected)


def test_bins_nanmax() -> None:
    data = sc.DataArray(
        data=sc.array(dims=['position'], values=[1.0, 2.0, -np.nan, 5.0, 6.0]),
        coords={'group': sc.array(dims=['position'], values=[0, 1, 1, 0, 2])},
    )
    groups = sc.array(dims=['group'], values=[0, 1, 2])
    binned = data.group(groups)
    expected = sc.DataArray(
        sc.array(dims=['group'], values=[5.0, 2.0, 6.0]), coords={'group': groups}
    )
    assert sc.identical(binned.bins.nanmax(), expected)


def test_bins_min() -> None:
    data = sc.DataArray(
        data=sc.array(dims=['position'], values=[1.0, 2.0, 3.0, 5.0, 6.0]),
        coords={'group': sc.array(dims=['position'], values=[0, 1, 1, 0, 2])},
    )
    groups = sc.array(dims=['group'], values=[0, 1, 2])
    binned = data.group(groups)
    expected = sc.DataArray(
        sc.array(dims=['group'], values=[1.0, 2.0, 6.0]), coords={'group': groups}
    )
    assert sc.identical(binned.bins.min(), expected)


def test_bins_nanmin() -> None:
    data = sc.DataArray(
        data=sc.array(dims=['position'], values=[1.0, 2.0, -np.nan, 5.0, 6.0]),
        coords={'group': sc.array(dims=['position'], values=[0, 1, 1, 0, 2])},
    )
    groups = sc.array(dims=['group'], values=[0, 1, 2])
    binned = data.group(groups)
    expected = sc.DataArray(
        sc.array(dims=['group'], values=[1.0, 2.0, 6.0]), coords={'group': groups}
    )
    assert sc.identical(binned.bins.nanmin(), expected)


def test_bins_all() -> None:
    data = sc.DataArray(
        data=sc.array(dims=['position'], values=[True, False, True, True, True]),
        coords={'group': sc.array(dims=['position'], values=[0, 1, 1, 0, 2])},
    )
    groups = sc.array(dims=['group'], values=[0, 1, 2])
    binned = data.group(groups)
    expected = sc.DataArray(
        sc.array(dims=['group'], values=[True, False, True]), coords={'group': groups}
    )
    assert sc.identical(binned.bins.all(), expected)


def test_bins_any() -> None:
    data = sc.DataArray(
        data=sc.array(dims=['position'], values=[False, False, True, False, False]),
        coords={'group': sc.array(dims=['position'], values=[0, 1, 1, 0, 2])},
    )
    groups = sc.array(dims=['group'], values=[0, 1, 2])
    binned = data.group(groups)
    expected = sc.DataArray(
        sc.array(dims=['group'], values=[False, True, False]), coords={'group': groups}
    )
    assert sc.identical(binned.bins.any(), expected)


def test_bins_like() -> None:
    data = sc.array(dims=['row'], values=[1, 2, 3, 4])
    begin = sc.array(dims=['x'], values=[0, 3], dtype=sc.DType.int64, unit=None)
    end = sc.array(dims=['x'], values=[3, 4], dtype=sc.DType.int64, unit=None)
    binned = sc.bins(begin=begin, end=end, dim='row', data=data)
    dense = sc.array(dims=['x'], values=[1.1, 2.2])
    expected_data = sc.array(dims=['row'], values=[1.1, 1.1, 1.1, 2.2])
    expected = sc.bins(begin=begin, end=end, dim='row', data=expected_data)
    # Prototype is binned variable
    assert sc.identical(sc.bins_like(binned, dense), expected)
    # Prototype is data array with binned data
    binned = sc.DataArray(data=binned)
    assert sc.identical(sc.bins_like(binned, dense), expected)
    # Broadcast
    expected_data = sc.array(dims=['row'], values=[1.1, 1.1, 1.1, 1.1])
    expected = sc.bins(begin=begin, end=end, dim='row', data=expected_data)
    assert sc.identical(sc.bins_like(binned, dense['x', 0]), expected)
    dense = dense.rename_dims({'x': 'y'})
    with pytest.raises(sc.DimensionError):
        (sc.bins_like(binned, dense),)


def test_bins_like_raises_when_given_data_group() -> None:
    binned = sc.data.binned_x(100, 10)
    dg = sc.DataGroup(a=binned.data)
    with pytest.raises(ValueError, match='DataGroup argument'):
        sc.bins_like(dg, sc.scalar(0.1))


def test_bins_concat() -> None:
    table = sc.data.table_xyz(nrow=100)
    table.data = sc.arange('row', 100, dtype='float64')
    da = table.bin(x=4, y=5)
    assert sc.identical(da.bins.concat('x').hist(), table.hist(y=5))
    assert sc.identical(da.bins.concat('y').hist(), table.hist(x=4))
    assert sc.identical(da.bins.concat().hist(), table.sum())


def test_bins_concat_variable() -> None:
    table = sc.data.table_xyz(nrow=100)
    table.data = sc.arange('row', 100, dtype='float64')
    da = table.bin(x=4, y=5)
    assert sc.identical(da.data.bins.concat('x'), da.bins.concat('x').data)


def test_bins_concat_content_variable() -> None:
    table = sc.data.table_xyz(nrow=100)
    table.data = sc.arange('row', 100, dtype='float64')
    da = table.bin(x=4, y=5)
    assert sc.identical(da.bins.data.bins.concat('x'), da.bins.concat('x').bins.data)


@pytest.mark.skip(reason="Need fix in Variable::setSlice")
def test_bins_concat_content_dataset() -> None:
    table = sc.data.table_xyz(nrow=100)
    table.data = sc.arange('row', 100, dtype='float64')
    da = table.bin(x=4, y=5)
    constituents = table.bin(x=4, y=5).bins.constituents
    constituents['data'] = sc.Dataset({'a': table, 'b': table + table})
    binned = sc.bins(**constituents)
    assert sc.identical(binned.bins.concat('x').bins['a'], da.bins.concat('x'))


def test_bins_concat_along_outer_length_1_dim_equivalent_to_squeeze() -> None:
    table = sc.data.table_xyz(nrow=100)
    da = table.bin(x=1, y=5, z=7)
    expected = da.squeeze()
    del expected.coords['x']
    assert sc.identical(da.bins.concat('x'), expected)


def test_bins_concat_along_middle_length_1_dim_equivalent_to_squeeze() -> None:
    table = sc.data.table_xyz(nrow=100)
    da = table.bin(x=5, y=1, z=7)
    expected = da.squeeze()
    del expected.coords['y']
    assert sc.identical(da.bins.concat('y'), expected)


def test_bins_concat_along_inner_length_1_dim_equivalent_to_squeeze() -> None:
    table = sc.data.table_xyz(nrow=100)
    da = table.bin(x=5, y=7, z=1)
    expected = da.squeeze()
    del expected.coords['z']
    assert sc.identical(da.bins.concat('z'), expected)


def test_bins_concat_gives_same_result_on_transposed() -> None:
    table = sc.data.table_xyz(nrow=100)
    da = table.bin(x=5, y=13)
    assert sc.identical(da.bins.concat('x'), da.transpose().bins.concat('x'))


def test_bins_concat_preserves_unrelated_mask() -> None:
    table = sc.data.table_xyz(nrow=100)
    da = table.bin(x=5, y=13)
    da.masks['masky'] = sc.zeros(dims=['y'], shape=[13], dtype=bool)
    result = da.bins.concat('x')
    assert 'masky' in result.masks


def test_bins_concat_applies_irreducible_masks() -> None:
    table = sc.data.table_xyz(nrow=10)
    da = table.bin(x=5, y=13)
    da.masks['maskx'] = sc.array(dims=['x'], values=[False, False, False, False, True])
    assert sc.identical(da.bins.concat('x'), da['x', :4].bins.concat('x'))


@pytest.mark.parametrize('aligned', [True, False])
def test_bin_1d_by_coord_without_event_coord(aligned: bool) -> None:
    table = sc.data.table_xyz(nrow=1000)
    da = table.bin(x=100)
    rng = default_rng(seed=1234)
    param = sc.array(dims='x', values=rng.random(da.sizes['x']))
    da.coords['param'] = param
    da.coords.set_aligned('param', aligned)
    edges = sc.linspace('param', 0.0, 1.0, num=13)
    from scipp.core.bin_remapping import combine_bins

    result = combine_bins(da, [edges], [], ['x'])
    # Setup flat table with param for computing expected result
    da.bins.coords['param'] = sc.bins_like(da, param)
    table = da.bins.constituents['data']
    assert sc.identical(result.hist(), table.hist(param=edges))


def test_bin_outer_of_2d_without_event_coord() -> None:
    table = sc.data.table_xyz(nrow=1000)
    table.data = sc.arange('row', 1000, dtype='float64')
    da = table.bin(x=7, y=3)
    rng = default_rng(seed=1234)
    param = sc.array(dims='x', values=rng.random(da.sizes['x']))
    da.coords['param'] = param
    edges = sc.linspace('param', 0.0, 1.0, num=5)
    from scipp.core.bin_remapping import combine_bins

    result = combine_bins(da, [edges], [], ['x'])
    # Setup flat table with param for computing expected result
    da.bins.coords['param'] = sc.bins_like(da, param)
    table = da.bins.constituents['data']
    assert sc.identical(result.hist(), table.hist(y=3, param=edges))


def test_bin_combined_outer_and_inner_of_2d_without_event_coord() -> None:
    table = sc.data.table_xyz(nrow=1000)
    table.data = sc.arange('row', 1000, dtype='float64')
    da = table.bin(x=7, y=3)
    rng = default_rng(seed=1234)
    param = sc.array(dims=['x', 'y'], values=rng.random(da.shape))
    edges = sc.linspace('param', 0.0, 1.0, num=5)
    from scipp.core.bin_remapping import combine_bins

    da.coords['param'] = param
    result_xy = combine_bins(da, [edges], [], ['x', 'y'])
    da.coords['param'] = param.transpose()
    result_yx = combine_bins(da, [edges], [], ['x', 'y'])
    assert sc.identical(result_xy, result_yx)
    # Setup flat table with param for computing expected result
    da.bins.coords['param'] = sc.bins_like(da, param)
    table = da.bins.constituents['data']
    assert sc.identical(result_xy.hist(), table.hist(param=edges))


def test_bin_outer_and_inner_of_2d_without_event_coord() -> None:
    table = sc.data.table_xyz(nrow=1000)
    table.data = sc.arange('row', 1000, dtype='float64')
    da = table.bin(x=7, y=3)
    rng = default_rng(seed=1234)
    x = sc.array(dims=['x'], values=rng.random(da.sizes['x']))
    y = sc.array(dims=['y'], values=rng.random(da.sizes['y']))
    xnew = sc.linspace('xnew', 0.0, 1.0, num=5)
    ynew = sc.linspace('ynew', 0.0, 1.0, num=4)
    from scipp.core.bin_remapping import combine_bins

    da.coords['xnew'] = x
    da.coords['ynew'] = y
    result = combine_bins(da, [xnew, ynew], [], ['x', 'y'])
    # Setup flat table with param for computing expected result
    da.bins.coords['xnew'] = sc.bins_like(da, x)
    da.bins.coords['ynew'] = sc.bins_like(da, y)
    table = da.bins.constituents['data']
    assert sc.identical(result.hist(), table.hist(xnew=xnew, ynew=ynew))


def test_bins_validate_indices_true() -> None:
    """Test that sc.bins validates indices when validate_indices=True."""
    data = sc.Variable(dims=['x'], values=[1, 2, 3, 4])
    begin = sc.Variable(dims=['y'], values=[0, 2], dtype=sc.DType.int64, unit=None)
    end = sc.Variable(dims=['y'], values=[2, 5], dtype=sc.DType.int64, unit=None)
    # End index 5 is out of bounds for data with size 4
    with pytest.raises(IndexError):
        sc.bins(begin=begin, end=end, dim='x', data=data, validate_indices=True)


def test_bins_validate_indices_false() -> None:
    """Test that sc.bins skips validation when validate_indices=False."""
    data = sc.Variable(dims=['x'], values=[1, 2, 3, 4])
    begin = sc.Variable(dims=['y'], values=[0, 2], dtype=sc.DType.int64, unit=None)
    end = sc.Variable(dims=['y'], values=[2, 5], dtype=sc.DType.int64, unit=None)
    # No exception with validate_indices=False, even with out-of-bounds indices
    var = sc.bins(begin=begin, end=end, dim='x', data=data, validate_indices=False)
    # The first bin should work correctly
    assert sc.identical(var['y', 0].value, data['x', 0:2])
    # The second bin has an out-of-bounds index, so accessing it may cause undefined
    # behavior


def test_bins_validate_indices_default() -> None:
    """Test that sc.bins validates indices by default."""
    data = sc.Variable(dims=['x'], values=[1, 2, 3, 4])
    begin = sc.Variable(dims=['y'], values=[0, 2], dtype=sc.DType.int64, unit=None)
    end = sc.Variable(dims=['y'], values=[2, 5], dtype=sc.DType.int64, unit=None)
    # By default, validate_indices=True, so this should raise an exception
    with pytest.raises(IndexError):
        sc.bins(begin=begin, end=end, dim='x', data=data)


def test_bins_validate_indices_with_valid_indices() -> None:
    """Test that both validation modes work with valid indices."""
    data = sc.Variable(dims=['x'], values=[1, 2, 3, 4])
    begin = sc.Variable(dims=['y'], values=[0, 2], dtype=sc.DType.int64, unit=None)
    end = sc.Variable(dims=['y'], values=[2, 4], dtype=sc.DType.int64, unit=None)

    # Both should work with valid indices
    var1 = sc.bins(begin=begin, end=end, dim='x', data=data, validate_indices=True)
    var2 = sc.bins(begin=begin, end=end, dim='x', data=data, validate_indices=False)

    # Results should be identical
    assert sc.identical(var1, var2)
    assert sc.identical(var1['y', 0].value, data['x', 0:2])
    assert sc.identical(var1['y', 1].value, data['x', 2:4])
    assert sc.identical(var2['y', 0].value, data['x', 0:2])
    assert sc.identical(var2['y', 1].value, data['x', 2:4])
