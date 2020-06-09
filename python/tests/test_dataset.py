# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import numpy as np
import pytest

import scipp as sc


def test_shape():
    a = sc.Variable(value=1)
    d = sc.Dataset(data={'a': a})
    assert d.shape == []
    a = sc.Variable(['x'], shape=[2])
    b = sc.Variable(['y', 'z'], shape=[3, 4])
    d = sc.Dataset(data={'a': a, 'b': b})
    assert not bool(set(d.shape) - set([4, 3, 2]))


def test_create_empty():
    d = sc.Dataset()
    assert len(d) == 0
    assert len(d.coords) == 0
    assert len(d.masks) == 0
    assert len(d.attrs) == 0
    assert len(d.dims) == 0


def test_create():
    x = sc.Variable(dims=['x'], values=np.arange(3))
    y = sc.Variable(dims=['y'], values=np.arange(4))
    xy = sc.Variable(dims=['x', 'y'], values=np.arange(12).reshape(3, 4))
    d = sc.Dataset({'xy': xy, 'x': x}, coords={'x': x, 'y': y})
    assert len(d) == 2
    assert d.coords['x'] == x
    assert d.coords['y'] == y
    assert d['xy'].data == xy
    assert d['x'].data == x
    assert bool(set(d.dims) - set(['y', 'x']))


def test_create_from_data_array():
    var = sc.Variable(dims=['x'], values=np.arange(4))
    da = sc.DataArray(var, coords={'x': var, 'aux': var})
    d = sc.Dataset(da)
    assert d[''] == da


def test_create_from_data_arrays():
    var1 = sc.Variable(dims=['x'], values=np.arange(4))
    var2 = sc.Variable(dims=['x'], values=np.ones(4))
    da1 = sc.DataArray(var1, coords={'x': var1, 'aux': var2})
    da2 = sc.DataArray(var1, coords={'x': var1, 'aux': var2})
    d = sc.Dataset()
    d['a'] = da1
    d['b'] = da2
    assert d == sc.Dataset(data={
        'a': var1,
        'b': var1
    },
                           coords={
                               'x': var1,
                               'aux': var2
                           })


def test_clear():
    d = sc.Dataset()
    d['a'] = sc.Variable(dims=['x'], values=np.arange(3))
    assert 'a' in d
    d.clear()
    assert len(d) == 0


def test_setitem():
    d = sc.Dataset()
    d['a'] = sc.Variable(1.0)
    assert len(d) == 1
    assert d['a'].data == sc.Variable(1.0)
    assert len(d.coords) == 0


def test_del_item():
    d = sc.Dataset()
    d['a'] = sc.Variable(1.0)
    assert 'a' in d
    del d['a']
    assert 'a' not in d


def test_del_item_missing():
    d = sc.Dataset()
    with pytest.raises(RuntimeError):
        del d['not an item']


def test_coord_setitem():
    var = sc.Variable(dims=['x'], values=np.arange(4))
    d = sc.Dataset({'a': var}, coords={'x': var})
    with pytest.raises(RuntimeError):
        d['x', 2:3].coords['y'] = sc.Variable(1.0)
    d.coords['y'] = sc.Variable(1.0)
    assert len(d) == 1
    assert len(d.coords) == 2
    assert d.coords['y'] == sc.Variable(1.0)


def test_coord_setitem_events():
    events = sc.Variable(dims=[], shape=[], dtype=sc.dtype.event_list_float64)
    d = sc.Dataset({'a': events})
    d.coords['x'] = events
    assert len(d.coords) == 1
    assert len(d['a'].coords) == 1
    assert d['a'].coords['x'] == events
    assert d['a'].coords['x'] == d.coords['x']


def test_create_events_via_DataArray():
    d = sc.Dataset()
    events = sc.Variable(dims=[], shape=[], dtype=sc.dtype.event_list_float64)
    d['a'] = sc.DataArray(data=events, coords={'x': events})
    assert len(d.coords) == 1
    assert len(d['a'].coords) == 1
    assert d['a'].coords['x'] == events
    assert d['a'].coords['x'] == d.coords['x']


def test_contains_coord():
    d = sc.Dataset()
    assert 'x' not in d.coords
    d.coords['x'] = sc.Variable(1.0)
    assert 'x' in d.coords


