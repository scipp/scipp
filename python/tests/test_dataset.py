# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import numpy as np
import pytest

import scipp as sc


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


def test_create_from_data_array():
    var = sc.Variable(dims=['x'], values=np.arange(4))
    base = sc.Dataset({'a': var}, coords={'x': var, 'aux': var})
    d = sc.Dataset(base['a'])
    assert d == base
    assert base['a'] == sc.DataArray(data=var, coords={'x': var, 'aux': var}) 


def test_create_from_data_arrays():
    var1 = sc.Variable(dims=['x'], values=np.arange(4))
    var2 = sc.Variable(dims=['x'], values=np.ones(4))
    base = sc.Dataset({'a': var1, 'b': var2}, coords={'x': var1, 'aux': var1})
    d = sc.Dataset({'a': base['a'], 'b': base['b']})
    assert d == base
    swapped = sc.Dataset({'a': base['b'], 'b': base['a']})
    assert swapped != base
    assert swapped['a'] == base['b']
    assert swapped['b'] == base['a']

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


def test_coord_setitem_sparse():
    events = sc.Variable(dims=[], shape=[], dtype=sc.dtype.event_list_float64)
    d = sc.Dataset({'a': events})
    d.coords['x'] = events
    assert len(d.coords) == 1
    assert len(d['a'].coords) == 1
    assert d['a'].coords['x'] == events
    assert d['a'].coords['x'] == d.coords['x']


def test_create_sparse_via_DataArray():
    d = sc.Dataset()
    sparse = sc.Variable(dims=[], shape=[], dtype=sc.dtype.event_list_float64)
    d['a'] = sc.DataArray(data=sparse, coords={'x': sparse})
    assert len(d.coords) == 1
    assert len(d['a'].coords) == 1
    assert d['a'].coords['x'] == sparse
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


def test_attrs_setitem_sparse():
    var = sc.Variable(dims=['x'], values=np.arange(4))
    sparse = sc.Variable(dims=[], shape=[], dtype=sc.dtype.event_list_float64)
    d = sc.Dataset({'a': sparse}, coords={'x': var})
    d.attrs['attr'] = sparse
    d['a'].attrs['attr'] = sparse


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


def test_sparse_setitem():
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
    expected = sc.Dataset({'a': sc.Variable(1.0)})

    assert d['x', 1] == expected
    assert 'a' in d['x', 1]
    assert 'b' not in d['x', 1]


def test_chained_slicing():
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
            'x': sc.Variable(dims=['x'], values=np.arange(11.0)),
            'y': sc.Variable(dims=['y'], values=np.arange(11.0)),
            'z': sc.Variable(dims=['z'], values=np.arange(11.0))
        })

    expected = sc.Dataset()
    expected.coords['y'] = sc.Variable(dims=['y'], values=np.arange(11.0))
    expected['a'] = sc.Variable(dims=['y'],
                                values=np.arange(501.0, 600.0, 10.0))
    expected['b'] = sc.Variable(1.5)

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
    ds['sparse'] = sc.DataArray(data=sc.Variable(dims=['x'],
                                                 values=np.ones(2),
                                                 variances=np.ones(2)),
                                coords={'y': var})
    hist = sc.histogram(
        ds['sparse'],
        sc.Variable(values=np.arange(5, dtype=np.float64), dims=['y']))
    assert np.array_equal(
        hist.values, np.array([[1.0, 4.0, 1.0, 0.0], [0.0, 6.0, 0.0, 0.0]]))


