# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import numpy as np
import pytest

import scipp as sc


def test_init_default() -> None:
    with pytest.raises(TypeError):
        sc.Dataset()


def test_init_empty() -> None:
    ds = sc.Dataset({})
    assert ds.sizes == {}
    assert len(ds) == 0

    ds = sc.Dataset(coords={})
    assert ds.sizes == {}
    assert len(ds) == 0

    ds = sc.Dataset({}, coords={})
    assert ds.sizes == {}
    assert len(ds) == 0


def test_init_dict_of_variables() -> None:
    d = sc.Dataset({'a': sc.arange('x', 5), 'b': sc.arange('x', 5, 10, unit='m')})
    assert sc.identical(d['a'], sc.DataArray(sc.arange('x', 5)))
    assert sc.identical(d['b'], sc.DataArray(sc.arange('x', 5, 10, unit='m')))


def test_init_dict_of_dataarrays() -> None:
    d = sc.Dataset(
        {
            'a': sc.DataArray(sc.arange('x', 5)),
            'b': sc.DataArray(
                sc.arange('x', 5, 10, unit='m'), coords={'x': sc.arange('x', 5, 10)}
            ),
        }
    )
    assert sc.identical(
        d['a'], sc.DataArray(sc.arange('x', 5), coords={'x': sc.arange('x', 5, 10)})
    )
    assert sc.identical(
        d['b'],
        sc.DataArray(
            sc.arange('x', 5, 10, unit='m'), coords={'x': sc.arange('x', 5, 10)}
        ),
    )


def test_init_dict_of_dataarray_and_variable() -> None:
    d = sc.Dataset(
        {
            'a': sc.arange('x', 5),
            'b': sc.DataArray(
                sc.arange('x', 5, 10, unit='m'), coords={'x': sc.arange('x', 5, 10)}
            ),
        }
    )
    assert sc.identical(
        d['a'], sc.DataArray(sc.arange('x', 5), coords={'x': sc.arange('x', 5, 10)})
    )
    assert sc.identical(
        d['b'],
        sc.DataArray(
            sc.arange('x', 5, 10, unit='m'), coords={'x': sc.arange('x', 5, 10)}
        ),
    )


def test_init_data_from_dataset() -> None:
    d1 = sc.Dataset(
        {
            'a': sc.arange('x', 5),
            'b': sc.DataArray(
                sc.arange('x', 5, 10, unit='m'), coords={'x': sc.arange('x', 5, 10)}
            ),
        }
    )
    d2 = sc.Dataset(d1)
    assert sc.identical(
        d2['a'], sc.DataArray(sc.arange('x', 5), coords={'x': sc.arange('x', 5, 10)})
    )
    assert sc.identical(
        d2['b'],
        sc.DataArray(
            sc.arange('x', 5, 10, unit='m'), coords={'x': sc.arange('x', 5, 10)}
        ),
    )


def test_init_iterator_of_tuples() -> None:
    d = sc.Dataset(
        {'a': sc.arange('x', 5), 'b': sc.arange('x', 5, 10, unit='m')}.items()
    )
    assert sc.identical(d['a'], sc.DataArray(sc.arange('x', 5)))
    assert sc.identical(d['b'], sc.DataArray(sc.arange('x', 5, 10, unit='m')))


def test_init_extra_coords_from_dict() -> None:
    d = sc.Dataset(
        {'a': sc.arange('x', 5), 'b': sc.arange('x', 5, 10, unit='m')},
        coords={'x': sc.arange('x', 5), 'y': sc.arange('x', 6)},
    )
    assert sc.identical(
        d['a'],
        sc.DataArray(
            sc.arange('x', 5), coords={'x': sc.arange('x', 5), 'y': sc.arange('x', 6)}
        ),
    )
    assert sc.identical(
        d['b'],
        sc.DataArray(
            sc.arange('x', 5, 10, unit='m'),
            coords={'x': sc.arange('x', 5), 'y': sc.arange('x', 6)},
        ),
    )