def test_coords_keys():
    d = sc.Dataset()
    d.coords['x'] = sc.Variable(1.0)
    assert len(d.coords.keys()) == 1
    assert 'x' in d.coords.keys()


def test_masks_setitem():
    var = sc.Variable(dims=['x'], values=np.arange(4))
    d = sc.Dataset({'a': var}, coords={'x': var})

    with pytest.raises(RuntimeError):
        d['x', 2:3].coords['label'] = sc.Variable(True)
    d.masks['mask'] = sc.Variable(dims=['x'],
                                  values=np.array([True, False, True, False]))
    assert len(d) == 1
    assert len(d.masks) == 1
    assert d.masks['mask'] == sc.Variable(dims=['x'],
                                          values=np.array(
                                              [True, False, True, False]))


def test_contains_masks():
    d = sc.Dataset()
    assert 'a' not in d.masks
    d.masks['a'] = sc.Variable(True)
    assert 'a' in d.masks


def test_attrs_setitem():
    var = sc.Variable(dims=['x'], values=np.arange(4))
    d = sc.Dataset({'a': var}, coords={'x': var})
    with pytest.raises(RuntimeError):
        d['x', 2:3].attrs['attr'] = sc.Variable(1.0)
    d.attrs['attr'] = sc.Variable(1.0)
    assert len(d) == 1
    assert len(d.attrs) == 1
    assert d.attrs['attr'] == sc.Variable(1.0)


def test_attrs_setitem_events():
    var = sc.Variable(dims=['x'], values=np.arange(4))
    events = sc.Variable(dims=[], shape=[], dtype=sc.dtype.event_list_float64)
    d = sc.Dataset({'a': events}, coords={'x': var})
    d.attrs['attr'] = events
    d['a'].attrs['attr'] = events


def test_contains_attrs():
    d = sc.Dataset()
    assert 'b' not in d.attrs
    d.attrs['b'] = sc.Variable(1.0)
    assert 'b' in d.attrs


def test_attrs_keys():
    d = sc.Dataset()
    d.attrs['b'] = sc.Variable(1.0)
    assert len(d.attrs.keys()) == 1
    assert 'b' in d.attrs.keys()


def test_slice_item():
    d = sc.Dataset(
        coords={'x': sc.Variable(dims=['x'], values=np.arange(4, 8))})
    d['a'] = sc.Variable(dims=['x'], values=np.arange(4))
    assert d['a']['x', 2:4].data == sc.Variable(dims=['x'],
                                                values=np.arange(2, 4))
    assert d['a']['x', 2:4].coords['x'] == sc.Variable(dims=['x'],
                                                       values=np.arange(6, 8))


def test_set_item_slice_from_numpy():
    d = sc.Dataset(
        coords={'x': sc.Variable(dims=['x'], values=np.arange(4, 8))})
    d['a'] = sc.Variable(dims=['x'], values=np.arange(4))
    d['a']['x', 2:4] = np.arange(2)
    assert d['a'].data == sc.Variable(dims=['x'],
                                      values=np.array([0, 1, 0, 1]))


def test_set_item_slice_with_variances_from_numpy():
    d = sc.Dataset(
        coords={'x': sc.Variable(dims=['x'], values=np.arange(4, 8))})
    d['a'] = sc.Variable(dims=['x'],
                         values=np.arange(4.0),
                         variances=np.arange(4.0))
    d['a']['x', 2:4].values = np.arange(2)
    d['a']['x', 2:4].variances = np.arange(2, 4)
    assert np.array_equal(d['a'].values, np.array([0.0, 1.0, 0.0, 1.0]))
    assert np.array_equal(d['a'].variances, np.array([0.0, 1.0, 2.0, 3.0]))


def test_events_setitem():
    d = sc.Dataset({
        'a':
        sc.Variable(dims=['x'], shape=[4], dtype=sc.dtype.event_list_float64)
    })
    d['a']['x', 0].values = np.arange(4)
    assert len(d['a']['x', 0].values) == 4


def test_iadd_slice():
    d = sc.Dataset(
        coords={'x': sc.Variable(dims=['x'], values=np.arange(4, 8))})
    d['a'] = sc.Variable(dims=['x'], values=np.arange(4))
    d['a']['x', 1] += d['a']['x', 2]
    assert d['a'].data == sc.Variable(dims=['x'],
                                      values=np.array([0, 3, 2, 3]))


