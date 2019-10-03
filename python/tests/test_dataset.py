# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import pytest

import numpy as np
import scipp as sc
from scipp import Dim


def test_create_empty():
    d = sc.Dataset()
    assert len(d) == 0


def test_create():
    x = sc.Variable([Dim.X], values=np.arange(3))
    y = sc.Variable([Dim.Y], values=np.arange(4))
    xy = sc.Variable([Dim.X, Dim.Y], values=np.arange(12).reshape(3, 4))
    d = sc.Dataset({'xy': xy, 'x': x}, coords={Dim.X: x, Dim.Y: y})
    assert len(d) == 2
    assert d.coords[Dim.X] == x
    assert d.coords[Dim.Y] == y
    assert d['xy'].data == xy
    assert d['x'].data == x


def test_create_from_data_array():
    var = sc.Variable([Dim.X], values=np.arange(4))
    base = sc.Dataset({'a': var}, coords={Dim.X: var}, labels={'aux': var})
    d = sc.Dataset(base['a'])
    assert d == base


def test_create_from_data_arrays():
    var1 = sc.Variable([Dim.X], values=np.arange(4))
    var2 = sc.Variable([Dim.X], values=np.ones(4))
    base = sc.Dataset({
        'a': var1,
        'b': var2
    },
                      coords={Dim.X: var1},
                      labels={'aux': var1})
    d = sc.Dataset({'a': base['a'], 'b': base['b']})
    assert d == base
    swapped = sc.Dataset({'a': base['b'], 'b': base['a']})
    assert swapped != base
    assert swapped['a'] == base['b']
    assert swapped['b'] == base['a']


def test_clear():
    d = sc.Dataset()
    d['a'] = sc.Variable([Dim.X], values=np.arange(3))
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
    var = sc.Variable([Dim.X], values=np.arange(4))
    d = sc.Dataset({'a': var}, coords={Dim.X: var})
    with pytest.raises(RuntimeError):
        d[Dim.X, 2:3].coords[Dim.Y] = sc.Variable(1.0)
    d.coords[Dim.Y] = sc.Variable(1.0)
    assert len(d) == 1
    assert len(d.coords) == 2
    assert d.coords[Dim.Y] == sc.Variable(1.0)


def test_coord_setitem_sparse():
    var = sc.Variable([Dim.X], values=np.arange(4))
    sparse = sc.Variable([sc.Dim.X], [sc.Dimensions.Sparse])
    d = sc.Dataset({'a': sparse}, coords={Dim.X: var})
    with pytest.raises(RuntimeError):
        d.coords[Dim.X] = sparse
    with pytest.raises(RuntimeError):
        d[Dim.X, 2:3].coords[Dim.X] = sparse
    # This is a slightly weird case with sparse data before coord is set.
    # Currently there is no way to set a sparse coord from a sparse variable
    # in this way, since otherwise 'a' would not exist in d.
    d['a'].coords[Dim.X] = sparse
    assert len(d.coords) == 1
    assert len(d['a'].coords) == 1
    assert d['a'].coords[Dim.X] == sparse
    assert d['a'].coords[Dim.X] != var
    assert d['a'].coords[Dim.X] != d.coords[Dim.X]
    # This would not work:
    with pytest.raises(IndexError):
        d['b'].coords[Dim.X] = sparse


def test_create_sparse_via_DataArray():
    var = sc.Variable([Dim.X], values=np.arange(4))
    d = sc.Dataset(coords={Dim.X: var})
    sparse = sc.Variable([sc.Dim.X], [sc.Dimensions.Sparse])
    d['a'] = sc.DataArray(coords={Dim.X: sparse})
    assert len(d.coords) == 1
    assert len(d['a'].coords) == 1
    assert d['a'].coords[Dim.X] == sparse
    assert d['a'].coords[Dim.X] != var
    assert d['a'].coords[Dim.X] != d.coords[Dim.X]


def test_contains_coord():
    d = sc.Dataset()
    assert Dim.X not in d.coords
    d.coords[Dim.X] = sc.Variable(1.0)
    assert Dim.X in d.coords


def test_labels_setitem():
    var = sc.Variable([Dim.X], values=np.arange(4))
    d = sc.Dataset({'a': var}, coords={Dim.X: var})
    with pytest.raises(RuntimeError):
        d[Dim.X, 2:3].labels['label'] = sc.Variable(1.0)
    d.labels['label'] = sc.Variable(1.0)
    assert len(d) == 1
    assert len(d.labels) == 1
    assert d.labels['label'] == sc.Variable(1.0)


def test_labels_setitem_sparse():
    var = sc.Variable([Dim.X], values=np.arange(4))
    sparse = sc.Variable([sc.Dim.X], [sc.Dimensions.Sparse])
    d = sc.Dataset({'a': sparse}, coords={Dim.X: var})
    with pytest.raises(RuntimeError):
        d.labels['label'] = sparse
    with pytest.raises(RuntimeError):
        d[Dim.X, 2:3].labels['label'] = sparse
    d['a'].labels['label'] = sparse


def test_contains_labels():
    d = sc.Dataset()
    assert "a" not in d.labels
    d.labels["a"] = sc.Variable(1.0)
    assert "a" in d.labels


def test_attrs_setitem():
    var = sc.Variable([Dim.X], values=np.arange(4))
    d = sc.Dataset({'a': var}, coords={Dim.X: var})
    with pytest.raises(RuntimeError):
        d[Dim.X, 2:3].attrs['attr'] = sc.Variable(1.0)
    d.attrs['attr'] = sc.Variable(1.0)
    assert len(d) == 1
    assert len(d.attrs) == 1
    assert d.attrs['attr'] == sc.Variable(1.0)


def test_attrs_setitem_sparse():
    var = sc.Variable([Dim.X], values=np.arange(4))
    sparse = sc.Variable([sc.Dim.X], [sc.Dimensions.Sparse])
    d = sc.Dataset({'a': sparse}, coords={Dim.X: var})
    with pytest.raises(RuntimeError):
        d.attrs['attr'] = sparse
    with pytest.raises(RuntimeError):
        d[Dim.X, 2:3].attrs['attr'] = sparse
    # Attributes cannot be sparse
    with pytest.raises(RuntimeError):
        d['a'].attrs['attr'] = sparse


def test_contains_attrs():
    d = sc.Dataset()
    assert "b" not in d.attrs
    d.attrs["b"] = sc.Variable(1.0)
    assert "b" in d.attrs


def test_slice_item():
    d = sc.Dataset(
        coords={Dim.X: sc.Variable([Dim.X], values=np.arange(4, 8))})
    d['a'] = sc.Variable([Dim.X], values=np.arange(4))
    assert d['a'][Dim.X, 2:4].data == sc.Variable([Dim.X],
                                                  values=np.arange(2, 4))
    assert d['a'][Dim.X,
                  2:4].coords[Dim.X] == sc.Variable([Dim.X],
                                                    values=np.arange(6, 8))


def test_set_item_slice_from_numpy():
    d = sc.Dataset(
        coords={Dim.X: sc.Variable([Dim.X], values=np.arange(4, 8))})
    d['a'] = sc.Variable([Dim.X], values=np.arange(4))
    d['a'][Dim.X, 2:4] = np.arange(2)
    assert d['a'].data == sc.Variable([Dim.X], values=np.array([0, 1, 0, 1]))


def test_set_item_slice_with_variances_from_numpy():
    d = sc.Dataset(
        coords={Dim.X: sc.Variable([Dim.X], values=np.arange(4, 8))})
    d['a'] = sc.Variable([Dim.X], values=np.arange(4), variances=np.arange(4))
    d['a'][Dim.X, 2:4].values = np.arange(2)
    d['a'][Dim.X, 2:4].variances = np.arange(2, 4)
    assert np.array_equal(d['a'].values, np.array([0.0, 1.0, 0.0, 1.0]))
    assert np.array_equal(d['a'].variances, np.array([0.0, 1.0, 2.0, 3.0]))


def test_sparse_setitem():
    d = sc.Dataset(
        {'a': sc.Variable([sc.Dim.X, sc.Dim.Y], [4, sc.Dimensions.Sparse])})
    d['a'][Dim.X, 0].values = np.arange(4)
    assert len(d['a'][Dim.X, 0].values) == 4


def test_iadd_slice():
    d = sc.Dataset(
        coords={Dim.X: sc.Variable([Dim.X], values=np.arange(4, 8))})
    d['a'] = sc.Variable([Dim.X], values=np.arange(4))
    d['a'][Dim.X, 1] += d['a'][Dim.X, 2]
    assert d['a'].data == sc.Variable([Dim.X], values=np.array([0, 3, 2, 3]))