def test_init_extra_coords_from_coords_object() -> None:
    da = sc.DataArray(
        sc.arange('x', 5), coords={'x': sc.arange('x', 5), 'z': sc.scalar('zz')}
    )
    d = sc.Dataset(
        {'a': sc.arange('x', 5), 'b': sc.arange('x', 5, 10, unit='m')}, coords=da.coords
    )
    assert sc.identical(
        d['a'],
        sc.DataArray(
            sc.arange('x', 5), coords={'x': sc.arange('x', 5), 'z': sc.scalar('zz')}
        ),
    )
    assert sc.identical(
        d['b'],
        sc.DataArray(
            sc.arange('x', 5, 10, unit='m'),
            coords={'x': sc.arange('x', 5), 'z': sc.scalar('zz')},
        ),
    )


def test_init_extra_coords_from_iterator_of_tuples() -> None:
    d = sc.Dataset(
        {'a': sc.arange('x', 5), 'b': sc.arange('x', 5, 10, unit='m')},
        coords={'x': sc.arange('x', 5), 'y': sc.arange('x', 5, 11)}.items(),
    )
    assert sc.identical(
        d['a'],
        sc.DataArray(
            sc.arange('x', 5),
            coords={'x': sc.arange('x', 5), 'y': sc.arange('x', 5, 11)},
        ),
    )
    assert sc.identical(
        d['b'],
        sc.DataArray(
            sc.arange('x', 5, 10, unit='m'),
            coords={'x': sc.arange('x', 5), 'y': sc.arange('x', 5, 11)},
        ),
    )


def test_init_coords_only() -> None:
    d = sc.Dataset(coords={'x': sc.arange('x', 5), 'y': sc.arange('x', 5, 11)})
    assert len(d) == 0
    assert len(d.coords) == 2
    assert sc.identical(d.coords['x'], sc.arange('x', 5))
    assert sc.identical(d.coords['y'], sc.arange('x', 5, 11))


def test_dims() -> None:
    a = sc.scalar(1)
    d = sc.Dataset(data={'a': a})
    assert d.dims == ()
    a = sc.empty(dims=['y', 'x'], shape=[3, 2])
    d = sc.Dataset(data={'a': a})
    assert set(d.dims) == {'x', 'y'}


def test_shape() -> None:
    a = sc.scalar(1)
    d = sc.Dataset(data={'a': a})
    assert d.shape == ()
    a = sc.empty(dims=['y', 'x'], shape=[3, 2])
    d = sc.Dataset(data={'a': a})
    assert set(d.shape) == {2, 3}


def test_sizes() -> None:
    d = sc.Dataset(data={'a': sc.scalar(value=1)})
    assert d.sizes == {}
    a = sc.empty(dims=['y', 'x'], shape=[3, 2])
    d = sc.Dataset(data={'a': a})
    assert d.sizes == {'x': 2, 'y': 3}


def test_clear() -> None:
    d = sc.Dataset({'a': sc.Variable(dims=['x'], values=np.arange(3))})
    assert 'a' in d
    d.clear()
    assert len(d) == 0


def test_setitem() -> None:
    d = sc.Dataset({'a': sc.scalar(1.0)})
    assert len(d) == 1
    assert sc.identical(d['a'].data, sc.scalar(1.0))
    assert len(d.coords) == 0


def test_del_item() -> None:
    d = sc.Dataset({'a': sc.scalar(1.0)})
    assert 'a' in d
    del d['a']
    assert 'a' not in d


def test_del_item_missing() -> None:
    d = sc.Dataset({'a': sc.scalar(1.0)})
    with pytest.raises(KeyError):
        del d['not an item']


def test_update_from_dict_adds_items() -> None:
    a = sc.DataArray(sc.scalar(1.0))
    b = sc.DataArray(sc.scalar(2.0))
    b2 = sc.DataArray(sc.scalar(3.0))
    c = sc.DataArray(sc.scalar(4.0))
    d = sc.Dataset({'a': a, 'b': b})
    d.update({'b': b2, 'c': c})
    assert sc.identical(d['a'], a)
    assert sc.identical(d['b'], b2)
    assert sc.identical(d['c'], c)