def test_iadd_range():
    d = sc.Dataset(
        coords={'x': sc.Variable(dims=['x'], values=np.arange(4, 8))})
    d['a'] = sc.Variable(dims=['x'], values=np.arange(4))
    with pytest.raises(RuntimeError):
        d['a']['x', 2:4] += d['a']['x', 1:3]
    d['a']['x', 2:4] += d['a']['x', 2:4]
    assert d['a'].data == sc.Variable(dims=['x'],
                                      values=np.array([0, 1, 4, 6]))


def test_contains():
    d = sc.Dataset()
    assert 'a' not in d
    d['a'] = sc.Variable(1.0)
    assert 'a' in d
    assert 'b' not in d
    d['b'] = sc.Variable(1.0)
    assert 'a' in d
    assert 'b' in d


def test_slice():
    d = sc.Dataset(
        {
            'a': sc.Variable(dims=['x'], values=np.arange(10.0)),
            'b': sc.Variable(1.0)
        },
        coords={'x': sc.Variable(dims=['x'], values=np.arange(10.0))})
    expected = sc.Dataset({
        'a':
        sc.DataArray(1.0 * sc.units.one, attrs={'x': 1.0 * sc.units.one})
    })

    assert d['x', 1] == expected
    assert 'a' in d['x', 1]
    assert 'b' not in d['x', 1]


def test_chained_slicing():
    x = sc.Variable(dims=['x'], values=np.arange(11.0))
    y = sc.Variable(dims=['y'], values=np.arange(11.0))
    z = sc.Variable(dims=['z'], values=np.arange(11.0))
    d = sc.Dataset(
        {
            'a':
            sc.Variable(dims=['z', 'y', 'x'],
                        values=np.arange(1000.0).reshape(10, 10, 10)),
            'b':
            sc.Variable(dims=['x', 'z'],
                        values=np.arange(0.0, 10.0, 0.1).reshape(10, 10))
        },
        coords={
            'x': x,
            'y': y,
            'z': z
        })

    expected = sc.Dataset()
    expected.coords['y'] = sc.Variable(dims=['y'], values=np.arange(11.0))
    expected['a'] = sc.Variable(dims=['y'],
                                values=np.arange(501.0, 600.0, 10.0))
    expected['b'] = sc.Variable(1.5)
    expected['a'].attrs['x'] = x['x', 1:3]
    expected['b'].attrs['x'] = x['x', 1:3]
    expected['a'].attrs['z'] = z['z', 5:7]
    expected['b'].attrs['z'] = z['z', 5:7]

    assert d['x', 1]['z', 5] == expected


def test_coords_view_comparison_operators():
    d = sc.Dataset(
        {
            'a': sc.Variable(dims=['x'], values=np.arange(10.0)),
            'b': sc.Variable(1.0)
        },
        coords={'x': sc.Variable(dims=['x'], values=np.arange(10.0))})

    d1 = sc.Dataset(
        {
            'a': sc.Variable(dims=['x'], values=np.arange(10.0)),
            'b': sc.Variable(1.0)
        },
        coords={'x': sc.Variable(dims=['x'], values=np.arange(10.0))})
    assert d1['a'].coords == d['a'].coords


def test_sum_mean():
    d = sc.Dataset(
        {
            'a':
            sc.Variable(dims=['x', 'y'],
                        values=np.arange(6, dtype=np.int64).reshape(2, 3)),
            'b':
            sc.Variable(dims=['y'], values=np.arange(3, dtype=np.int64))
        },
        coords={
            'x':
            sc.Variable(dims=['x'], values=np.arange(2, dtype=np.int64)),
            'y':
            sc.Variable(dims=['y'], values=np.arange(3, dtype=np.int64)),
            'l1':
            sc.Variable(dims=['x', 'y'],
                        values=np.arange(6, dtype=np.int64).reshape(2, 3)),
            'l2':
            sc.Variable(dims=['x'], values=np.arange(2, dtype=np.int64))
        })
    d_ref = sc.Dataset(
        {
            'a': sc.Variable(dims=['x'],
                             values=np.array([3, 12], dtype=np.int64)),
            'b': sc.Variable(3)
        },
        coords={
            'x': sc.Variable(dims=['x'], values=np.arange(2, dtype=np.int64)),
            'l2': sc.Variable(dims=['x'], values=np.arange(2, dtype=np.int64))
        })

    assert sc.sum(d, 'y') == d_ref
    assert (sc.mean(d, 'y')['a'].values == [1.0, 4.0]).all()
    assert sc.mean(d, 'y')['b'].value == 1.0