@pytest.mark.skip(reason="Requires unaligned dataset items")
def test_dataset_histogram():
    var = sc.Variable(dims=['x'], shape=[2], dtype=sc.dtype.event_list_float64)
    var['x', 0].values = np.arange(3)
    var['x', 0].values.append(42)
    var['x', 0].values.extend(np.ones(3))
    var['x', 1].values = np.ones(6)
    ds = sc.Dataset()
    ds['s'] = sc.DataArray(data=sc.Variable(dims=['x'],
                                            values=np.ones(2),
                                            variances=np.ones(2)),
                           coords={'y': var})
    ds['s1'] = sc.DataArray(data=sc.Variable(dims=['x'],
                                             values=np.ones(2),
                                             variances=np.ones(2)),
                            coords={'y': var * 5.0})
    h = sc.histogram(
        ds, sc.Variable(values=np.arange(5, dtype=np.float64), dims=['y']))
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
    ds['sparse'] = sc.DataArray(data=sc.Variable(dims=['x'],
                                                 values=np.ones(2),
                                                 variances=np.ones(2)),
                                coords={'y': var})
    assert ds['sparse'].values is not None


def test_binary_with_broadcast():
    d = sc.Dataset(
        {'data': sc.Variable(dims=['x'], values=np.arange(10.0))},
        coords={'x': sc.Variable(dims=['x'], values=np.arange(10.0))})

    d2 = d - d['x', 0]
    d -= d['x', 0]
    assert d == d2


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


@pytest.mark.skip(reason="Requires unaligned dataset items")
def test_coords_delitem_sparse():
    var = sc.Variable(dims=['x'], values=np.arange(4))
    sparse = sc.Variable(dims=[], shape=[], dtype=sc.dtype.event_list_float64)
    d = sc.Dataset({'a': sparse}, coords={'x': var})
    d['a'].coords['x'] = sparse
    with pytest.raises(RuntimeError):
        del d['a'].coords['z']
    del d['a'].coords['x']
    assert 'x' not in d['a'].coords
    with pytest.raises(RuntimeError):
        d['a'].coords['x']


def test_coords_delitem():
    var = sc.Variable(dims=['x'], values=np.arange(4))
    d = sc.Dataset({'a': var}, coords={'x': var})
    dref = d.copy()
    dref.coords['x'] = sc.Variable(dims=['x'], values=np.arange(1,5))
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