def test_update_from_dataset_adds_items() -> None:
    a = sc.DataArray(sc.scalar(1.0))
    b = sc.DataArray(sc.scalar(2.0))
    b2 = sc.DataArray(sc.scalar(3.0))
    c = sc.DataArray(sc.scalar(4.0))
    d = sc.Dataset({'a': a, 'b': b})
    d.update(sc.Dataset({'b': b2, 'c': c}))
    assert sc.identical(d['a'], a)
    assert sc.identical(d['b'], b2)
    assert sc.identical(d['c'], c)


def test_update_from_sequence_of_tuples_adds_items() -> None:
    a = sc.DataArray(sc.scalar(1.0))
    b = sc.DataArray(sc.scalar(2.0))
    b2 = sc.DataArray(sc.scalar(3.0))
    c = sc.DataArray(sc.scalar(4.0))
    d = sc.Dataset({'a': a, 'b': b})
    d.update([('b', b2), ('c', c)])
    assert sc.identical(d['a'], a)
    assert sc.identical(d['b'], b2)
    assert sc.identical(d['c'], c)


def test_update_from_iterable_of_tuples_adds_items() -> None:
    a = sc.DataArray(sc.scalar(1.0))
    b = sc.DataArray(sc.scalar(2.0))
    b2 = sc.DataArray(sc.scalar(3.0))
    c = sc.DataArray(sc.scalar(4.0))

    def extra_items():
        yield 'b', b2
        yield 'c', c

    d = sc.Dataset({'a': a, 'b': b})
    d.update(extra_items())
    assert sc.identical(d['a'], a)
    assert sc.identical(d['b'], b2)
    assert sc.identical(d['c'], c)


def test_update_from_kwargs_adds_items() -> None:
    a = sc.DataArray(sc.scalar(1.0))
    b = sc.DataArray(sc.scalar(2.0))
    b2 = sc.DataArray(sc.scalar(3.0))
    c = sc.DataArray(sc.scalar(4.0))
    d = sc.Dataset({'a': a, 'b': b})
    d.update(b=b2, c=c)
    assert sc.identical(d['a'], a)
    assert sc.identical(d['b'], b2)
    assert sc.identical(d['c'], c)


def test_update_from_kwargs_overwrites_other_dict() -> None:
    a = sc.DataArray(sc.scalar(1.0))
    b = sc.DataArray(sc.scalar(2.0))
    b2 = sc.DataArray(sc.scalar(3.0))
    d = sc.Dataset({'a': a})
    d.update({'b': b}, b=b2)
    assert sc.identical(d['a'], a)
    assert sc.identical(d['b'], b2)


def test_ipython_key_completion() -> None:
    var = sc.Variable(dims=['x'], values=np.arange(4))
    da = sc.DataArray(data=var, coords={'x': var, 'aux': var})
    ds = sc.Dataset(data={'array 1': da, 'array 2': da})
    assert set(ds._ipython_key_completions_()) == set(ds.keys())


def test_slice_item() -> None:
    d = sc.Dataset({'a': sc.arange('x', 4)}, coords={'x': sc.arange('x', 4, 8)})
    assert sc.identical(d['a']['x', 2:4].data, sc.arange('x', 2, 4))
    assert sc.identical(d['a']['x', 2:4].coords['x'], sc.arange('x', 6, 8))


def test_set_item_slice_from_numpy() -> None:
    d = sc.Dataset({'a': sc.arange('x', 4)}, coords={'x': sc.arange('x', 4, 8)})
    d['a']['x', 2:4] = np.arange(2)
    assert sc.identical(d['a'].data, sc.array(dims=['x'], values=[0, 1, 0, 1]))


def test_set_item_slice_with_variances_from_numpy() -> None:
    d = sc.Dataset(
        {'a': sc.array(dims=['x'], values=np.arange(4.0), variances=np.arange(4.0))},
        coords={'x': sc.arange('x', 4, 8)},
    )
    d['a']['x', 2:4].values = np.arange(2)
    d['a']['x', 2:4].variances = np.arange(2, 4)
    assert np.array_equal(d['a'].values, np.array([0.0, 1.0, 0.0, 1.0]))
    assert np.array_equal(d['a'].variances, np.array([0.0, 1.0, 2.0, 3.0]))