def test_sum_masked():
    d = sc.Dataset(
        {
            'a':
            sc.Variable(dims=['x'],
                        values=np.array([1, 5, 4, 5, 1], dtype=np.int64))
        },
        masks={
            'm1':
            sc.Variable(dims=['x'],
                        values=np.array([False, True, False, True, False]))
        })

    d_ref = sc.Dataset({'a': sc.Variable(np.int64(6))})

    result = sc.sum(d, 'x')['a']
    assert result == d_ref['a']


def test_mean_masked():
    d = sc.Dataset(
        {'a': sc.Variable(dims=['x'], values=np.array([1, 5, 4, 5, 1]))},
        masks={
            'm1':
            sc.Variable(dims=['x'],
                        values=np.array([False, True, False, True, False]))
        })
    d_ref = sc.Dataset({'a': sc.Variable(2.0)})
    assert sc.mean(d, 'x')['a'] == d_ref['a']


def test_variable_histogram():
    var = sc.Variable(dims=['x'], shape=[2], dtype=sc.dtype.event_list_float64)
    var['x', 0].values = np.arange(3)
    var['x', 0].values.append(42)
    var['x', 0].values.extend(np.ones(3))
    var['x', 1].values = np.ones(6)
    ds = sc.Dataset()
    ds['events'] = sc.DataArray(data=sc.Variable(dims=['x'],
                                                 values=np.ones(2),
                                                 variances=np.ones(2)),
                                coords={'y': var})
    hist = sc.histogram(
        ds['events'],
        sc.Variable(values=np.arange(5, dtype=np.float64), dims=['y']))
    assert np.array_equal(
        hist.values, np.array([[1.0, 4.0, 1.0, 0.0], [0.0, 6.0, 0.0, 0.0]]))


def test_dataset_histogram():
    var = sc.Variable(dims=['x'], shape=[2], dtype=sc.dtype.event_list_float64)
    var['x', 0].values = np.arange(3)
    var['x', 0].values.append(42)
    var['x', 0].values.extend(np.ones(3))
    var['x', 1].values = np.ones(6)
    ds = sc.Dataset()
    s = sc.DataArray(data=sc.Variable(dims=['x'],
                                      values=np.ones(2),
                                      variances=np.ones(2)),
                     coords={'y': var})
    s1 = sc.DataArray(data=sc.Variable(dims=['x'],
                                       values=np.ones(2),
                                       variances=np.ones(2)),
                      coords={'y': var * 5.0})
    realign_coords = {
        'y': sc.Variable(values=np.arange(5, dtype=np.float64), dims=['y'])
    }
    ds['s'] = sc.realign(s, realign_coords)
    ds['s1'] = sc.realign(s1, realign_coords)
    h = sc.histogram(ds)
    assert np.array_equal(
        h['s'].values, np.array([[1.0, 4.0, 1.0, 0.0], [0.0, 6.0, 0.0, 0.0]]))
    assert np.array_equal(
        h['s1'].values, np.array([[1.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0]]))


def test_histogram_and_setitem():
    var = sc.Variable(dims=['x'],
                      shape=[2],
                      dtype=sc.dtype.event_list_float64,
                      unit=sc.units.us)
    var['x', 0].values = np.arange(3)
    var['x', 0].values.append(42)
    var['x', 0].values.extend(np.ones(3))
    var['x', 1].values = np.ones(6)
    ds = sc.Dataset()
    ds['s'] = sc.DataArray(data=sc.Variable(dims=['x'],
                                            values=np.ones(2),
                                            variances=np.ones(2)),
                           coords={'tof': var})
    assert 'tof' in ds.coords
    assert 'tof' in ds['s'].coords
    edges = sc.Variable(dims=['tof'], values=np.arange(5.0), unit=sc.units.us)
    h = sc.histogram(ds['s'], edges)
    assert np.array_equal(
        h.values, np.array([[1.0, 4.0, 1.0, 0.0], [0.0, 6.0, 0.0, 0.0]]))
    assert 'tof' in ds.coords