# test_realign - done in test_realign.py 
# test_delitem - tested elsewhere
# test_insert_default_init - not relevant
# test_insert - equivalent tested elsewhere
# test_replace - refactored
# test_insert_Variable - tested elsewhere
# test_demo_int_to_float_issue - not relevant? : Indeed issue gone
# test_binary_dataset_rhs_operations - tested elsewhere
# test_binary_variable_rhs_operations - tested elsewhere
# test_binary_float_operations - tested elsewhere
# test_equal_not_equal - tested elsewhere
# test_numpy_interoperable : Tested in test_variable.py
# test_concatenate: Tested above
# test_sort: Already done
# test_filter : Exports tested elsewhere 
#    dataset[sc.Data.Value, 'data'] = (['z', 'y', 'x'],
#                                      (1, 2, 3))
#    dataset[sc.Coord.X] = (['x'], np.arange(3))
#    del dataset[sc.Data.Value, 'data']
#    self.assertFalse((sc.Data.Value, 'data') in dataset)
#    self.assertEqual(len(dataset.dimensions), 1)
#    del dataset[sc.Coord.X]
#    self.assertFalse(sc.Coord.X in dataset)
#    self.assertEqual(len(dataset.dimensions), 0)
#
# def test_insert_default_init(self):
#    d = sc.Dataset()
#    d[sc.Data.Value, 'data1'] = (('z', 'y', 'x'), (4, 3, 2))
#    self.assertEqual(len(d), 1)
#    np.testing.assert_array_equal(
#        d[sc.Data.Value, 'data1'].numpy, np.zeros(shape=(4, 3, 2)))
#
# def test_insert(self):
#    d = sc.Dataset()
#    d[sc.Data.Value, 'data1'] = (
#        ['z', 'y', 'x'], np.arange(24).reshape(4, 3, 2))
#    self.assertEqual(len(d), 1)
#    np.testing.assert_array_equal(
#        d[sc.Data.Value, 'data1'].numpy, self.reference_data1)
#
#    self.assertRaisesRegex(
#        RuntimeError,
#        'Cannot insert variable into Dataset: Dimensions do not match.',
#        d.__setitem__,
#        (sc.Data.Value,
#         'data2'),
#        ([
#            'z',
#            'y',
#            'x'],
#            np.arange(24).reshape(
#            4,
#            2,
#            3)))
#
# def test_replace(self):
#    d = sc.Dataset()
#    d[sc.Data.Value, 'data1'] = (
#        ['z', 'y', 'x'], np.zeros(24).reshape(4, 3, 2))
#    d[sc.Data.Value, 'data1'] = (
#        ['z', 'y', 'x'], np.arange(24).reshape(4, 3, 2))
#    self.assertEqual(len(d), 1)
#    np.testing.assert_array_equal(
#        d[sc.Data.Value, 'data1'].numpy, self.reference_data1)
#
# def test_insert_Variable(self):
#    d = sc.Dataset()
#    d[sc.Data.Value, 'data1'] = (
#        ['z', 'y', 'x'], np.arange(24).reshape(4, 3, 2))
#
#    var = sc.Variable(dims=['x'], np.arange(2))
#    d[sc.Data.Value, 'data2'] = var
#    d[sc.Data.Variance, 'data2'] = var
#    self.assertEqual(len(d), 3)
#
# def test_insert_variable_slice(self):
#    d = sc.Dataset()
#    d[sc.Data.Value, 'data1'] = (
#        ['z', 'y', 'x'], np.arange(24).reshape(4, 3, 2))
#
#    d[sc.Data.Value, 'data2'] = d[sc.Data.Value, 'data1']
#    d[sc.Data.Variance, 'data2'] = d[sc.Data.Value, 'data1']
#    self.assertEqual(len(d), 3)
#
# This characterises existing broken behaviour. Will need to be fixed.
# def test_demo_int_to_float_issue(self):
#    # Demo bug
#    d = sc.Dataset()
#    # Variable containing int array data
#    d[sc.Data.Value, 'v1'] = (['x', 'y'], np.ndarray.tolist(
#        np.arange(0, 10).reshape(2, 5)))
#    # Correct behaviour should be int64
#    self.assertEqual(d[sc.Data.Value, 'v1'].numpy.dtype, 'float64')
#
#    # Demo working 1D
#    d = sc.Dataset()
#    d[sc.Data.Value, 'v2'] = (['x'], np.ndarray.tolist(
#        np.arange(0, 10)))  # Variable containing int array data
#    self.assertEqual(d[sc.Data.Value, 'v2'].numpy.dtype, 'int64')
#
# def test_set_data(self):
#    d = sc.Dataset()
#    d[sc.Data.Value, 'data1'] = (
#        ['z', 'y', 'x'], np.arange(24).reshape(4, 3, 2))
#    self.assertEqual(d[sc.Data.Value, 'data1'].numpy.dtype, np.int64)
#    d[sc.Data.Value, 'data1'] = np.arange(24).reshape(4, 3, 2)
#    self.assertEqual(d[sc.Data.Value, 'data1'].numpy.dtype, np.int64)
#    # For existing items we do *not* change the dtype, but convert.
#    d[sc.Data.Value, 'data1'] = np.arange(24.0).reshape(4, 3, 2)
#    self.assertEqual(d[sc.Data.Value, 'data1'].numpy.dtype, np.int64)
#
# def test_nested_0D_empty_item(self):
#    d = sc.Dataset()
#    d[sc.Data.Events] = ([], sc.Dataset())
#    self.assertEqual(d[sc.Data.Events].data[0], sc.Dataset())
#
# def test_set_data_nested(self):
#    d = sc.Dataset()
#    table = sc.Dataset()
#    table[sc.Data.Value, 'col1'] = (['row'], [3.0, 2.0, 1.0, 0.0])
#    table[sc.Data.Value, 'col2'] = (['row'], np.arange(4.0))
#    d[sc.Data.Value, 'data1'] = (['x'], [table, table])
#    d[sc.Data.Value, 'data1'].data[1][sc.Data.Value, 'col1'].data[0] = 0.0
#    self.assertEqual(d[sc.Data.Value, 'data1'].data[0], table)
#    self.assertNotEqual(d[sc.Data.Value, 'data1'].data[1], table)
#    self.assertNotEqual(d[sc.Data.Value, 'data1'].data[0],
#                        d[sc.Data.Value, 'data1'].data[1])
#
# def test_dimensions(self):
#    self.assertEqual(self.dataset.dimensions['x'], 2)
#    self.assertEqual(self.dataset.dimensions['y'], 3)
#    self.assertEqual(self.dataset.dimensions['z'], 4)
#
# def test_data(self):
#    self.assertEqual(len(self.dataset[sc.Coord.X].data), 2)
#    self.assertSequenceEqual(self.dataset[sc.Coord.X].data, [0, 1])
#    # `data` property provides a flat view
#    self.assertEqual(len(self.dataset[sc.Data.Value, 'data1'].data), 24)
#    self.assertSequenceEqual(
#        self.dataset[sc.Data.Value, 'data1'].data, range(24))
#
# def test_numpy_data(self):
#    np.testing.assert_array_equal(
#        self.dataset[sc.Coord.X].numpy, self.reference_x)
#    np.testing.assert_array_equal(
#        self.dataset[sc.Coord.Y].numpy, self.reference_y)
#    np.testing.assert_array_equal(
#        self.dataset[sc.Coord.Z].numpy, self.reference_z)
#    np.testing.assert_array_equal(
#        self.dataset[sc.Data.Value, 'data1'].numpy, self.reference_data1)
#    np.testing.assert_array_equal(
#        self.dataset[sc.Data.Value, 'data2'].numpy, self.reference_data2)
#    np.testing.assert_array_equal(
#        self.dataset[sc.Data.Value, 'data3'].numpy, self.reference_data3)
#
# def test_view_subdata(self):
#    view = self.dataset.subset['data1']
#    # TODO Need consistent dimensions() implementation for Dataset and its
#    # views.
#    self.assertEqual(view.dimensions['x'], 2)
#    self.assertEqual(view.dimensions['y'], 3)
#    self.assertEqual(view.dimensions['z'], 4)
#    self.assertEqual(len(view), 4)
#
# def test_insert_subdata(self):
#    d1 = sc.Dataset()
#    d1[sc.Data.Value, 'a'] = (['x'], np.arange(10, dtype='double'))
#    d1[sc.Data.Variance, 'a'] = (['x'], np.arange(10, dtype='double'))
#    ds_slice = d1.subset['a']
#
#    d2 = sc.Dataset()
#    # Insert from subset
#    d2.subset['a'] = ds_slice
#    self.assertEqual(len(d1), len(d2))
#    self.assertEqual(d1, d2)
#
#    d3 = sc.Dataset()
#    # Insert from subset
#    d3.subset['b'] = ds_slice
#    self.assertEqual(len(d3), 2)
#    self.assertNotEqual(d1, d3)  # imported names should differ
#
#    d4 = sc.Dataset()
#    d4.subset['2a'] = ds_slice + ds_slice
#    self.assertEqual(len(d4), 2)
#    self.assertNotEqual(d1, d4)
#    self.assertTrue(np.array_equal(
#        d4[sc.Data.Value, '2a'].numpy,
#        ds_slice[sc.Data.Value, 'a'].numpy * 2))
#
# def test_insert_subdata_different_variable_types(self):
#    a = sc.Dataset()
#    xcoord = sc.Variable(dims=['x'], np.arange(4))
#    a[sc.Data.Value] = (['x'], np.arange(3))
#    a[sc.Coord.X] = xcoord
#    a[sc.Attr.ExperimentLog] = ([], sc.Dataset())
#
#    b = sc.Dataset()
#    with self.assertRaises(RuntimeError):
#        b.subset['b'] = a['x', :]  # Coordinates dont match
#    b[sc.Coord.X] = xcoord
#    b.subset['b'] = a['x', :]  # Should now work
#    self.assertEqual(len(a), len(b))
#    self.assertTrue((sc.Attr.ExperimentLog, 'b') in b)
#
# def test_slice_dataset(self):
#    for x in range(2):
#        view = self.dataset['x', x]
#        self.assertRaisesRegex(
#            RuntimeError,
#            'could not find variable with tag Coord::X and name ``.',
#            view.__getitem__,
#            sc.Coord.X)
#        np.testing.assert_array_equal(
#            view[sc.Coord.Y].numpy, self.reference_y)
#        np.testing.assert_array_equal(
#            view[sc.Coord.Z].numpy, self.reference_z)
#        np.testing.assert_array_equal(
#            view[sc.Data.Value, 'data1'].numpy,
#            self.reference_data1[:, :, x])
#        np.testing.assert_array_equal(
#            view[sc.Data.Value, 'data2'].numpy,
#            self.reference_data2[:, :, x])
#        np.testing.assert_array_equal(
#            view[sc.Data.Value, 'data3'].numpy,
#            self.reference_data3[:, x])
#    for y in range(3):
#        view = self.dataset['y', y]
#        np.testing.assert_array_equal(
#            view[sc.Coord.X].numpy, self.reference_x)
#        self.assertRaisesRegex(
#            RuntimeError,
#            'could not find variable with tag Coord::Y and name ``.',
#            view.__getitem__,
#            sc.Coord.Y)
#        np.testing.assert_array_equal(
#            view[sc.Coord.Z].numpy, self.reference_z)
#        np.testing.assert_array_equal(
#            view[sc.Data.Value, 'data1'].numpy,
#            self.reference_data1[:, y, :])
#        np.testing.assert_array_equal(
#            view[sc.Data.Value, 'data2'].numpy,
#            self.reference_data2[:, y, :])
#        np.testing.assert_array_equal(
#            view[sc.Data.Value, 'data3'].numpy,
#            self.reference_data3)
#    for z in range(4):
#        view = self.dataset['z', z]
#        np.testing.assert_array_equal(
#            view[sc.Coord.X].numpy, self.reference_x)
#        np.testing.assert_array_equal(
#            view[sc.Coord.Y].numpy, self.reference_y)
#        self.assertRaisesRegex(
#            RuntimeError,
#            'could not find variable with tag Coord::Z and name ``.',
#            view.__getitem__,
#            sc.Coord.Z)
#        np.testing.assert_array_equal(
#            view[sc.Data.Value, 'data1'].numpy,
#            self.reference_data1[z, :, :])
#        np.testing.assert_array_equal(
#            view[sc.Data.Value, 'data2'].numpy,
#            self.reference_data2[z, :, :])
#        np.testing.assert_array_equal(
#            view[sc.Data.Value, 'data3'].numpy, self.reference_data3[z, :])
#    for x in range(2):
#        for delta in range(3 - x):
#            view = self.dataset['x', x:x + delta]
#            np.testing.assert_array_equal(
#                view[sc.Coord.X].numpy, self.reference_x[x:x + delta])
#            np.testing.assert_array_equal(
#                view[sc.Coord.Y].numpy, self.reference_y)
#            np.testing.assert_array_equal(
#                view[sc.Coord.Z].numpy, self.reference_z)
#            np.testing.assert_array_equal(
#                view[sc.Data.Value, 'data1'].numpy,
#                self.reference_data1[:, :, x:x + delta])
#            np.testing.assert_array_equal(
#                view[sc.Data.Value, 'data2'].numpy,
#                self.reference_data2[:, :, x:x + delta])
#            np.testing.assert_array_equal(
#                view[sc.Data.Value, 'data3'].numpy,
#                self.reference_data3[:, x:x + delta])
#    for y in range(3):
#        for delta in range(4 - y):
#            view = self.dataset['y', y:y + delta]
#            np.testing.assert_array_equal(
#                view[sc.Coord.X].numpy, self.reference_x)
#            np.testing.assert_array_equal(
#                view[sc.Coord.Y].numpy, self.reference_y[y:y + delta])
#            np.testing.assert_array_equal(
#                view[sc.Coord.Z].numpy, self.reference_z)
#            np.testing.assert_array_equal(
#                view[sc.Data.Value, 'data1'].numpy,
#                self.reference_data1[:, y:y + delta, :])
#            np.testing.assert_array_equal(
#                view[sc.Data.Value, 'data2'].numpy,
#                self.reference_data2[:, y:y + delta, :])
#            np.testing.assert_array_equal(
#                view[sc.Data.Value, 'data3'].numpy,
#                self.reference_data3)
#    for z in range(4):
#        for delta in range(5 - z):
#            view = self.dataset['z', z:z + delta]
#            np.testing.assert_array_equal(
#                view[sc.Coord.X].numpy, self.reference_x)
#            np.testing.assert_array_equal(
#                view[sc.Coord.Y].numpy, self.reference_y)
#            np.testing.assert_array_equal(
#                view[sc.Coord.Z].numpy, self.reference_z[z:z + delta])
#            np.testing.assert_array_equal(
#                view[sc.Data.Value, 'data1'].numpy,
#                self.reference_data1[z:z + delta, :, :])
#            np.testing.assert_array_equal(
#                view[sc.Data.Value, 'data2'].numpy,
#                self.reference_data2[z:z + delta, :, :])
#            np.testing.assert_array_equal(
#                view[sc.Data.Value, 'data3'].numpy,
#                self.reference_data3[z:z + delta, :])
#
# def _apply_test_op_rhs_dataset(
#    self,
#    op,
#    a,
#    b,
#    data,
#    lh_var_name='i',
#        rh_var_name='j'):
#    # Assume numpy operations are correct as comparitor
#    op(data, b[sc.Data.Value, rh_var_name].numpy)
#    op(a, b)
#    # Desired nan comparisons
#    np.testing.assert_equal(a[sc.Data.Value, lh_var_name].numpy, data)
#
# def _apply_test_op_rhs_Variable(
#    self,
#    op,
#    a,
#    b,
#    data,
#    lh_var_name='i',
#        rh_var_name='j'):
#    # Assume numpy operations are correct as comparitor
#    op(data, b.numpy)
#    op(a, b)
#    # Desired nan comparisons
#    np.testing.assert_equal(a[sc.Data.Value, lh_var_name].numpy, data)
#
#
# def test_equal_not_equal(self):
#    a = sc.Dataset()
#    a[sc.Coord.X] = (['x'], np.arange(10))
#    a[sc.Data.Value, 'i'] = (['x'], np.arange(10, dtype='float64'))
#    b = sc.Dataset()
#    b[sc.Data.Value, 'j'] = (['x'], np.arange(10, dtype='float64'))
#    c = a + b
#    d = sc.Dataset()
#    d[sc.Coord.X] = (['x'], np.arange(10))
#    d[sc.Data.Value, 'i'] = (['x'], np.arange(10, dtype='float64'))
#    a_slice = a['x', :]
#    d_slice = d['x', :]
#    # Equal
#    self.assertEqual(a, d)
#    self.assertEqual(a, a_slice)
#    self.assertEqual(a_slice, d_slice)
#    self.assertEqual(d, a)
#    self.assertEqual(d_slice, a)
#    self.assertEqual(d_slice, a_slice)
#    # Not equal
#    self.assertNotEqual(a, b)
#    self.assertNotEqual(a, c)
#    self.assertNotEqual(a_slice, c)
#    self.assertNotEqual(c, a)
#    self.assertNotEqual(c, a_slice)
#
#
#
# def test_concatenate(self):
#    dataset = sc.Dataset()
#    dataset[sc.Data.Value, 'data'] = (['x'], np.arange(4))
#    dataset[sc.Coord.X] = (['x'], np.array([3, 2, 4, 1]))
#    dataset = sc.concatenate(dataset, dataset, 'x')
#    np.testing.assert_array_equal(
#        dataset[sc.Coord.X].numpy, np.array([3, 2, 4, 1, 3, 2, 4, 1]))
#    np.testing.assert_array_equal(
#        dataset[sc.Data.Value, 'data'].numpy,
#        np.array([0, 1, 2, 3, 0, 1, 2, 3]))
#
# def test_concatenate_with_slice(self):
#    dataset = sc.Dataset()
#    dataset[sc.Data.Value, 'data'] = (['x'], np.arange(4))
#    dataset[sc.Coord.X] = (['x'], np.array([3, 2, 4, 1]))
#    dataset = sc.concatenate(dataset, dataset['x', 0:2], 'x')
#    np.testing.assert_array_equal(
#        dataset[sc.Coord.X].numpy, np.array([3, 2, 4, 1, 3, 2]))
#    np.testing.assert_array_equal(
#        dataset[sc.Data.Value, 'data'].numpy, np.array([0, 1, 2, 3, 0, 1]))
#