def test_iadd_slice() -> None:
    d = sc.Dataset({'a': sc.arange('x', 4)}, coords={'x': sc.arange('x', 4, 8)})
    d['a']['x', 1] += d['a']['x', 2]
    assert sc.identical(d['a'].data, sc.array(dims=['x'], values=[0, 3, 2, 3]))


def test_iadd_range() -> None:
    d = sc.Dataset({'a': sc.arange('x', 4)}, coords={'x': sc.arange('x', 4, 8)})
    with pytest.raises(RuntimeError):
        d['a']['x', 2:4] += d['a']['x', 1:3]
    d['a']['x', 2:4] += d['a']['x', 2:4]
    assert sc.identical(d['a'].data, sc.array(dims=['x'], values=[0, 1, 4, 6]))


def test_contains() -> None:
    d = sc.Dataset({'a': sc.scalar(1.0)})
    assert 'a' in d
    assert 'b' not in d
    d['b'] = sc.scalar(1.0)
    assert 'a' in d
    assert 'b' in d


def test_slice() -> None:
    d = sc.Dataset(
        data={
            'a': sc.arange('x', 10.0),
        },
        coords={'x': sc.arange('x', 10.0)},
    )
    expected = sc.Dataset(
        data={
            'a': sc.DataArray(1.0 * sc.units.one, coords={'x': 1.0 * sc.units.one}),
        }
    )
    expected.coords.set_aligned('x', False)
    assert sc.identical(d['x', 1], expected)


def test_chained_slicing() -> None:
    x = sc.arange('x', 11.0)
    y = sc.arange('y', 11.0)
    z = sc.arange('z', 11.0)
    d = sc.Dataset(
        data={
            'a': sc.arange('aux', 1000.0).fold(
                'aux', sizes={'z': 10, 'y': 10, 'x': 10}
            ),
        },
        coords={'x': x, 'y': y, 'z': z},
    )

    expected = sc.Dataset({'a': sc.arange('y', 501.0, 600.0, 10.0)})
    expected.coords['x'] = x['x', 1:3]
    expected.coords.set_aligned('x', False)
    expected.coords['y'] = sc.arange('y', 11.0)
    expected.coords['z'] = z['z', 5:7]
    expected.coords.set_aligned('z', False)

    assert sc.identical(d['x', 1]['z', 5], expected)


def test_dataset_merge() -> None:
    a = sc.Dataset(data={'d1': sc.Variable(dims=['x'], values=np.array([1, 2, 3]))})
    b = sc.Dataset(data={'d2': sc.Variable(dims=['x'], values=np.array([4, 5, 6]))})
    c = sc.merge(a, b)
    assert sc.identical(a['d1'], c['d1'])
    assert sc.identical(b['d2'], c['d2'])


def test_dataset_concat() -> None:
    a = sc.Dataset(
        data={'data': sc.Variable(dims=['x'], values=np.array([11, 12]))},
        coords={'x': sc.Variable(dims=['x'], values=np.array([1, 2]))},
    )
    b = sc.Dataset(
        data={'data': sc.Variable(dims=['x'], values=np.array([13, 14]))},
        coords={'x': sc.Variable(dims=['x'], values=np.array([3, 4]))},
    )

    c = sc.concat([a, b], 'x')

    assert np.array_equal(c.coords['x'].values, np.array([1, 2, 3, 4]))
    assert np.array_equal(c['data'].values, np.array([11, 12, 13, 14]))