def test_iadd_range():
    d = sc.Dataset(
        coords={Dim.X: sc.Variable([Dim.X], values=np.arange(4, 8))})
    d['a'] = sc.Variable([Dim.X], values=np.arange(4))
    with pytest.raises(RuntimeError):
        d['a'][Dim.X, 2:4] += d['a'][Dim.X, 1:3]
    d['a'][Dim.X, 2:4] += d['a'][Dim.X, 2:4]
    assert d['a'].data == sc.Variable([Dim.X], values=np.array([0, 1, 4, 6]))


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
            'a': sc.Variable([Dim.X], values=np.arange(10.0)),
            'b': sc.Variable(1.0)
        },
        coords={Dim.X: sc.Variable([Dim.X], values=np.arange(10.0))})
    expected = sc.Dataset({'a': sc.Variable(1.0)})

    assert d[Dim.X, 1] == expected
    assert 'a' in d[Dim.X, 1]
    assert 'b' not in d[Dim.X, 1]


def test_chained_slicing():
    d = sc.Dataset(
        {
            'a':
            sc.Variable([Dim.Z, Dim.Y, Dim.X],
                        values=np.arange(1000.0).reshape(10, 10, 10)),
            'b':
            sc.Variable([Dim.X, Dim.Z],
                        values=np.arange(0.0, 10.0, 0.1).reshape(10, 10))
        },
        coords={
            Dim.X: sc.Variable([Dim.X], values=np.arange(11.0)),
            Dim.Y: sc.Variable([Dim.Y], values=np.arange(11.0)),
            Dim.Z: sc.Variable([Dim.Z], values=np.arange(11.0))
        })

    expected = sc.Dataset()
    expected.coords[Dim.Y] = sc.Variable([Dim.Y], values=np.arange(11.0))
    expected["a"] = sc.Variable([Dim.Y], values=np.arange(501.0, 600.0, 10.0))
    expected["b"] = sc.Variable(1.5)

    assert d[Dim.X, 1][Dim.Z, 5] == expected


def test_coords_proxy_comparison_operators():
    d = sc.Dataset(
        {
            'a': sc.Variable([Dim.X], values=np.arange(10.0)),
            'b': sc.Variable(1.0)
        },
        coords={Dim.X: sc.Variable([Dim.X], values=np.arange(10.0))})

    d1 = sc.Dataset(
        {
            'a': sc.Variable([Dim.X], values=np.arange(10.0)),
            'b': sc.Variable(1.0)
        },
        coords={Dim.X: sc.Variable([Dim.X], values=np.arange(10.0))})
    assert d1['a'].coords == d['a'].coords


def test_sum_mean():
    d = sc.Dataset(
        {
            'a': sc.Variable([Dim.X, Dim.Y], values=np.arange(6).reshape(2,
                                                                         3)),
            'b': sc.Variable([Dim.Y], values=np.arange(3))
        },
        coords={
            Dim.X: sc.Variable([Dim.X], values=np.arange(2)),
            Dim.Y: sc.Variable([Dim.Y], values=np.arange(3))
        },
        labels={
            "l1": sc.Variable([Dim.X, Dim.Y],
                              values=np.arange(6).reshape(2, 3)),
            "l2": sc.Variable([Dim.X], values=np.arange(2))
        })
    d_ref = sc.Dataset(
        {
            'a': sc.Variable([Dim.X], values=np.array([3, 12])),
            'b': sc.Variable(3)
        },
        coords={Dim.X: sc.Variable([Dim.X], values=np.arange(2))},
        labels={"l2": sc.Variable([Dim.X], values=np.arange(2))})

    assert sc.sum(d, Dim.Y) == d_ref
    assert (sc.mean(d, Dim.Y)["a"].values == [1.0, 4.0]).all()
    assert sc.mean(d, Dim.Y)["b"].value == 1.0


def test_variable_histogram():
    var = sc.Variable(dims=[Dim.X, Dim.Y], shape=[2, sc.Dimensions.Sparse])
    var[Dim.X, 0].values = np.arange(3)
    var[Dim.X, 0].values.append(42)
    var[Dim.X, 0].values.extend(np.ones(3))
    var[Dim.X, 1].values = np.ones(6)
    ds = sc.Dataset()
    ds["sparse"] = sc.DataArray(coords={Dim.Y: var})
    hist = sc.histogram(
        ds["sparse"],
        sc.Variable(values=np.arange(5, dtype=np.float64), dims=[Dim.Y]))
    assert np.array_equal(
        hist.values, np.array([[1.0, 4.0, 1.0, 0.0], [0.0, 6.0, 0.0, 0.0]]))


def test_dataset_histogram():
    var = sc.Variable(dims=[Dim.X, Dim.Y], shape=[2, sc.Dimensions.Sparse])
    var[Dim.X, 0].values = np.arange(3)
    var[Dim.X, 0].values.append(42)
    var[Dim.X, 0].values.extend(np.ones(3))
    var[Dim.X, 1].values = np.ones(6)
    ds = sc.Dataset()
    ds["s"] = sc.DataArray(coords={Dim.Y: var})
    ds["s1"] = sc.DataArray(coords={Dim.Y: var * 5})
    h = sc.histogram(
        ds, sc.Variable(values=np.arange(5, dtype=np.float64), dims=[Dim.Y]))
    assert np.array_equal(
        h["s"].values, np.array([[1.0, 4.0, 1.0, 0.0], [0.0, 6.0, 0.0, 0.0]]))
    assert np.array_equal(
        h["s1"].values, np.array([[1.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0]]))


def test_histogram_and_setitem():
    var = sc.Variable(dims=[Dim.X, Dim.Tof],
                      shape=[2, sc.Dimensions.Sparse],
                      unit=sc.units.us)
    var[Dim.X, 0].values = np.arange(3)
    var[Dim.X, 0].values.append(42)
    var[Dim.X, 0].values.extend(np.ones(3))
    var[Dim.X, 1].values = np.ones(6)
    ds = sc.Dataset()
    ds["s"] = sc.DataArray(coords={Dim.Tof: var})
    assert Dim.Tof not in ds.coords
    assert Dim.Tof in ds["s"].coords
    edges = sc.Variable([sc.Dim.Tof], values=np.arange(5.0), unit=sc.units.us)
    ds["h"] = sc.histogram(ds["s"], edges)
    assert np.array_equal(
        ds["h"].values, np.array([[1.0, 4.0, 1.0, 0.0], [0.0, 6.0, 0.0, 0.0]]))
    assert Dim.Tof in ds.coords


def test_dataset_merge():
    a = sc.Dataset({'d1': sc.Variable([Dim.X], values=np.array([1, 2, 3]))})
    b = sc.Dataset({'d2': sc.Variable([Dim.X], values=np.array([4, 5, 6]))})
    c = sc.merge(a, b)
    assert a['d1'] == c['d1']
    assert b['d2'] == c['d2']


def test_dataset_concatenate():
    a = sc.Dataset(
        {'data': sc.Variable([Dim.X], values=np.array([11, 12]))},
        coords={Dim.X: sc.Variable([Dim.X], values=np.array([1, 2]))})
    b = sc.Dataset(
        {'data': sc.Variable([Dim.X], values=np.array([13, 14]))},
        coords={Dim.X: sc.Variable([Dim.X], values=np.array([3, 4]))})

    c = sc.concatenate(a, b, Dim.X)

    assert np.array_equal(c.coords[Dim.X].values, np.array([1, 2, 3, 4]))
    assert np.array_equal(c["data"].values, np.array([11, 12, 13, 14]))


def test_dataset_set_data():
    d1 = sc.Dataset(
        {
            'a': sc.Variable(dims=[Dim.X, Dim.Y], values=np.random.rand(2, 3)),
            'b': sc.Variable(1.0)
        },
        coords={
            Dim.X: sc.Variable([Dim.X], values=np.arange(2.0),
                               unit=sc.units.m),
            Dim.Y: sc.Variable([Dim.Y], values=np.arange(3.0), unit=sc.units.m)
        },
        labels={'aux': sc.Variable([Dim.Y], values=np.arange(3))})

    d2 = sc.Dataset(
        {
            'a': sc.Variable(dims=[Dim.X, Dim.Y], values=np.random.rand(2, 3)),
            'b': sc.Variable(1.0)
        },
        coords={
            Dim.X: sc.Variable([Dim.X], values=np.arange(2.0),
                               unit=sc.units.m),
            Dim.Y: sc.Variable([Dim.Y], values=np.arange(3.0), unit=sc.units.m)
        },
        labels={'aux': sc.Variable([Dim.Y], values=np.arange(3))})

    d3 = sc.Dataset()
    d3['b'] = d1['a']
    assert d3["b"].data == d1['a'].data
    assert d3["b"].coords == d1['a'].coords
    d1['a'] = d2['a']
    d1['c'] = d2['a']
    assert d2['a'].data == d1['a'].data
    assert d2['a'].data == d1['c'].data

    d = sc.Dataset()
    d.coords[Dim.Row] = sc.Variable([sc.Dim.Row], values=np.arange(10.0))
    d["a"] = sc.Variable([sc.Dim.Row],
                         values=np.arange(10.0),
                         variances=np.arange(10.0))
    d["b"] = sc.Variable([sc.Dim.Row], values=np.arange(10.0, 20.0))
    d1 = d[sc.Dim.Row, 0:1]
    d2 = sc.Dataset({'a': d1["a"].data},
                    coords={sc.Dim.Row: d1["a"].coords[sc.Dim.Row]})
    d2["b"] = d1["b"]
    expected = sc.Dataset()
    expected.coords[Dim.Row] = sc.Variable([sc.Dim.Row], values=np.arange(1.0))
    expected["a"] = sc.Variable([sc.Dim.Row],
                                values=np.arange(1.0),
                                variances=np.arange(1.0))
    expected["b"] = sc.Variable([sc.Dim.Row], values=np.arange(10.0, 11.0))
    assert d2 == expected