def test_dataset_merge():
    a = sc.Dataset({'d1': sc.Variable(dims=['x'], values=np.array([1, 2, 3]))})
    b = sc.Dataset({'d2': sc.Variable(dims=['x'], values=np.array([4, 5, 6]))})
    c = sc.merge(a, b)
    assert a['d1'] == c['d1']
    assert b['d2'] == c['d2']


def test_dataset_concatenate():
    a = sc.Dataset(
        {'data': sc.Variable(dims=['x'], values=np.array([11, 12]))},
        coords={'x': sc.Variable(dims=['x'], values=np.array([1, 2]))})
    b = sc.Dataset(
        {'data': sc.Variable(dims=['x'], values=np.array([13, 14]))},
        coords={'x': sc.Variable(dims=['x'], values=np.array([3, 4]))})

    c = sc.concatenate(a, b, 'x')

    assert np.array_equal(c.coords['x'].values, np.array([1, 2, 3, 4]))
    assert np.array_equal(c['data'].values, np.array([11, 12, 13, 14]))


def test_dataset_set_data():
    d1 = sc.Dataset(
        {
            'a': sc.Variable(dims=['x', 'y'], values=np.random.rand(2, 3)),
            'b': sc.Variable(1.0)
        },
        coords={
            'x': sc.Variable(dims=['x'],
                             values=np.arange(2.0),
                             unit=sc.units.m),
            'y': sc.Variable(dims=['y'],
                             values=np.arange(3.0),
                             unit=sc.units.m),
            'aux': sc.Variable(dims=['y'], values=np.arange(3))
        })

    d2 = sc.Dataset(
        {
            'a': sc.Variable(dims=['x', 'y'], values=np.random.rand(2, 3)),
            'b': sc.Variable(1.0)
        },
        coords={
            'x': sc.Variable(dims=['x'],
                             values=np.arange(2.0),
                             unit=sc.units.m),
            'y': sc.Variable(dims=['y'],
                             values=np.arange(3.0),
                             unit=sc.units.m),
            'aux': sc.Variable(dims=['y'], values=np.arange(3))
        })

    d3 = sc.Dataset()
    d3['b'] = d1['a']
    assert d3['b'].data == d1['a'].data
    assert d3['b'].coords == d1['a'].coords
    d1['a'] = d2['a']
    d1['c'] = d2['a']
    assert d2['a'].data == d1['a'].data
    assert d2['a'].data == d1['c'].data

    d = sc.Dataset()
    d.coords['row'] = sc.Variable(dims=['row'], values=np.arange(10.0))
    d['a'] = sc.Variable(dims=['row'],
                         values=np.arange(10.0),
                         variances=np.arange(10.0))
    d['b'] = sc.Variable(dims=['row'], values=np.arange(10.0, 20.0))
    d1 = d['row', 0:1]
    d2 = sc.Dataset({'a': d1['a'].data}, coords={'row': d1['a'].coords['row']})
    d2['b'] = d1['b']
    expected = sc.Dataset()
    expected.coords['row'] = sc.Variable(dims=['row'], values=np.arange(1.0))
    expected['a'] = sc.Variable(dims=['row'],
                                values=np.arange(1.0),
                                variances=np.arange(1.0))
    expected['b'] = sc.Variable(dims=['row'], values=np.arange(10.0, 11.0))
    assert d2 == expected


def test_dataset_data_access():
    var = sc.Variable(dims=['x'], shape=[2], dtype=sc.dtype.event_list_float64)
    ds = sc.Dataset()
    ds['events'] = sc.DataArray(data=sc.Variable(dims=['x'],
                                                 values=np.ones(2),
                                                 variances=np.ones(2)),
                                coords={'y': var})
    assert ds['events'].values is not None


def test_binary_with_broadcast():
    d = sc.Dataset(
        {'data': sc.Variable(dims=['x'], values=np.arange(10.0))},
        coords={'x': sc.Variable(dims=['x'], values=np.arange(10.0))})

    d2 = d - d['x', 0]
    d -= d['x', 0]
    assert d == d2