def test_dataset_set_data() -> None:
    d1 = sc.Dataset(
        data={
            'a': sc.array(dims=['x', 'y'], values=np.random.rand(2, 3)),
        },
        coords={
            'x': sc.arange('x', 2.0, unit=sc.units.m),
            'y': sc.arange('y', 3.0, unit=sc.units.m),
            'aux': sc.arange('y', 3),
        },
    )

    d2 = sc.Dataset(
        data={
            'a': sc.array(dims=['x', 'y'], values=np.random.rand(2, 3)),
        },
        coords={
            'x': sc.arange('x', 2.0, unit=sc.units.m),
            'y': sc.arange('y', 3.0, unit=sc.units.m),
            'aux': sc.arange('y', 3),
        },
    )

    d3 = sc.Dataset({'b': d1['a']})
    assert sc.identical(d3['b'].data, d1['a'].data)
    assert d3['b'].coords == d1['a'].coords
    d1['a'] = d2['a']
    d1['c'] = d2['a']
    assert sc.identical(d2['a'].data, d1['a'].data)
    assert sc.identical(d2['a'].data, d1['c'].data)

    d = sc.Dataset(
        {'a': sc.array(dims=['row'], values=np.arange(10.0), variances=np.arange(10.0))}
    )
    d['b'] = sc.arange('row', 10.0, 20.0)
    d.coords['row'] = sc.arange('row', 10.0)
    d1 = d['row', 0:1]
    d2 = sc.Dataset(data={'a': d1['a'].data}, coords={'row': d1['a'].coords['row']})
    d2['b'] = d1['b']
    expected = sc.Dataset(
        {
            'a': sc.array(
                dims=['row'], values=np.arange(1.0), variances=np.arange(1.0)
            ),
            'b': sc.arange('row', 10.0, 11.0),
        },
        coords={'row': sc.arange('row', 1.0)},
    )
    assert sc.identical(d2, expected)


def test_add_sum_of_columns() -> None:
    d = sc.Dataset(
        data={
            'a': sc.Variable(dims=['x'], values=np.arange(10.0)),
            'b': sc.Variable(dims=['x'], values=np.ones(10)),
        }
    )
    d['sum'] = d['a'] + d['b']
    d['a'] += d['b']
    assert sc.identical(d['sum'], d['a'])


def test_name() -> None:
    d = sc.Dataset(
        data={
            'a': sc.Variable(dims=['x'], values=np.arange(10.0)),
            'b': sc.Variable(dims=['x'], values=np.ones(10)),
        },
        coords={'x': sc.Variable(dims=['x'], values=np.arange(10))},
    )
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
        data={
            'a': sc.array(dims=[dim1, dim2], values=np.random.rand(2, 3)),
        },
        coords={
            dim1: sc.arange(dim1, 2.0, unit=sc.units.m),
            dim2: sc.arange(dim2, 3.0, unit=sc.units.m),
            'aux': sc.array(dims=[dim2], values=np.random.rand(3)),
        },
    )


def test_dataset_view_set_variance() -> None:
    d = make_simple_dataset()
    variances = np.arange(6).reshape(2, 3)
    assert d['a'].variances is None
    d['a'].variances = variances
    assert d['a'].variances is not None
    np.testing.assert_array_equal(d['a'].variances, variances)
    d['a'].variances = None
    assert d['a'].variances is None


def test_sort() -> None:
    d = sc.Dataset(
        data={
            'a': sc.array(dims=['x', 'y'], values=np.arange(6).reshape(2, 3)),
        },
        coords={
            'x': sc.arange('x', 2.0, unit=sc.units.m),
            'y': sc.arange('y', 3.0, unit=sc.units.m),
            'aux': sc.arange('x', 2.0),
        },
    )
    expected = sc.Dataset(
        data={
            'a': sc.array(
                dims=['x', 'y'], values=np.flip(np.arange(6).reshape(2, 3), axis=0)
            ),
        },
        coords={
            'x': sc.array(dims=['x'], values=[1.0, 0.0], unit=sc.units.m),
            'y': sc.arange('y', 3.0, unit=sc.units.m),
            'aux': sc.array(dims=['x'], values=[1.0, 0.0]),
        },
    )
    assert sc.identical(sc.sort(d, sc.array(dims=['x'], values=['b', 'a'])), expected)


def test_rename_dims() -> None:
    d = make_simple_dataset('x', 'y', seed=0)
    original = d.copy()
    renamed = d.rename_dims({'y': 'z'})
    assert sc.identical(d, original)
    renamed.coords['z'] = renamed.coords['y']
    del renamed.coords['y']
    assert sc.identical(renamed, make_simple_dataset('x', 'z', seed=0))
    renamed = renamed.rename_dims({'x': 'y', 'z': 'x'})
    renamed.coords['y'] = renamed.coords['x']
    renamed.coords['x'] = renamed.coords['z']
    del renamed.coords['z']
    assert sc.identical(renamed, make_simple_dataset('y', 'x', seed=0))