def test_dataset_data_access():
    var = sc.Variable(dims=[Dim.X, Dim.Y], shape=[2, sc.Dimensions.Sparse])
    ds = sc.Dataset()
    ds["sparse"] = sc.DataArray(coords={Dim.Y: var})
    assert ds["sparse"].values is None


def test_binary_with_broadcast():
    d = sc.Dataset(
        {'data': sc.Variable([Dim.X], values=np.arange(10.0))},
        coords={Dim.X: sc.Variable([Dim.X], values=np.arange(10.0))})

    d2 = d - d[Dim.X, 0]
    d -= d[Dim.X, 0]
    assert d == d2


def test_binary_of_item_with_variable():
    d = sc.Dataset(
        {'data': sc.Variable([Dim.X], values=np.arange(10.0))},
        coords={Dim.X: sc.Variable([Dim.X], values=np.arange(10.0))})
    copy = d.copy()

    d['data'] += 2.0 * sc.units.dimensionless
    d['data'] *= 2.0 * sc.units.m
    d['data'] -= 4.0 * sc.units.m
    d['data'] /= 2.0 * sc.units.m
    assert d == copy


def test_in_place_binary_with_scalar():
    d = sc.Dataset(
        {'data': sc.Variable([Dim.X], values=[10])},
        coords={Dim.X: sc.Variable([Dim.X], values=[10])})
    copy = d.copy()

    d += 2
    d *= 2
    d -= 4
    d /= 2
    assert d == copy


def test_proxy_in_place_binary_with_scalar():
    d = sc.Dataset(
        {'data': sc.Variable([Dim.X], values=[10])},
        coords={Dim.X: sc.Variable([Dim.X], values=[10])})
    copy = d.copy()

    d['data'] += 2
    d['data'] *= 2
    d['data'] -= 4
    d['data'] /= 2
    assert d == copy


def test_add_sum_of_columns():
    d = sc.Dataset({
        'a': sc.Variable([Dim.X], values=np.arange(10.0)),
        'b': sc.Variable([Dim.X], values=np.ones(10))
    })
    d['sum'] = d['a'] + d['b']
    d['a'] += d['b']
    assert d['sum'] == d['a']


def test_name():
    d = sc.Dataset(
        {
            'a': sc.Variable([Dim.X], values=np.arange(10.0)),
            'b': sc.Variable([Dim.X], values=np.ones(10))
        },
        coords={Dim.X: sc.Variable([Dim.X], values=np.arange(10))})
    assert d['a'].name == 'a'
    assert d['b'].name == 'b'
    assert d[Dim.X, 2]['b'].name == 'b'
    assert d['b'][Dim.X, 2].name == 'b'
    array = d['a'] + d['b']
    assert array.name == ''


def make_simple_dataset(dim1=Dim.X, dim2=Dim.Y, seed=None):
    if seed is not None:
        np.random.seed(seed)
    return sc.Dataset(
        {
            'a': sc.Variable(dims=[dim1, dim2], values=np.random.rand(2, 3)),
            'b': sc.Variable(1.0)
        },
        coords={
            dim1: sc.Variable([dim1], values=np.arange(2.0),
                               unit=sc.units.m),
            dim2: sc.Variable([dim2], values=np.arange(3.0), unit=sc.units.m)
        },
        labels={'aux': sc.Variable([dim2], values=np.random.rand(3))})


def test_dataset_proxy_set_variance():
    d = make_simple_dataset()
    variances = np.arange(6).reshape(2, 3)
    assert d["a"].variances is None
    d["a"].variances = variances
    assert d["a"].variances is not None
    np.testing.assert_array_equal(d["a"].variances, variances)


def test_sort():
    d = sc.Dataset(
        {
            'a':
            sc.Variable(dims=[Dim.X, Dim.Y], values=np.arange(6).reshape(2,
                                                                         3)),
            'b':
            sc.Variable(dims=[Dim.X], values=['b', 'a'])
        },
        coords={
            Dim.X: sc.Variable([Dim.X], values=np.arange(2.0),
                               unit=sc.units.m),
            Dim.Y: sc.Variable([Dim.Y], values=np.arange(3.0), unit=sc.units.m)
        },
        labels={'aux': sc.Variable([Dim.X], values=np.arange(2.0))})
    expected = sc.Dataset(
        {
            'a':
            sc.Variable(dims=[Dim.X, Dim.Y],
                        values=np.flip(np.arange(6).reshape(2, 3), axis=0)),
            'b':
            sc.Variable(dims=[Dim.X], values=['a', 'b'])
        },
        coords={
            Dim.X: sc.Variable([Dim.X], values=[1.0, 0.0], unit=sc.units.m),
            Dim.Y: sc.Variable([Dim.Y], values=np.arange(3.0), unit=sc.units.m)
        },
        labels={'aux': sc.Variable([Dim.X], values=[1.0, 0.0])})
    assert sc.sort(d, d['b'].data) == expected


def test_rename_dims():
    d = make_simple_dataset(Dim.X, Dim.Y, seed=0)
    d.rename_dims({Dim.Y: Dim.Z})
    assert d == make_simple_dataset(Dim.X, Dim.Z, seed=0)
    d.rename_dims(dims_dict={Dim.X: Dim.Y, Dim.Z: Dim.X})
    assert d == make_simple_dataset(Dim.Y, Dim.X, seed=0)