def test_rebin():
    dataset = sc.Dataset()
    dataset['data'] = sc.Variable(['x'], values=np.array(10 * [1.0]), unit=sc.units.counts)
    dataset.coords['x'] = sc.Variable(['x'], values=np.arange(11.0))
    new_coord = sc.Variable(dims=['x'], values=np.arange(0.0, 11, 2))
    dataset = sc.rebin(dataset, 'x', new_coord)
    np.testing.assert_array_equal(
        dataset['data'].values, np.array(5 * [2]))
    np.testing.assert_array_equal(
        dataset.coords['x'].values, np.arange(0, 11, 2))

# def test_sort(self):
#    dataset = sc.Dataset()
#    dataset[sc.Data.Value, 'data'] = (['x'], np.arange(4))
#    dataset[sc.Data.Value, 'data2'] = (['x'], np.arange(4))
#    dataset[sc.Coord.X] = (['x'], np.array([3, 2, 4, 1]))
#    dataset = sc.sort(dataset, sc.Coord.X)
#    np.testing.assert_array_equal(
#        dataset[sc.Data.Value, 'data'].numpy, np.array([3, 1, 0, 2]))
#    np.testing.assert_array_equal(
#        dataset[sc.Coord.X].numpy, np.array([1, 2, 3, 4]))
#    dataset = sc.sort(dataset, sc.Data.Value, 'data')
#    np.testing.assert_array_equal(
#        dataset[sc.Data.Value, 'data'].numpy, np.arange(4))
#    np.testing.assert_array_equal(
#        dataset[sc.Coord.X].numpy, np.array([3, 2, 4, 1]))
#
# def test_filter(self):
#    dataset = sc.Dataset()
#    dataset[sc.Data.Value, 'data'] = (['x'], np.arange(4))
#    dataset[sc.Coord.X] = (['x'], np.array([3, 2, 4, 1]))
#    select = sc.Variable(dims=['x'], np.array([False, True, False, True]))
#    dataset = sc.filter(dataset, select)
#    np.testing.assert_array_equal(
#        dataset[sc.Data.Value, 'data'].numpy, np.array([1, 3]))
#    np.testing.assert_array_equal(dataset[sc.Coord.X].numpy,
#                                  np.array([2, 1]))
#
# @unittest.skip('Tag-derived dtype not available anymore, need to implement \
#               way of specifying list type for events.')
# def test_zip(self):
#    d = sc.Dataset()
#    d[sc.Coord.SpectrumNumber] = ([sc.Dim.Position], np.arange(1, 6))
#    d[sc.Data.EventTofs, ''] = ([sc.Dim.Position], (5,))
#    d[sc.Data.EventPulseTimes, ''] = ([sc.Dim.Position], (5,))
#    self.assertEqual(len(d[sc.Data.EventTofs, ''].data), 5)
#    d[sc.Data.EventTofs, ''].data[0].append(10)
#    d[sc.Data.EventPulseTimes, ''].data[0].append(1000)
#    d[sc.Data.EventTofs, ''].data[1].append(10)
#    d[sc.Data.EventPulseTimes, ''].data[1].append(1000)
#    # Don't do this, there are no compatiblity checks:
#    # for el in zip(d[sc.Data.EventTofs, ''].data,
#    #     d[sc.Data.EventPulseTimes, ''].data):
#    for el, size in zip(d.zip(), [1, 1, 0, 0, 0]):
#        self.assertEqual(len(el), size)
#        for e in el:
#            self.assertEqual(e.first(), 10)
#            self.assertEqual(e.second(), 1000)
#        el.append((10, 300))
#        self.assertEqual(len(el), size + 1)
#
# def test_np_array_strides(self):
#    N = 6
#    M = 4
#    d1 = sc.Dataset()
#    d1[sc.Coord.X] = (['x'], np.arange(N + 1).astype(np.float64))
#    d1[sc.Coord.Y] = (['y'], np.arange(M + 1).astype(np.float64))
#
#    arr1 = np.arange(N * M).reshape(N, M).astype(np.float64)
#    arr2 = np.transpose(arr1)
#    K = 3
#    arr_buf = np.arange(N * K * M).reshape(N, K, M)
#    arr3 = arr_buf[:, 1, :]
#    d1[sc.Data.Value, 'A'] = (['x', 'y'], arr1)
#    d1[sc.Data.Value, 'B'] = (['y', 'x'], arr2)
#    d1[sc.Data.Value, 'C'] = (['x', 'y'], arr3)
#    np.testing.assert_array_equal(arr1, d1[sc.Data.Value, 'A'].numpy)
#    np.testing.assert_array_equal(arr2, d1[sc.Data.Value, 'B'].numpy)
#    np.testing.assert_array_equal(arr3, d1[sc.Data.Value, 'C'].numpy)
#
# def test_rebin(self):
#    N = 6
#    M = 4
#    d1 = sc.Dataset()
#    d1[sc.Coord.X] = (['x'], np.arange(N + 1).astype(np.float64))
#    d1[sc.Coord.Y] = (['y'], np.arange(M + 1).astype(np.float64))
#
#    arr1 = np.arange(N * M).reshape(N, M).astype(np.float64)
#    # TODO copy not needed after correct treatment of strides
#    arr2 = np.transpose(arr1).copy()
#
#    d1[sc.Data.Value, 'A'] = (['x', 'y'], arr1)
#    d1[sc.Data.Value, 'B'] = (['y', 'x'], arr2)
#    d1[sc.Data.Value, 'A'].unit = sc.units.counts
#    d1[sc.Data.Value, 'B'].unit = sc.units.counts
#    rd1 = sc.rebin(d1, sc.Variable(dims=['x'], np.arange(
#        0, N + 1, 1.5).astype(np.float64)))
#    np.testing.assert_array_equal(rd1[sc.Data.Value, 'A'].numpy,
#                                  np.transpose(
#                                      rd1[sc.Data.Value, 'B'].numpy))
#
def test_copy():
    import copy
    a = sc.Dataset()
    a['x'] = sc.Variable(value=1)
    assert copy.copy(a) == a
    assert copy.deepcopy(a) == a

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