def test_rename() -> None:
    d = make_simple_dataset('x', 'y', seed=0)
    original = d.copy()
    renamed = d.rename({'y': 'z'})
    assert sc.identical(d, original)
    assert sc.identical(renamed, make_simple_dataset('x', 'z', seed=0))
    renamed = renamed.rename({'x': 'y', 'z': 'x'})
    assert sc.identical(renamed, make_simple_dataset('y', 'x', seed=0))


def test_rename_kwargs() -> None:
    d = make_simple_dataset('x', 'y', seed=0)
    renamed = d.rename(y='z')
    assert sc.identical(renamed, make_simple_dataset('x', 'z', seed=0))
    renamed = renamed.rename(x='y', z='x')
    assert sc.identical(renamed, make_simple_dataset('y', 'x', seed=0))


def test_replace() -> None:
    v1 = sc.Variable(dims=['x'], values=np.array([1, 2, 3]))
    d = sc.Dataset(data={'a': v1})
    assert sc.identical(d['a'].data, v1)
    v2 = sc.Variable(dims=['x'], values=np.array([4, 5, 6]))
    d['a'].data = v2
    assert sc.identical(d['a'].data, v2)


def test_rebin() -> None:
    dataset = sc.Dataset(
        {
            'data': sc.array(
                dims=['x'], values=np.array(10 * [1.0]), unit=sc.units.counts
            )
        }
    )
    dataset.coords['x'] = sc.Variable(dims=['x'], values=np.arange(11.0))
    new_coord = sc.Variable(dims=['x'], values=np.arange(0.0, 11, 2))
    dataset = dataset.rebin(x=new_coord)
    np.testing.assert_array_equal(dataset['data'].values, np.array(5 * [2]))
    np.testing.assert_array_equal(dataset.coords['x'].values, np.arange(0, 11, 2))


def _is_copy_of(orig, copy):
    assert sc.identical(orig, copy)
    assert not id(orig) == id(copy)
    orig += 1
    assert sc.identical(orig, copy)


def _is_deep_copy_of(orig, copy):
    assert sc.identical(orig, copy)
    assert not id(orig) == id(copy)
    orig += 1
    assert not sc.identical(orig, copy)


def test_copy() -> None:
    import copy

    a = sc.Dataset({'x': sc.scalar(1)})
    _is_copy_of(a, a.copy(deep=False))
    _is_deep_copy_of(a, a.copy())
    _is_copy_of(a, copy.copy(a))
    _is_deep_copy_of(a, copy.deepcopy(a))


def test_iteration() -> None:
    var = sc.scalar(1)
    d = sc.Dataset(data={'a': var, 'b': var})
    expected = ['a', 'b']
    for k in d:
        assert k in expected


def test_is_edges() -> None:
    da = sc.DataArray(
        sc.zeros(sizes={'a': 2, 'b': 3}),
        coords={'coord': sc.zeros(sizes={'a': 3})},
        masks={'mask': sc.zeros(sizes={'b': 3})},
    )
    assert da.coords.is_edges('coord')
    assert da.coords.is_edges('coord', 'a')
    assert not da.masks.is_edges('mask', 'b')


def test_is_edges_raises_for_2d_suggests_using_second_arg() -> None:
    da = sc.DataArray(
        sc.zeros(sizes={'a': 2, 'b': 3}),
        coords={'coord': sc.zeros(sizes={'a': 3, 'b': 3})},
    )
    with pytest.raises(sc.DimensionError, match=r'.*Use the second argument.*'):
        assert da.coords.is_edges('coord')