def test_binary__with_dataarray():
    da = sc.DataArray(
        data=sc.Variable(dims=['x'], values=np.arange(1.0, 10.0)),
        coords={'x': sc.Variable(dims=['x'], values=np.arange(1.0, 10.0))})
    ds = sc.Dataset(da)
    orig = ds.copy()
    ds += da
    ds -= da
    ds *= da
    ds /= da
    assert ds == orig


def test_binary_of_item_with_variable():
    d = sc.Dataset(
        {'data': sc.Variable(dims=['x'], values=np.arange(10.0))},
        coords={'x': sc.Variable(dims=['x'], values=np.arange(10.0))})
    copy = d.copy()

    d['data'] += 2.0 * sc.units.dimensionless
    d['data'] *= 2.0 * sc.units.m
    d['data'] -= 4.0 * sc.units.m
    d['data'] /= 2.0 * sc.units.m
    assert d == copy


def test_in_place_binary_with_scalar():
    d = sc.Dataset({'data': sc.Variable(dims=['x'], values=[10])},
                   coords={'x': sc.Variable(dims=['x'], values=[10])})
    copy = d.copy()

    d += 2
    d *= 2
    d -= 4
    d /= 2
    assert d == copy


def test_view_in_place_binary_with_scalar():
    d = sc.Dataset({'data': sc.Variable(dims=['x'], values=[10])},
                   coords={'x': sc.Variable(dims=['x'], values=[10])})
    copy = d.copy()

    d['data'] += 2
    d['data'] *= 2
    d['data'] -= 4
    d['data'] /= 2
    assert d == copy


def test_add_sum_of_columns():
    d = sc.Dataset({
        'a': sc.Variable(dims=['x'], values=np.arange(10.0)),
        'b': sc.Variable(dims=['x'], values=np.ones(10))
    })
    d['sum'] = d['a'] + d['b']
    d['a'] += d['b']
    assert d['sum'] == d['a']


def test_name():
    d = sc.Dataset(
        {
            'a': sc.Variable(dims=['x'], values=np.arange(10.0)),
            'b': sc.Variable(dims=['x'], values=np.ones(10))
        },
        coords={'x': sc.Variable(dims=['x'], values=np.arange(10))})
    assert d['a'].name == 'a'
    assert d['b'].name == 'b'
    assert d['x', 2]['b'].name == 'b'
    assert d['b']['x', 2].name == 'b'
    array = d['a'] + d['b']
    assert array.name == ''


def make_simple_dataset(dim1='x', dim2='y', seed=None):
    if seed is not None:
        np.random.seed(seed)
    return sc.Dataset(
        {
            'a': sc.Variable(dims=[dim1, dim2], values=np.random.rand(2, 3)),
            'b': sc.Variable(1.0)
        },
        coords={
            dim1: sc.Variable(dims=[dim1],
                              values=np.arange(2.0),
                              unit=sc.units.m),
            dim2: sc.Variable(dims=[dim2],
                              values=np.arange(3.0),
                              unit=sc.units.m),
            'aux': sc.Variable(dims=[dim2], values=np.random.rand(3))
        })


def test_dataset_view_set_variance():
    d = make_simple_dataset()
    variances = np.arange(6).reshape(2, 3)
    assert d['a'].variances is None
    d['a'].variances = variances
    assert d['a'].variances is not None
    np.testing.assert_array_equal(d['a'].variances, variances)
    d['a'].variances = None
    assert d['a'].variances is None


def test_sort():
    d = sc.Dataset(
        {
            'a': sc.Variable(dims=['x', 'y'],
                             values=np.arange(6).reshape(2, 3)),
            'b': sc.Variable(dims=['x'], values=['b', 'a'])
        },
        coords={
            'x': sc.Variable(dims=['x'],
                             values=np.arange(2.0),
                             unit=sc.units.m),
            'y': sc.Variable(dims=['y'],
                             values=np.arange(3.0),
                             unit=sc.units.m),
            'aux': sc.Variable(dims=['x'], values=np.arange(2.0))
        })
    expected = sc.Dataset(
        {
            'a':
            sc.Variable(dims=['x', 'y'],
                        values=np.flip(np.arange(6).reshape(2, 3), axis=0)),
            'b':
            sc.Variable(dims=['x'], values=['a', 'b'])
        },
        coords={
            'x': sc.Variable(dims=['x'], values=[1.0, 0.0], unit=sc.units.m),
            'y': sc.Variable(dims=['y'],
                             values=np.arange(3.0),
                             unit=sc.units.m),
            'aux': sc.Variable(dims=['x'], values=[1.0, 0.0])
        })
    assert sc.sort(d, d['b'].data) == expected