# def test_delitem(self):
#    dataset = sc.Dataset()
#    dataset[sc.Data.Value, "data"] = ([sc.Dim.Z, sc.Dim.Y, sc.Dim.X],
#                                      (1, 2, 3))
#    dataset[sc.Data.Value, "aux"] = ([], ())
#    self.assertTrue((sc.Data.Value, "data") in dataset)
#    self.assertEqual(len(dataset.dimensions), 3)
#    del dataset[sc.Data.Value, "data"]
#    self.assertFalse((sc.Data.Value, "data") in dataset)
#    self.assertEqual(len(dataset.dimensions), 0)
#
#    dataset[sc.Data.Value, "data"] = ([sc.Dim.Z, sc.Dim.Y, sc.Dim.X],
#                                      (1, 2, 3))
#    dataset[sc.Coord.X] = ([sc.Dim.X], np.arange(3))
#    del dataset[sc.Data.Value, "data"]
#    self.assertFalse((sc.Data.Value, "data") in dataset)
#    self.assertEqual(len(dataset.dimensions), 1)
#    del dataset[sc.Coord.X]
#    self.assertFalse(sc.Coord.X in dataset)
#    self.assertEqual(len(dataset.dimensions), 0)
#
# def test_insert_default_init(self):
#    d = sc.Dataset()
#    d[sc.Data.Value, "data1"] = ((sc.Dim.Z, sc.Dim.Y, sc.Dim.X), (4, 3, 2))
#    self.assertEqual(len(d), 1)
#    np.testing.assert_array_equal(
#        d[sc.Data.Value, "data1"].numpy, np.zeros(shape=(4, 3, 2)))
#
# def test_insert(self):
#    d = sc.Dataset()
#    d[sc.Data.Value, "data1"] = (
#        [sc.Dim.Z, sc.Dim.Y, sc.Dim.X], np.arange(24).reshape(4, 3, 2))
#    self.assertEqual(len(d), 1)
#    np.testing.assert_array_equal(
#        d[sc.Data.Value, "data1"].numpy, self.reference_data1)
#
#    self.assertRaisesRegex(
#        RuntimeError,
#        "Cannot insert variable into Dataset: Dimensions do not match.",
#        d.__setitem__,
#        (sc.Data.Value,
#         "data2"),
#        ([
#            sc.Dim.Z,
#            sc.Dim.Y,
#            sc.Dim.X],
#            np.arange(24).reshape(
#            4,
#            2,
#            3)))
#
# def test_replace(self):
#    d = sc.Dataset()
#    d[sc.Data.Value, "data1"] = (
#        [sc.Dim.Z, sc.Dim.Y, sc.Dim.X], np.zeros(24).reshape(4, 3, 2))
#    d[sc.Data.Value, "data1"] = (
#        [sc.Dim.Z, sc.Dim.Y, sc.Dim.X], np.arange(24).reshape(4, 3, 2))
#    self.assertEqual(len(d), 1)
#    np.testing.assert_array_equal(
#        d[sc.Data.Value, "data1"].numpy, self.reference_data1)
#
# def test_insert_Variable(self):
#    d = sc.Dataset()
#    d[sc.Data.Value, "data1"] = (
#        [sc.Dim.Z, sc.Dim.Y, sc.Dim.X], np.arange(24).reshape(4, 3, 2))
#
#    var = sc.Variable([sc.Dim.X], np.arange(2))
#    d[sc.Data.Value, "data2"] = var
#    d[sc.Data.Variance, "data2"] = var
#    self.assertEqual(len(d), 3)
#
# def test_insert_variable_slice(self):
#    d = sc.Dataset()
#    d[sc.Data.Value, "data1"] = (
#        [sc.Dim.Z, sc.Dim.Y, sc.Dim.X], np.arange(24).reshape(4, 3, 2))
#
#    d[sc.Data.Value, "data2"] = d[sc.Data.Value, "data1"]
#    d[sc.Data.Variance, "data2"] = d[sc.Data.Value, "data1"]
#    self.assertEqual(len(d), 3)
#
# This characterises existing broken behaviour. Will need to be fixed.
# def test_demo_int_to_float_issue(self):
#    # Demo bug
#    d = sc.Dataset()
#    # Variable containing int array data
#    d[sc.Data.Value, "v1"] = ([sc.Dim.X, sc.Dim.Y], np.ndarray.tolist(
#        np.arange(0, 10).reshape(2, 5)))
#    # Correct behaviour should be int64
#    self.assertEqual(d[sc.Data.Value, "v1"].numpy.dtype, 'float64')
#
#    # Demo working 1D
#    d = sc.Dataset()
#    d[sc.Data.Value, "v2"] = ([sc.Dim.X], np.ndarray.tolist(
#        np.arange(0, 10)))  # Variable containing int array data
#    self.assertEqual(d[sc.Data.Value, "v2"].numpy.dtype, 'int64')
#
# def test_set_data(self):
#    d = sc.Dataset()
#    d[sc.Data.Value, "data1"] = (
#        [sc.Dim.Z, sc.Dim.Y, sc.Dim.X], np.arange(24).reshape(4, 3, 2))
#    self.assertEqual(d[sc.Data.Value, "data1"].numpy.dtype, np.int64)
#    d[sc.Data.Value, "data1"] = np.arange(24).reshape(4, 3, 2)
#    self.assertEqual(d[sc.Data.Value, "data1"].numpy.dtype, np.int64)
#    # For existing items we do *not* change the dtype, but convert.
#    d[sc.Data.Value, "data1"] = np.arange(24.0).reshape(4, 3, 2)
#    self.assertEqual(d[sc.Data.Value, "data1"].numpy.dtype, np.int64)
#
# def test_nested_0D_empty_item(self):
#    d = sc.Dataset()
#    d[sc.Data.Events] = ([], sc.Dataset())
#    self.assertEqual(d[sc.Data.Events].data[0], sc.Dataset())
#
# def test_set_data_nested(self):
#    d = sc.Dataset()
#    table = sc.Dataset()
#    table[sc.Data.Value, "col1"] = ([sc.Dim.Row], [3.0, 2.0, 1.0, 0.0])
#    table[sc.Data.Value, "col2"] = ([sc.Dim.Row], np.arange(4.0))
#    d[sc.Data.Value, "data1"] = ([sc.Dim.X], [table, table])
#    d[sc.Data.Value, "data1"].data[1][sc.Data.Value, "col1"].data[0] = 0.0
#    self.assertEqual(d[sc.Data.Value, "data1"].data[0], table)
#    self.assertNotEqual(d[sc.Data.Value, "data1"].data[1], table)
#    self.assertNotEqual(d[sc.Data.Value, "data1"].data[0],
#                        d[sc.Data.Value, "data1"].data[1])
#
# def test_dimensions(self):
#    self.assertEqual(self.dataset.dimensions[sc.Dim.X], 2)
#    self.assertEqual(self.dataset.dimensions[sc.Dim.Y], 3)
#    self.assertEqual(self.dataset.dimensions[sc.Dim.Z], 4)
#
# def test_data(self):
#    self.assertEqual(len(self.dataset[sc.Coord.X].data), 2)
#    self.assertSequenceEqual(self.dataset[sc.Coord.X].data, [0, 1])
#    # `data` property provides a flat view
#    self.assertEqual(len(self.dataset[sc.Data.Value, "data1"].data), 24)
#    self.assertSequenceEqual(
#        self.dataset[sc.Data.Value, "data1"].data, range(24))
#
# def test_numpy_data(self):
#    np.testing.assert_array_equal(
#        self.dataset[sc.Coord.X].numpy, self.reference_x)
#    np.testing.assert_array_equal(
#        self.dataset[sc.Coord.Y].numpy, self.reference_y)
#    np.testing.assert_array_equal(
#        self.dataset[sc.Coord.Z].numpy, self.reference_z)
#    np.testing.assert_array_equal(
#        self.dataset[sc.Data.Value, "data1"].numpy, self.reference_data1)
#    np.testing.assert_array_equal(
#        self.dataset[sc.Data.Value, "data2"].numpy, self.reference_data2)
#    np.testing.assert_array_equal(
#        self.dataset[sc.Data.Value, "data3"].numpy, self.reference_data3)
#
# def test_view_subdata(self):
#    view = self.dataset.subset["data1"]
#    # TODO Need consistent dimensions() implementation for Dataset and its
#    # views.
#    self.assertEqual(view.dimensions[sc.Dim.X], 2)
#    self.assertEqual(view.dimensions[sc.Dim.Y], 3)
#    self.assertEqual(view.dimensions[sc.Dim.Z], 4)
#    self.assertEqual(len(view), 4)
#
# def test_insert_subdata(self):
#    d1 = sc.Dataset()
#    d1[sc.Data.Value, "a"] = ([sc.Dim.X], np.arange(10, dtype="double"))
#    d1[sc.Data.Variance, "a"] = ([sc.Dim.X], np.arange(10, dtype="double"))
#    ds_slice = d1.subset["a"]
#
#    d2 = sc.Dataset()
#    # Insert from subset
#    d2.subset["a"] = ds_slice
#    self.assertEqual(len(d1), len(d2))
#    self.assertEqual(d1, d2)
#
#    d3 = sc.Dataset()
#    # Insert from subset
#    d3.subset["b"] = ds_slice
#    self.assertEqual(len(d3), 2)
#    self.assertNotEqual(d1, d3)  # imported names should differ
#
#    d4 = sc.Dataset()
#    d4.subset["2a"] = ds_slice + ds_slice
#    self.assertEqual(len(d4), 2)
#    self.assertNotEqual(d1, d4)
#    self.assertTrue(np.array_equal(
#        d4[sc.Data.Value, "2a"].numpy,
#        ds_slice[sc.Data.Value, "a"].numpy * 2))
#
# def test_insert_subdata_different_variable_types(self):
#    a = sc.Dataset()
#    xcoord = sc.Variable([sc.Dim.X], np.arange(4))
#    a[sc.Data.Value] = ([sc.Dim.X], np.arange(3))
#    a[sc.Coord.X] = xcoord
#    a[sc.Attr.ExperimentLog] = ([], sc.Dataset())
#
#    b = sc.Dataset()
#    with self.assertRaises(RuntimeError):
#        b.subset["b"] = a[sc.Dim.X, :]  # Coordinates dont match
#    b[sc.Coord.X] = xcoord
#    b.subset["b"] = a[sc.Dim.X, :]  # Should now work
#    self.assertEqual(len(a), len(b))
#    self.assertTrue((sc.Attr.ExperimentLog, "b") in b)
#
# def test_slice_dataset(self):
#    for x in range(2):
#        view = self.dataset[sc.Dim.X, x]
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
#            view[sc.Data.Value, "data1"].numpy,
#            self.reference_data1[:, :, x])
#        np.testing.assert_array_equal(
#            view[sc.Data.Value, "data2"].numpy,
#            self.reference_data2[:, :, x])
#        np.testing.assert_array_equal(
#            view[sc.Data.Value, "data3"].numpy,
#            self.reference_data3[:, x])
#    for y in range(3):
#        view = self.dataset[sc.Dim.Y, y]
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
#            view[sc.Data.Value, "data1"].numpy,
#            self.reference_data1[:, y, :])
#        np.testing.assert_array_equal(
#            view[sc.Data.Value, "data2"].numpy,
#            self.reference_data2[:, y, :])
#        np.testing.assert_array_equal(
#            view[sc.Data.Value, "data3"].numpy,
#            self.reference_data3)
#    for z in range(4):
#        view = self.dataset[sc.Dim.Z, z]
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
#            view[sc.Data.Value, "data1"].numpy,
#            self.reference_data1[z, :, :])
#        np.testing.assert_array_equal(
#            view[sc.Data.Value, "data2"].numpy,
#            self.reference_data2[z, :, :])
#        np.testing.assert_array_equal(
#            view[sc.Data.Value, "data3"].numpy, self.reference_data3[z, :])
#    for x in range(2):
#        for delta in range(3 - x):
#            view = self.dataset[sc.Dim.X, x:x + delta]
#            np.testing.assert_array_equal(
#                view[sc.Coord.X].numpy, self.reference_x[x:x + delta])
#            np.testing.assert_array_equal(
#                view[sc.Coord.Y].numpy, self.reference_y)
#            np.testing.assert_array_equal(
#                view[sc.Coord.Z].numpy, self.reference_z)
#            np.testing.assert_array_equal(
#                view[sc.Data.Value, "data1"].numpy,
#                self.reference_data1[:, :, x:x + delta])
#            np.testing.assert_array_equal(
#                view[sc.Data.Value, "data2"].numpy,
#                self.reference_data2[:, :, x:x + delta])
#            np.testing.assert_array_equal(
#                view[sc.Data.Value, "data3"].numpy,
#                self.reference_data3[:, x:x + delta])
#    for y in range(3):
#        for delta in range(4 - y):
#            view = self.dataset[sc.Dim.Y, y:y + delta]
#            np.testing.assert_array_equal(
#                view[sc.Coord.X].numpy, self.reference_x)
#            np.testing.assert_array_equal(
#                view[sc.Coord.Y].numpy, self.reference_y[y:y + delta])
#            np.testing.assert_array_equal(
#                view[sc.Coord.Z].numpy, self.reference_z)
#            np.testing.assert_array_equal(
#                view[sc.Data.Value, "data1"].numpy,
#                self.reference_data1[:, y:y + delta, :])
#            np.testing.assert_array_equal(
#                view[sc.Data.Value, "data2"].numpy,
#                self.reference_data2[:, y:y + delta, :])
#            np.testing.assert_array_equal(
#                view[sc.Data.Value, "data3"].numpy,
#                self.reference_data3)
#    for z in range(4):
#        for delta in range(5 - z):
#            view = self.dataset[sc.Dim.Z, z:z + delta]
#            np.testing.assert_array_equal(
#                view[sc.Coord.X].numpy, self.reference_x)
#            np.testing.assert_array_equal(
#                view[sc.Coord.Y].numpy, self.reference_y)
#            np.testing.assert_array_equal(
#                view[sc.Coord.Z].numpy, self.reference_z[z:z + delta])
#            np.testing.assert_array_equal(
#                view[sc.Data.Value, "data1"].numpy,
#                self.reference_data1[z:z + delta, :, :])
#            np.testing.assert_array_equal(
#                view[sc.Data.Value, "data2"].numpy,
#                self.reference_data2[z:z + delta, :, :])
#            np.testing.assert_array_equal(
#                view[sc.Data.Value, "data3"].numpy,
#                self.reference_data3[z:z + delta, :])
#
# def _apply_test_op_rhs_dataset(
#    self,
#    op,
#    a,
#    b,
#    data,
#    lh_var_name="i",
#        rh_var_name="j"):
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
#    lh_var_name="i",
#        rh_var_name="j"):
#    # Assume numpy operations are correct as comparitor
#    op(data, b.numpy)
#    op(a, b)
#    # Desired nan comparisons
#    np.testing.assert_equal(a[sc.Data.Value, lh_var_name].numpy, data)
#
# def test_binary_dataset_rhs_operations(self):
#    a = sc.Dataset()
#    a[sc.Coord.X] = ([sc.Dim.X], np.arange(10))
#    a[sc.Data.Value, "i"] = ([sc.Dim.X], np.arange(10, dtype='float64'))
#    a[sc.Data.Variance, "i"] = ([sc.Dim.X], np.arange(10, dtype='float64'))
#    b = sc.Dataset()
#    b[sc.Data.Value, "j"] = ([sc.Dim.X], np.arange(10, dtype='float64'))
#    b[sc.Data.Variance, "j"] = ([sc.Dim.X], np.arange(10, dtype='float64'))
#    data = np.copy(a[sc.Data.Value, "i"].numpy)
#    variance = np.copy(a[sc.Data.Variance, "i"].numpy)
#
#    c = a + b
#    # Variables "i" and "j" added despite different names
#    self.assertTrue(np.array_equal(c[sc.Data.Value, "i"].numpy,
#                                   data + data))
#    self.assertTrue(np.array_equal(
#        c[sc.Data.Variance, "i"].numpy, variance + variance))
#
#    c = a - b
#    # Variables "a" and "b" subtracted despite different names
#    self.assertTrue(np.array_equal(c[sc.Data.Value, "i"].numpy,
#                                   data - data))
#    self.assertTrue(np.array_equal(
#        c[sc.Data.Variance, "i"].numpy, variance + variance))
#
#    c = a * b
#    # Variables "a" and "b" multiplied despite different names
#    self.assertTrue(np.array_equal(c[sc.Data.Value, "i"].numpy,
#                                   data * data))
#    self.assertTrue(np.array_equal(
#        c[sc.Data.Variance, "i"].numpy, variance * (data * data) * 2))
#
#    c = a / b
#    # Variables "a" and "b" divided despite different names
#    with np.errstate(invalid="ignore"):
#        np.testing.assert_equal(c[sc.Data.Value, "i"].numpy, data / data)
#        np.testing.assert_equal(c[sc.Data.Variance, "i"].numpy,
#                                2 * variance / (data * data))
#
#    self._apply_test_op_rhs_dataset(operator.iadd, a, b, data)
#    self._apply_test_op_rhs_dataset(operator.isub, a, b, data)
#    self._apply_test_op_rhs_dataset(operator.imul, a, b, data)
#    with np.errstate(invalid="ignore"):
#        self._apply_test_op_rhs_dataset(operator.itruediv, a, b, data)
#
# def test_binary_variable_rhs_operations(self):
#    data = np.ones(10, dtype='float64')
#
#    a = sc.Dataset()
#    a[sc.Coord.X] = ([sc.Dim.X], np.arange(10))
#    a[sc.Data.Value, "i"] = ([sc.Dim.X], data)
#
#    b_var = sc.Variable([sc.Dim.X], data)
#
#    c = a + b_var
#    self.assertTrue(np.array_equal(c[sc.Data.Value, "i"].numpy,
#                                   data + data))
#    c = a - b_var
#    self.assertTrue(np.array_equal(c[sc.Data.Value, "i"].numpy,
#                                   data - data))
#    c = a * b_var
#    self.assertTrue(np.array_equal(c[sc.Data.Value, "i"].numpy,
#                                   data * data))
#    c = a / b_var
#    self.assertTrue(np.array_equal(c[sc.Data.Value, "i"].numpy,
#                                   data / data))
#
#    self._apply_test_op_rhs_Variable(operator.iadd, a, b_var, data)
#    self._apply_test_op_rhs_Variable(operator.isub, a, b_var, data)
#    self._apply_test_op_rhs_Variable(operator.imul, a, b_var, data)
#    with np.errstate(invalid="ignore"):
#        self._apply_test_op_rhs_Variable(operator.itruediv, a, b_var, data)
#
# def test_binary_float_operations(self):
#    a = sc.Dataset()
#    a[sc.Coord.X] = ([sc.Dim.X], np.arange(10))
#    a[sc.Data.Value, "i"] = ([sc.Dim.X], np.arange(10, dtype='float64'))
#    b = sc.Dataset()
#    b[sc.Data.Value, "j"] = ([sc.Dim.X], np.arange(10, dtype='float64'))
#    data = np.copy(a[sc.Data.Value, "i"].numpy)
#
#    c = a + 2.0
#    self.assertTrue(np.array_equal(c[sc.Data.Value, "i"].numpy,
#                                   data + 2.0))
#    c = a - b
#    self.assertTrue(np.array_equal(c[sc.Data.Value, "i"].numpy,
#                                   data - data))
#    c = a - 2.0
#    self.assertTrue(np.array_equal(c[sc.Data.Value, "i"].numpy,
#                                   data - 2.0))
#    c = a * 2.0
#    self.assertTrue(np.array_equal(c[sc.Data.Value, "i"].numpy,
#                                   data * 2.0))
#    c = a / 2.0
#    self.assertTrue(np.array_equal(c[sc.Data.Value, "i"].numpy,
#                                   data / 2.0))
#    c = 2.0 + a
#    self.assertTrue(np.array_equal(c[sc.Data.Value, "i"].numpy,
#                                   data + 2.0))
#    c = 2.0 - a
#    self.assertTrue(np.array_equal(c[sc.Data.Value, "i"].numpy,
#                                   2.0 - data))
#    c = 2.0 * a
#    self.assertTrue(np.array_equal(c[sc.Data.Value, "i"].numpy,
#                                   data * 2.0))
#
# def test_equal_not_equal(self):
#    a = sc.Dataset()
#    a[sc.Coord.X] = ([sc.Dim.X], np.arange(10))
#    a[sc.Data.Value, "i"] = ([sc.Dim.X], np.arange(10, dtype='float64'))
#    b = sc.Dataset()
#    b[sc.Data.Value, "j"] = ([sc.Dim.X], np.arange(10, dtype='float64'))
#    c = a + b
#    d = sc.Dataset()
#    d[sc.Coord.X] = ([sc.Dim.X], np.arange(10))
#    d[sc.Data.Value, "i"] = ([sc.Dim.X], np.arange(10, dtype='float64'))
#    a_slice = a[sc.Dim.X, :]
#    d_slice = d[sc.Dim.X, :]
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
# def test_plus_equals_slice(self):
#    dataset = sc.Dataset()
#    dataset[sc.Data.Value, "data1"] = (
#        [sc.Dim.Z, sc.Dim.Y, sc.Dim.X], self.reference_data1)
#    a = sc.Dataset(dataset[sc.Dim.X, 0])
#    b = dataset[sc.Dim.X, 1]
#    a += b
#
# def test_numpy_interoperable(self):
#    # TODO: Need also __setitem__ with view.
#    # self.dataset[sc.Data.Value, 'data2'] =
#    #     self.dataset[sc.Data.Value, 'data1']
#    self.dataset[sc.Data.Value, 'data2'] = np.exp(
#        self.dataset[sc.Data.Value, 'data1'])
#    np.testing.assert_array_equal(
#        self.dataset[sc.Data.Value, "data2"].numpy,
#        np.exp(self.reference_data1))
#    # Restore original value.
#    self.dataset[sc.Data.Value, 'data2'] = self.reference_data2
#    np.testing.assert_array_equal(
#        self.dataset[sc.Data.Value, "data2"].numpy, self.reference_data2)
#
# def test_slice_numpy_interoperable(self):
#    # Dataset subset then view single variable
#    self.dataset.subset['data2'][sc.Data.Value, 'data2'] = np.exp(
#        self.dataset[sc.Data.Value, 'data1'])
#    np.testing.assert_array_equal(
#        self.dataset[sc.Data.Value, "data2"].numpy,
#        np.exp(self.reference_data1))
#    # Slice view of dataset then view single variable
#    self.dataset[sc.Dim.X, 0][sc.Data.Value, 'data2'] = np.exp(
#        self.dataset[sc.Dim.X, 1][sc.Data.Value, 'data1'])
#    np.testing.assert_array_equal(
#        self.dataset[sc.Data.Value, "data2"].numpy[..., 0],
#        np.exp(self.reference_data1[..., 1]))
#    # View single variable then slice view
#    self.dataset[sc.Data.Value, 'data2'][sc.Dim.X, 1] = np.exp(
#        self.dataset[sc.Data.Value, 'data1'][sc.Dim.X, 0])
#    np.testing.assert_array_equal(
#        self.dataset[sc.Data.Value, "data2"].numpy[..., 1],
#        np.exp(self.reference_data1[..., 0]))
#    # View single variable then view range of slices
#    self.dataset[sc.Data.Value, 'data2'][sc.Dim.Y, 1:3] = np.exp(
#        self.dataset[sc.Data.Value, 'data1'][sc.Dim.Y, 0:2])
#    np.testing.assert_array_equal(self.dataset[sc.Data.Value,
#                                               "data2"].numpy[:, 1:3, :],
#                                  np.exp(self.reference_data1[:, 0:2, :]))
#
#    # Restore original value.
#    self.dataset[sc.Data.Value, 'data2'] = self.reference_data2
#    np.testing.assert_array_equal(
#        self.dataset[sc.Data.Value, "data2"].numpy, self.reference_data2)
#
# def test_concatenate(self):
#    dataset = sc.Dataset()
#    dataset[sc.Data.Value, "data"] = ([sc.Dim.X], np.arange(4))
#    dataset[sc.Coord.X] = ([sc.Dim.X], np.array([3, 2, 4, 1]))
#    dataset = sc.concatenate(dataset, dataset, sc.Dim.X)
#    np.testing.assert_array_equal(
#        dataset[sc.Coord.X].numpy, np.array([3, 2, 4, 1, 3, 2, 4, 1]))
#    np.testing.assert_array_equal(
#        dataset[sc.Data.Value, "data"].numpy,
#        np.array([0, 1, 2, 3, 0, 1, 2, 3]))
#
# def test_concatenate_with_slice(self):
#    dataset = sc.Dataset()
#    dataset[sc.Data.Value, "data"] = ([sc.Dim.X], np.arange(4))
#    dataset[sc.Coord.X] = ([sc.Dim.X], np.array([3, 2, 4, 1]))
#    dataset = sc.concatenate(dataset, dataset[sc.Dim.X, 0:2], sc.Dim.X)
#    np.testing.assert_array_equal(
#        dataset[sc.Coord.X].numpy, np.array([3, 2, 4, 1, 3, 2]))
#    np.testing.assert_array_equal(
#        dataset[sc.Data.Value, "data"].numpy, np.array([0, 1, 2, 3, 0, 1]))
#
# def test_rebin(self):
#    dataset = sc.Dataset()
#    dataset[sc.Data.Value, "data"] = ([sc.Dim.X], np.array(10 * [1.0]))
#    dataset[sc.Data.Value, "data"].unit = sc.units.counts
#    dataset[sc.Coord.X] = ([sc.Dim.X], np.arange(11.0))
#    new_coord = sc.Variable([sc.Dim.X], np.arange(0.0, 11, 2))
#    dataset = sc.rebin(dataset, new_coord)
#    np.testing.assert_array_equal(
#        dataset[sc.Data.Value, "data"].numpy, np.array(5 * [2]))
#    np.testing.assert_array_equal(
#        dataset[sc.Coord.X].numpy, np.arange(0, 11, 2))
#
# def test_sort(self):
#    dataset = sc.Dataset()
#    dataset[sc.Data.Value, "data"] = ([sc.Dim.X], np.arange(4))
#    dataset[sc.Data.Value, "data2"] = ([sc.Dim.X], np.arange(4))
#    dataset[sc.Coord.X] = ([sc.Dim.X], np.array([3, 2, 4, 1]))
#    dataset = sc.sort(dataset, sc.Coord.X)
#    np.testing.assert_array_equal(
#        dataset[sc.Data.Value, "data"].numpy, np.array([3, 1, 0, 2]))
#    np.testing.assert_array_equal(
#        dataset[sc.Coord.X].numpy, np.array([1, 2, 3, 4]))
#    dataset = sc.sort(dataset, sc.Data.Value, "data")
#    np.testing.assert_array_equal(
#        dataset[sc.Data.Value, "data"].numpy, np.arange(4))
#    np.testing.assert_array_equal(
#        dataset[sc.Coord.X].numpy, np.array([3, 2, 4, 1]))
#
# def test_filter(self):
#    dataset = sc.Dataset()
#    dataset[sc.Data.Value, "data"] = ([sc.Dim.X], np.arange(4))
#    dataset[sc.Coord.X] = ([sc.Dim.X], np.array([3, 2, 4, 1]))
#    select = sc.Variable([sc.Dim.X], np.array([False, True, False, True]))
#    dataset = sc.filter(dataset, select)
#    np.testing.assert_array_equal(
#        dataset[sc.Data.Value, "data"].numpy, np.array([1, 3]))
#    np.testing.assert_array_equal(dataset[sc.Coord.X].numpy,
#                                  np.array([2, 1]))
#
#
# s TestDatasetExamples(unittest.TestCase):
# def test_table_example(self):
#    table = sc.Dataset()
#    table[sc.Coord.Row] = ([sc.Dim.Row], ['a', 'bb', 'ccc', 'dddd'])
#    self.assertSequenceEqual(table[sc.Coord.Row].data, [
#                             'a', 'bb', 'ccc', 'dddd'])
#    table[sc.Data.Value, "col1"] = ([sc.Dim.Row], [3.0, 2.0, 1.0, 0.0])
#    table[sc.Data.Value, "col2"] = ([sc.Dim.Row], np.arange(4.0))
#    self.assertEqual(len(table), 3)
#
#    table[sc.Data.Value, "sum"] = ([sc.Dim.Row],
#                                   (len(table[sc.Coord.Row]),))
#
#    for name, tag, col in table:
#        if not tag.is_coord and name != "sum":
#            table[sc.Data.Value, "sum"] += col
#    np.testing.assert_array_equal(
#        table[sc.Data.Value, "col2"].numpy, np.array([0, 1, 2, 3]))
#    np.testing.assert_array_equal(
#        table[sc.Data.Value, "sum"].numpy, np.array([3, 3, 3, 3]))
#
#    table = sc.concatenate(table, table, sc.Dim.Row)
#    np.testing.assert_array_equal(
#        table[sc.Data.Value, "col1"].numpy,
#        np.array([3, 2, 1, 0, 3, 2, 1, 0]))
#
#    table = sc.concatenate(table[sc.Dim.Row, 0:2], table[sc.Dim.Row, 5:7],
#                           sc.Dim.Row)
#    np.testing.assert_array_equal(
#        table[sc.Data.Value, "col1"].numpy, np.array([3, 2, 2, 1]))
#
#    table = sc.sort(table, sc.Data.Value, "col1")
#    np.testing.assert_array_equal(
#        table[sc.Data.Value, "col1"].numpy, np.array([1, 2, 2, 3]))
#
#    table = sc.sort(table, sc.Coord.Row)
#    np.testing.assert_array_equal(
#        table[sc.Data.Value, "col1"].numpy, np.array([3, 2, 2, 1]))
#
#    for i in range(1, len(table[sc.Coord.Row])):
#        table[sc.Dim.Row, i] += table[sc.Dim.Row, i - 1]
#
#    np.testing.assert_array_equal(
#        table[sc.Data.Value, "col1"].numpy, np.array([3, 5, 7, 8]))
#
#    table[sc.Data.Value, "exp1"] = (
#        [sc.Dim.Row], np.exp(table[sc.Data.Value, "col1"]))
#    table[sc.Data.Value, "exp1"] -= table[sc.Data.Value, "col1"]
#    np.testing.assert_array_equal(table[sc.Data.Value, "exp1"].numpy,
#                                  np.exp(np.array([3, 5, 7, 8]))
#                                  - np.array([3, 5, 7, 8]))
#
#    table += table
#    self.assertSequenceEqual(table[sc.Coord.Row].data, [
#                             'a', 'bb', 'bb', 'ccc'])
#
# def test_table_example_no_assert(self):
#    table = sc.Dataset()
#
#    # Add columns
#    table[sc.Coord.Row] = ([sc.Dim.Row], ['a', 'bb', 'ccc', 'dddd'])
#    table[sc.Data.Value, "col1"] = ([sc.Dim.Row], [3.0, 2.0, 1.0, 0.0])
#    table[sc.Data.Value, "col2"] = ([sc.Dim.Row], np.arange(4.0))
#    table[sc.Data.Value, "sum"] = ([sc.Dim.Row], (4,))
#
#    # Do something for each column (here: sum)
#    for name, tag, col in table:
#        if not tag.is_coord and name != "sum":
#            table[sc.Data.Value, "sum"] += col
#
#    # Append tables (append rows of second table to first)
#    table = sc.concatenate(table, table, sc.Dim.Row)
#
#    # Append tables sections (e.g., to remove rows from the middle)
#    table = sc.concatenate(table[sc.Dim.Row, 0:2], table[sc.Dim.Row, 5:7],
#                           sc.Dim.Row)
#
#    # Sort by column
#    table = sc.sort(table, sc.Data.Value, "col1")
#    # ... or another one
#    table = sc.sort(table, sc.Coord.Row)
#
#    # Do something for each row (here: cumulative sum)
#    for i in range(1, len(table[sc.Coord.Row])):
#        table[sc.Dim.Row, i] += table[sc.Dim.Row, i - 1]
#
#    # Apply numpy function to column, store result as a new column
#    table[sc.Data.Value, "exp1"] = (
#        [sc.Dim.Row], np.exp(table[sc.Data.Value, "col1"]))
#    # ... or as an existing column
#    table[sc.Data.Value, "exp1"] = np.sin(table[sc.Data.Value, "exp1"])
#
#    # Remove column
#    del table[sc.Data.Value, "exp1"]
#
#    # Arithmetics with tables (here: add two tables)
#    table += table
#
# def test_MDHistoWorkspace_example(self):
#    L = 30
#    d = sc.Dataset()
#
#    # Add bin-edge axis for X
#    d[sc.Coord.X] = ([sc.Dim.X], np.arange(L + 1).astype(np.float64))
#    # ... and normal axes for Y and Z
#    d[sc.Coord.Y] = ([sc.Dim.Y], np.arange(L))
#    d[sc.Coord.Z] = ([sc.Dim.Z], np.arange(L))
#
#    # Add data variables
#    d[sc.Data.Value, "temperature"] = (
#        [sc.Dim.Z, sc.Dim.Y, sc.Dim.X],
#        np.random.normal(size=L * L * L).reshape([L, L, L]))
#    d[sc.Data.Value, "pressure"] = (
#        [sc.Dim.Z, sc.Dim.Y, sc.Dim.X],
#        np.random.normal(size=L * L * L).reshape([L, L, L]))
#    # Add uncertainties, matching name implicitly links it to corresponding
#    # data
#    d[sc.Data.Variance, "temperature"] = (
#        [sc.Dim.Z, sc.Dim.Y, sc.Dim.X],
#        d[sc.Data.Value, "temperature"].numpy)
#
#    # Uncertainties are propagated using grouping mechanism based on name
#    square = d * d
#    np.testing.assert_array_equal(
#        square[sc.Data.Variance, "temperature"].numpy,
#        2.0 * (d[sc.Data.Value, "temperature"].numpy**2
#               * d[sc.Data.Variance, "temperature"].numpy))
#
#    # Add the counts units
#    d[sc.Data.Value, "temperature"].unit = sc.units.counts
#    d[sc.Data.Value, "pressure"].unit = sc.units.counts
#    d[sc.Data.Variance, "temperature"].unit = sc.units.counts \
#        * sc.units.counts
#    # The square operation is now prevented because the resulting counts
#    # variance unit (counts^4) is not part of the supported units, i.e. the
#    # result of that operation makes little physical sense.
#    with self.assertRaisesRegex(RuntimeError,
#                                r"Unsupported unit as result of "
#                                r"multiplication: \(counts\^2\) \* "
#                                r"\(counts\^2\)"):
#        d = d * d
#
#    # Rebin the X axis
#    d = sc.rebin(d, sc.Variable(
#        [sc.Dim.X], np.arange(0, L + 1, 2).astype(np.float64)))
#    # Rebin to different axis for every y
#    # Our rebin implementatinon is broken for this case for now
#    # rebinned = sc.rebin(d, sc.Variable(sc.Coord.X, [sc.Dim.Y, sc.Dim.X],
#    #                  np.arange(0,2*L).reshape([L,2]).astype(np.float64)))
#
#    # Do something with numpy and insert result
#    d[sc.Data.Value, "dz(p)"] = ([sc.Dim.Z, sc.Dim.Y, sc.Dim.X],
#                                 np.gradient(d[sc.Data.Value, "pressure"],
#                                             d[sc.Coord.Z], axis=0))
#
#    # Truncate Y and Z axes
#    d = sc.Dataset(d[sc.Dim.Y, 10:20][sc.Dim.Z, 10:20])
#
#    # Mean along Y axis
#    meanY = sc.mean(d, sc.Dim.Y)
#    # Subtract from original, making use of automatic broadcasting
#    d -= meanY
#
#    # Extract a Z slice
#    sliceZ = sc.Dataset(d[sc.Dim.Z, 7])
#    self.assertEqual(len(sliceZ.dimensions), 2)
#
# def test_Workspace2D_example(self):
#    d = sc.Dataset()
#
#    d[sc.Coord.SpectrumNumber] = ([sc.Dim.Spectrum], np.arange(1, 101))
#
#    # Add a (common) time-of-flight axis
#    d[sc.Coord.Tof] = ([sc.Dim.Tof], np.arange(1000))
#
#    # Add data with uncertainties
#    d[sc.Data.Value, "sample1"] = (
#        [sc.Dim.Spectrum, sc.Dim.Tof],
#        np.random.exponential(size=100 * 1000).reshape([100, 1000]))
#    d[sc.Data.Variance, "sample1"] = d[sc.Data.Value, "sample1"]
#
#    # Create a mask and use it to extract some of the spectra
#    select = sc.Variable([sc.Dim.Spectrum], np.isin(
#        d[sc.Coord.SpectrumNumber], np.arange(10, 20)))
#    spectra = sc.filter(d, select)
#    self.assertEqual(spectra.dimensions.shape[1], 10)
#
#    # Direct representation of a simple instrument (more standard Mantid
#    # instrument representation is of course supported, this is just to
#    # demonstrate the flexibility)
#    steps = np.arange(-0.45, 0.46, 0.1)
#    x = np.tile(steps, (10,))
#    y = x.reshape([10, 10]).transpose().flatten()
#    d[sc.Coord.X] = ([sc.Dim.Spectrum], x)
#    d[sc.Coord.Y] = ([sc.Dim.Spectrum], y)
#    d[sc.Coord.Z] = ([], 10.0)
#
#    # Mask some spectra based on distance from beam center
#    r = np.sqrt(np.square(d[sc.Coord.X]) + np.square(d[sc.Coord.Y]))
#    d[sc.Coord.Mask] = ([sc.Dim.Spectrum], np.less(r, 0.2))
#
#    # Do something for each spectrum (here: apply mask)
#    d[sc.Coord.Mask].data
#    for i, masked in enumerate(d[sc.Coord.Mask].numpy):
#        spec = d[sc.Dim.Spectrum, i]
#        if masked:
#            spec[sc.Data.Value, "sample1"] = np.zeros(1000)
#            spec[sc.Data.Variance, "sample1"] = np.zeros(1000)
#
# def test_monitors_example(self):
#    d = sc.Dataset()
#
#    d[sc.Coord.SpectrumNumber] = ([sc.Dim.Spectrum], np.arange(1, 101))
#
#    # Add a (common) time-of-flight axis
#    d[sc.Coord.Tof] = ([sc.Dim.Tof], np.arange(9))
#
#    # Add data with uncertainties
#    d[sc.Data.Value, "sample1"] = (
#        [sc.Dim.Spectrum, sc.Dim.Tof],
#        np.random.exponential(size=100 * 8).reshape([100, 8]))
#    d[sc.Data.Variance, "sample1"] = d[sc.Data.Value, "sample1"]
#
#    # Add event-mode beam-status monitor
#    status = sc.Dataset()
#    status[sc.Data.Tof] = ([sc.Dim.Event],
#                           np.random.exponential(size=1000))
#    status[sc.Data.PulseTime] = (
#        [sc.Dim.Event], np.random.exponential(size=1000))
#    d[sc.Coord.Monitor, "beam-status"] = ([], status)
#
#    # Add position-resolved beam-profile monitor
#    profile = sc.Dataset()
#    profile[sc.Coord.X] = ([sc.Dim.X], [-0.02, -0.01, 0.0, 0.01, 0.02])
#    profile[sc.Coord.Y] = ([sc.Dim.Y], [-0.02, -0.01, 0.0, 0.01, 0.02])
#    profile[sc.Data.Value] = ([sc.Dim.Y, sc.Dim.X], (4, 4))
#    # Monitors can also be attributes, so they are not required to match in
#    # operations
#    d[sc.Attr.Monitor, "beam-profile"] = ([], profile)
#
#    # Add histogram-mode transmission monitor
#    transmission = sc.Dataset()
#    transmission[sc.Coord.Energy] = ([sc.Dim.Energy], np.arange(9))
#    transmission[sc.Data.Value] = (
#        [sc.Dim.Energy], np.random.exponential(size=8))
#    d[sc.Coord.Monitor, "transmission"] = ([], ())
#
# @unittest.skip("Tag-derived dtype not available anymore, need to implement \
#               way of specifying list type for events.")
# def test_zip(self):
#    d = sc.Dataset()
#    d[sc.Coord.SpectrumNumber] = ([sc.Dim.Position], np.arange(1, 6))
#    d[sc.Data.EventTofs, ""] = ([sc.Dim.Position], (5,))
#    d[sc.Data.EventPulseTimes, ""] = ([sc.Dim.Position], (5,))
#    self.assertEqual(len(d[sc.Data.EventTofs, ""].data), 5)
#    d[sc.Data.EventTofs, ""].data[0].append(10)
#    d[sc.Data.EventPulseTimes, ""].data[0].append(1000)
#    d[sc.Data.EventTofs, ""].data[1].append(10)
#    d[sc.Data.EventPulseTimes, ""].data[1].append(1000)
#    # Don't do this, there are no compatiblity checks:
#    # for el in zip(d[sc.Data.EventTofs, ""].data,
#    #     d[sc.Data.EventPulseTimes, ""].data):
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
#    d1[sc.Coord.X] = ([sc.Dim.X], np.arange(N + 1).astype(np.float64))
#    d1[sc.Coord.Y] = ([sc.Dim.Y], np.arange(M + 1).astype(np.float64))
#
#    arr1 = np.arange(N * M).reshape(N, M).astype(np.float64)
#    arr2 = np.transpose(arr1)
#    K = 3
#    arr_buf = np.arange(N * K * M).reshape(N, K, M)
#    arr3 = arr_buf[:, 1, :]
#    d1[sc.Data.Value, "A"] = ([sc.Dim.X, sc.Dim.Y], arr1)
#    d1[sc.Data.Value, "B"] = ([sc.Dim.Y, sc.Dim.X], arr2)
#    d1[sc.Data.Value, "C"] = ([sc.Dim.X, sc.Dim.Y], arr3)
#    np.testing.assert_array_equal(arr1, d1[sc.Data.Value, "A"].numpy)
#    np.testing.assert_array_equal(arr2, d1[sc.Data.Value, "B"].numpy)
#    np.testing.assert_array_equal(arr3, d1[sc.Data.Value, "C"].numpy)
#
# def test_rebin(self):
#    N = 6
#    M = 4
#    d1 = sc.Dataset()
#    d1[sc.Coord.X] = ([sc.Dim.X], np.arange(N + 1).astype(np.float64))
#    d1[sc.Coord.Y] = ([sc.Dim.Y], np.arange(M + 1).astype(np.float64))
#
#    arr1 = np.arange(N * M).reshape(N, M).astype(np.float64)
#    # TODO copy not needed after correct treatment of strides
#    arr2 = np.transpose(arr1).copy()
#
#    d1[sc.Data.Value, "A"] = ([sc.Dim.X, sc.Dim.Y], arr1)
#    d1[sc.Data.Value, "B"] = ([sc.Dim.Y, sc.Dim.X], arr2)
#    d1[sc.Data.Value, "A"].unit = sc.units.counts
#    d1[sc.Data.Value, "B"].unit = sc.units.counts
#    rd1 = sc.rebin(d1, sc.Variable([sc.Dim.X], np.arange(
#        0, N + 1, 1.5).astype(np.float64)))
#    np.testing.assert_array_equal(rd1[sc.Data.Value, "A"].numpy,
#                                  np.transpose(
#                                      rd1[sc.Data.Value, "B"].numpy))
#
# def test_copy(self):
#    import copy
#    N = 6
#    M = 4
#    d1 = sc.Dataset()
#    d1[sc.Coord.X] = ([sc.Dim.X], np.arange(N + 1).astype(np.float64))
#    d1[sc.Coord.Y] = ([sc.Dim.Y], np.arange(M + 1).astype(np.float64))
#    arr1 = np.arange(N * M).reshape(N, M).astype(np.float64) + 1
#    d1[sc.Data.Value, "A"] = ([sc.Dim.X, sc.Dim.Y], arr1)
#    d2 = copy.copy(d1)
#    d3 = copy.deepcopy(d2)
#    self.assertEqual(d1, d2)
#    self.assertEqual(d3, d2)
#    d2[sc.Data.Value, "A"] *= d2[sc.Data.Value, "A"]
#    self.assertNotEqual(d1, d2)
#    self.assertNotEqual(d3, d2)
#
# def test_correct_temporaries(self):
#    N = 6
#    M = 4
#    d1 = sc.Dataset()
#    d1[sc.Coord.X] = ([sc.Dim.X], np.arange(N + 1).astype(np.float64))
#    d1[sc.Coord.Y] = ([sc.Dim.Y], np.arange(M + 1).astype(np.float64))
#    arr1 = np.arange(N * M).reshape(N, M).astype(np.float64) + 1
#    d1[sc.Data.Value, "A"] = ([sc.Dim.X, sc.Dim.Y], arr1)
#    d1 = d1[sc.Dim.X, 1:2]
#    self.assertEqual(list(d1[sc.Data.Value, "A"].data),
#                     [5.0, 6.0, 7.0, 8.0])
#    d1 = d1[sc.Dim.Y, 2:3]
#    self.assertEqual(list(d1[sc.Data.Value, "A"].data), [7])