def test_drop_coords() -> None:
    coord0 = sc.linspace('x', start=0.2, stop=1.61, num=4)
    coord1 = sc.linspace('y', start=1, stop=4, num=3)
    coord2 = sc.linspace('z', start=-12, stop=0, num=5)
    data = sc.array(dims=['x', 'y', 'z'], values=np.random.rand(4, 3, 5))
    ds = sc.Dataset(
        data={'data': data},
        coords={'coord0': coord0, 'coord1': coord1, 'coord2': coord2},
    )

    assert 'coord0' not in ds.drop_coords('coord0').coords
    assert 'coord1' in ds.drop_coords('coord0').coords
    assert 'coord2' in ds.drop_coords('coord0').coords
    assert 'coord0' in ds.drop_coords(['coord1', 'coord2']).coords
    assert 'coord1' not in ds.drop_coords(['coord1', 'coord2']).coords
    assert 'coord2' not in ds.drop_coords(['coord1', 'coord2']).coords
    expected_ds = sc.Dataset(
        data={'data': data},
        coords={'coord0': coord0, 'coord2': coord2},
    )
    assert sc.identical(ds.drop_coords('coord1'), expected_ds)


def test_assign_coords() -> None:
    data = sc.array(dims=['x', 'y', 'z'], values=np.random.rand(4, 3, 5))
    ds_o = sc.Dataset(data={'data': data})

    coord0 = sc.linspace('x', start=0.2, stop=1.61, num=4)
    coord1 = sc.linspace('y', start=1, stop=4, num=3)
    ds_n = ds_o.assign_coords({'coord0': coord0, 'coord1': coord1})
    assert sc.identical(ds_o, sc.Dataset(data={'data': data}))
    assert sc.identical(
        ds_n,
        sc.Dataset(
            data={'data': data},
            coords={'coord0': coord0, 'coord1': coord1},
        ),
    )
    assert sc.identical(
        ds_n['data'],
        sc.DataArray(data=data, coords={'coord0': coord0, 'coord1': coord1}),
    )


def test_assign_coords_kwargs() -> None:
    data = sc.array(dims=['x', 'y', 'z'], values=np.random.rand(4, 3, 5))
    ds_o = sc.Dataset(data={'data': data})

    coord0 = sc.linspace('x', start=0.2, stop=1.61, num=4)
    coord1 = sc.linspace('y', start=1, stop=4, num=3)
    ds_n = ds_o.assign_coords(coord0=coord0, coord1=coord1)
    assert sc.identical(ds_o, sc.Dataset(data={'data': data}))
    assert sc.identical(
        ds_n,
        sc.Dataset(
            data={'data': data},
            coords={'coord0': coord0, 'coord1': coord1},
        ),
    )
    assert sc.identical(
        ds_n['data'],
        sc.DataArray(data=data, coords={'coord0': coord0, 'coord1': coord1}),
    )


def test_assign_coords_overlapping_names() -> None:
    data = sc.array(dims=['x', 'y', 'z'], values=np.random.rand(4, 3, 5))
    ds = sc.Dataset(data={'data0': data})
    coord0 = sc.linspace('x', start=0.2, stop=1.61, num=4)

    with pytest.raises(TypeError, match='names .* distinct'):
        ds.assign_coords({'coord0': coord0}, coord0=coord0)


def test_assign_update_coords() -> None:
    data = sc.array(dims=['x', 'y', 'z'], values=np.random.rand(4, 3, 5))
    coord0_o = sc.linspace('x', start=0.2, stop=1.61, num=4)
    coord1_o = sc.linspace('y', start=1, stop=4, num=3)
    ds_o = sc.Dataset(
        data={'data': data},
        coords={'coord0': coord0_o, 'coord1': coord1_o},
    )

    coord0_n = sc.linspace('x', start=0.2, stop=1.61, num=4)
    coord1_n = sc.linspace('y', start=1, stop=4, num=3)
    ds_n = ds_o.assign_coords({'coord0': coord0_n, 'coord1': coord1_n})

    assert sc.identical(
        ds_o,
        sc.Dataset(
            data={'data': data},
            coords={'coord0': coord0_o, 'coord1': coord1_o},
        ),
    )
    assert sc.identical(
        ds_n,
        sc.Dataset(
            data={'data': data},
            coords={'coord0': coord0_n, 'coord1': coord1_n},
        ),
    )
    assert sc.identical(
        ds_n['data'],
        sc.DataArray(data=data, coords={'coord0': coord0_n, 'coord1': coord1_n}),
    )