def test_rename_dims():
    d = make_simple_dataset('x', 'y', seed=0)
    d.rename_dims({'y': 'z'})
    assert d == make_simple_dataset('x', 'z', seed=0)
    d.rename_dims(dims_dict={'x': 'y', 'z': 'x'})
    assert d == make_simple_dataset('y', 'x', seed=0)


def test_coord_delitem():
    var = sc.Variable(dims=['x'], values=np.arange(4))
    d = sc.Dataset({'a': var}, coords={'x': var})
    dref = d.copy()
    d.coords['y'] = sc.Variable(1.0)
    assert dref != d
    del d.coords['y']
    assert dref == d


def test_coords_delitem():
    var = sc.Variable(dims=['x'], values=np.arange(4))
    d = sc.Dataset({'a': var}, coords={'x': var})
    dref = d.copy()
    dref.coords['x'] = sc.Variable(dims=['x'], values=np.arange(1, 5))
    assert d != dref
    del dref.coords['x']
    assert d != dref
    dref.coords['x'] = d.coords['x']
    assert d == dref


def test_attrs_delitem():
    var = sc.Variable(dims=['x'], values=np.arange(4))
    d = sc.Dataset({'a': var}, coords={'x': var})
    dref = d.copy()
    d.attrs['attr'] = sc.Variable(1.0)
    assert d != dref
    del d.attrs['attr']
    assert d == dref


def test_masks_delitem():
    var = sc.Variable(dims=['x'], values=np.array([True, True, False]))
    d = sc.Dataset({'a': var}, coords={'x': var})
    dref = d.copy()
    d.masks['masks'] = var
    assert d != dref
    del d.masks['masks']
    assert d == dref


def test_replace():
    v1 = sc.Variable([sc.Dim.X], values=np.array([1, 2, 3]))
    d = sc.Dataset({'a': v1})
    d['a'].data == v1
    v2 = sc.Variable([sc.Dim.X], values=np.array([4, 5, 6]))
    d['a'].data == v2


def test_rebin():
    dataset = sc.Dataset()
    dataset['data'] = sc.Variable(['x'],
                                  values=np.array(10 * [1.0]),
                                  unit=sc.units.counts)
    dataset.coords['x'] = sc.Variable(['x'], values=np.arange(11.0))
    new_coord = sc.Variable(dims=['x'], values=np.arange(0.0, 11, 2))
    dataset = sc.rebin(dataset, 'x', new_coord)
    np.testing.assert_array_equal(dataset['data'].values, np.array(5 * [2]))
    np.testing.assert_array_equal(dataset.coords['x'].values,
                                  np.arange(0, 11, 2))


def _is_deep_copy_of(orig, copy):
    assert orig == copy
    assert not id(orig) == id(copy)


def test_copy():
    import copy
    a = sc.Dataset()
    a['x'] = sc.Variable(value=1)
    _is_deep_copy_of(a, a.copy())
    _is_deep_copy_of(a, copy.copy(a))
    _is_deep_copy_of(a, copy.deepcopy(a))


def test_correct_temporaries():
    N = 6
    M = 4
    d1 = sc.Dataset()
    d1['x'] = sc.Variable(['x'], values=np.arange(N + 1).astype(np.float64))
    d1['y'] = sc.Variable(['y'], values=np.arange(M + 1).astype(np.float64))
    arr1 = np.arange(N * M).reshape(N, M).astype(np.float64) + 1
    d1['A'] = sc.Variable(['x', 'y'], values=arr1)
    d1 = d1['x', 1:2]
    assert d1['A'].values.tolist() == [[5.0, 6.0, 7.0, 8.0]]
    d1 = d1['y', 2:3]
    assert list(d1['A'].values) == [7]


def test_iteration():
    var = sc.Variable(value=1)
    d = sc.Dataset(data={'a': var, 'b': var})
    expected = ['a', 'b']
    for k in d:
        assert k in expected
