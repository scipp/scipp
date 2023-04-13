# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import numpy as np
import pytest

import scipp as sc


def test_init_default():
    d = sc.Dataset()
    assert len(d) == 0
    assert len(d.coords) == 0


def test_init_dict_of_variables():
    d = sc.Dataset({'a': sc.arange('x', 5), 'b': sc.arange('y', 10, unit='m')})
    assert sc.identical(d['a'], sc.DataArray(sc.arange('x', 5)))
    assert sc.identical(d['b'], sc.DataArray(sc.arange('y', 10, unit='m')))


def test_init_dict_of_dataarrays():
    d = sc.Dataset(
        {
            'a': sc.DataArray(sc.arange('x', 5)),
            'b': sc.DataArray(
                sc.arange('y', 10, unit='m'), coords={'y': sc.arange('y', 10)}
            ),
        }
    )
    assert sc.identical(d['a'], sc.DataArray(sc.arange('x', 5)))
    assert sc.identical(
        d['b'],
        sc.DataArray(sc.arange('y', 10, unit='m'), coords={'y': sc.arange('y', 10)}),
    )


def test_init_dict_of_dataarray_and_variable():
    d = sc.Dataset(
        {
            'a': sc.arange('x', 5),
            'b': sc.DataArray(
                sc.arange('y', 10, unit='m'), coords={'y': sc.arange('y', 10)}
            ),
        }
    )
    assert sc.identical(d['a'], sc.DataArray(sc.arange('x', 5)))
    assert sc.identical(
        d['b'],
        sc.DataArray(sc.arange('y', 10, unit='m'), coords={'y': sc.arange('y', 10)}),
    )


def test_init_data_from_dataset():
    d1 = sc.Dataset(
        {
            'a': sc.arange('x', 5),
            'b': sc.DataArray(
                sc.arange('y', 10, unit='m'), coords={'y': sc.arange('y', 10)}
            ),
        }
    )
    d2 = sc.Dataset(d1)
    assert sc.identical(d2['a'], sc.DataArray(sc.arange('x', 5)))
    assert sc.identical(
        d2['b'],
        sc.DataArray(sc.arange('y', 10, unit='m'), coords={'y': sc.arange('y', 10)}),
    )


def test_init_iterator_of_tuples():
    d = sc.Dataset({'a': sc.arange('x', 5), 'b': sc.arange('y', 10, unit='m')}.items())
    assert sc.identical(d['a'], sc.DataArray(sc.arange('x', 5)))
    assert sc.identical(d['b'], sc.DataArray(sc.arange('y', 10, unit='m')))


def test_init_extra_coords_from_dict():
    d = sc.Dataset(
        {'a': sc.arange('x', 5), 'b': sc.arange('y', 10, unit='m')},
        coords={'x': sc.arange('x', 5), 'y': sc.arange('y', 11)},
    )
    assert sc.identical(
        d['a'], sc.DataArray(sc.arange('x', 5), coords={'x': sc.arange('x', 5)})
    )
    assert sc.identical(
        d['b'],
        sc.DataArray(sc.arange('y', 10, unit='m'), coords={'y': sc.arange('y', 11)}),
    )


def test_init_extra_coords_from_coords_object():
    da = sc.DataArray(
        sc.arange('x', 5), coords={'x': sc.arange('x', 5), 'z': sc.scalar('zz')}
    )
    d = sc.Dataset(
        {'a': sc.arange('x', 5), 'b': sc.arange('y', 10, unit='m')}, coords=da.coords
    )
    assert sc.identical(
        d['a'],
        sc.DataArray(
            sc.arange('x', 5), coords={'x': sc.arange('x', 5), 'z': sc.scalar('zz')}
        ),
    )
    assert sc.identical(
        d['b'],
        sc.DataArray(sc.arange('y', 10, unit='m'), coords={'z': sc.scalar('zz')}),
    )


def test_init_extra_coords_from_iterator_of_tuples():
    d = sc.Dataset(
        {'a': sc.arange('x', 5), 'b': sc.arange('y', 10, unit='m')},
        coords={'x': sc.arange('x', 5), 'y': sc.arange('y', 11)}.items(),
    )
    assert sc.identical(
        d['a'], sc.DataArray(sc.arange('x', 5), coords={'x': sc.arange('x', 5)})
    )
    assert sc.identical(
        d['b'],
        sc.DataArray(sc.arange('y', 10, unit='m'), coords={'y': sc.arange('y', 11)}),
    )


def test_shape():
    a = sc.scalar(1)
    d = sc.Dataset(data={'a': a})
    assert d.shape == ()
    a = sc.empty(dims=['x'], shape=[2])
    b = sc.empty(dims=['y', 'z'], shape=[3, 4])
    d = sc.Dataset(data={'a': a, 'b': b})
    assert not bool(set(d.shape) - {2, 3, 4})


def test_sizes():
    d = sc.Dataset(data={'a': sc.scalar(value=1)})
    assert d.sizes == {}
    a = sc.empty(dims=['x'], shape=[2])
    b = sc.empty(dims=['y', 'z'], shape=[3, 4])
    d = sc.Dataset(data={'a': a, 'b': b})
    assert d.sizes == {'x': 2, 'y': 3, 'z': 4}


def test_create_empty():
    d = sc.Dataset()
    assert len(d) == 0
    assert len(d.coords) == 0
    assert len(d.dims) == 0


def test_create():
    x = sc.Variable(dims=['x'], values=np.arange(3))
    y = sc.Variable(dims=['y'], values=np.arange(4))
    xy = sc.Variable(dims=['x', 'y'], values=np.arange(12).reshape(3, 4))
    d = sc.Dataset(data={'xy': xy, 'x': x}, coords={'x': x, 'y': y})
    assert len(d) == 2
    assert sc.identical(d.coords['x'], x)
    assert sc.identical(d.coords['y'], y)
    assert sc.identical(d['xy'].data, xy)
    assert sc.identical(d['x'].data, x)
    assert set(d.dims) == {'y', 'x'}


def test_create_from_data_array():
    var = sc.Variable(dims=['x'], values=np.arange(4))
    da = sc.DataArray(var, coords={'x': var, 'aux': var})
    d = sc.Dataset(data={da.name: da})
    assert sc.identical(d[''], da)


def test_create_from_data_arrays():
    var1 = sc.Variable(dims=['x'], values=np.arange(4))
    var2 = sc.Variable(dims=['x'], values=np.ones(4))
    da1 = sc.DataArray(var1, coords={'x': var1, 'aux': var2})
    da2 = sc.DataArray(var1, coords={'x': var1, 'aux': var2})
    d = sc.Dataset()
    d['a'] = da1
    d['b'] = da2
    assert sc.identical(
        d, sc.Dataset(data={'a': var1, 'b': var1}, coords={'x': var1, 'aux': var2})
    )


def test_create_from_data_array_and_variable_mix():
    var_1 = sc.Variable(dims=['x'], values=np.arange(4))
    var_2 = sc.Variable(dims=['x'], values=np.arange(4))
    da = sc.DataArray(data=var_1, coords={'x': var_1, 'aux': var_1})
    d = sc.Dataset({'array': da, 'variable': var_2})
    assert sc.identical(d['array'], da)
    assert sc.identical(d['variable'].data, var_2)


def test_create_with_data_array_and_additional_coords():
    var = sc.Variable(dims=['x'], values=np.arange(4))
    coord = sc.Variable(dims=['x'], values=np.arange(4))
    da = sc.DataArray(data=var, coords={'x': var, 'aux': var})
    d = sc.Dataset(data={'array': da}, coords={'y': coord})
    da.coords['y'] = coord
    assert sc.identical(d['array'], da)
    assert sc.identical(d.coords['y'], coord)
    assert sc.identical(d.coords['x'], var)
    assert sc.identical(d.coords['aux'], var)


def test_clear():
    d = sc.Dataset()
    d['a'] = sc.Variable(dims=['x'], values=np.arange(3))
    assert 'a' in d
    d.clear()
    assert len(d) == 0


def test_setitem():
    d = sc.Dataset()
    d['a'] = sc.scalar(1.0)
    assert len(d) == 1
    assert sc.identical(d['a'].data, sc.scalar(1.0))
    assert len(d.coords) == 0


def test_del_item():
    d = sc.Dataset()
    d['a'] = sc.scalar(1.0)
    assert 'a' in d
    del d['a']
    assert 'a' not in d


def test_del_item_missing():
    d = sc.Dataset()
    with pytest.raises(KeyError):
        del d['not an item']


def test_update_from_dict_adds_items():
    a = sc.DataArray(sc.scalar(1.0))
    b = sc.DataArray(sc.scalar(2.0))
    b2 = sc.DataArray(sc.scalar(3.0))
    c = sc.DataArray(sc.scalar(4.0))
    d = sc.Dataset({'a': a, 'b': b})
    d.update({'b': b2, 'c': c})
    assert sc.identical(d['a'], a)
    assert sc.identical(d['b'], b2)
    assert sc.identical(d['c'], c)


def test_update_from_dataset_adds_items():
    a = sc.DataArray(sc.scalar(1.0))
    b = sc.DataArray(sc.scalar(2.0))
    b2 = sc.DataArray(sc.scalar(3.0))
    c = sc.DataArray(sc.scalar(4.0))
    d = sc.Dataset({'a': a, 'b': b})
    d.update(sc.Dataset({'b': b2, 'c': c}))
    assert sc.identical(d['a'], a)
    assert sc.identical(d['b'], b2)
    assert sc.identical(d['c'], c)


def test_update_from_sequence_of_tuples_adds_items():
    a = sc.DataArray(sc.scalar(1.0))
    b = sc.DataArray(sc.scalar(2.0))
    b2 = sc.DataArray(sc.scalar(3.0))
    c = sc.DataArray(sc.scalar(4.0))
    d = sc.Dataset({'a': a, 'b': b})
    d.update([('b', b2), ('c', c)])
    assert sc.identical(d['a'], a)
    assert sc.identical(d['b'], b2)
    assert sc.identical(d['c'], c)


def test_update_from_iterable_of_tuples_adds_items():
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


def test_update_from_kwargs_adds_items():
    a = sc.DataArray(sc.scalar(1.0))
    b = sc.DataArray(sc.scalar(2.0))
    b2 = sc.DataArray(sc.scalar(3.0))
    c = sc.DataArray(sc.scalar(4.0))
    d = sc.Dataset({'a': a, 'b': b})
    d.update(b=b2, c=c)
    assert sc.identical(d['a'], a)
    assert sc.identical(d['b'], b2)
    assert sc.identical(d['c'], c)


def test_update_from_kwargs_overwrites_other_dict():
    a = sc.DataArray(sc.scalar(1.0))
    b = sc.DataArray(sc.scalar(2.0))
    b2 = sc.DataArray(sc.scalar(3.0))
    d = sc.Dataset({'a': a})
    d.update({'b': b}, b=b2)
    assert sc.identical(d['a'], a)
    assert sc.identical(d['b'], b2)


def test_ipython_key_completion():
    var = sc.Variable(dims=['x'], values=np.arange(4))
    da = sc.DataArray(data=var, coords={'x': var, 'aux': var})
    ds = sc.Dataset(data={'array 1': da, 'array 2': da})
    assert set(ds._ipython_key_completions_()) == set(ds.keys())


def test_slice_item():
    d = sc.Dataset(coords={'x': sc.Variable(dims=['x'], values=np.arange(4, 8))})
    d['a'] = sc.Variable(dims=['x'], values=np.arange(4))
    assert sc.identical(
        d['a']['x', 2:4].data, sc.Variable(dims=['x'], values=np.arange(2, 4))
    )
    assert sc.identical(
        d['a']['x', 2:4].coords['x'], sc.Variable(dims=['x'], values=np.arange(6, 8))
    )


def test_set_item_slice_from_numpy():
    d = sc.Dataset(coords={'x': sc.Variable(dims=['x'], values=np.arange(4, 8))})
    d['a'] = sc.Variable(dims=['x'], values=np.arange(4))
    d['a']['x', 2:4] = np.arange(2)
    assert sc.identical(
        d['a'].data, sc.Variable(dims=['x'], values=np.array([0, 1, 0, 1]))
    )


def test_set_item_slice_with_variances_from_numpy():
    d = sc.Dataset(coords={'x': sc.Variable(dims=['x'], values=np.arange(4, 8))})
    d['a'] = sc.Variable(dims=['x'], values=np.arange(4.0), variances=np.arange(4.0))
    d['a']['x', 2:4].values = np.arange(2)
    d['a']['x', 2:4].variances = np.arange(2, 4)
    assert np.array_equal(d['a'].values, np.array([0.0, 1.0, 0.0, 1.0]))
    assert np.array_equal(d['a'].variances, np.array([0.0, 1.0, 2.0, 3.0]))


def test_iadd_slice():
    d = sc.Dataset(coords={'x': sc.Variable(dims=['x'], values=np.arange(4, 8))})
    d['a'] = sc.Variable(dims=['x'], values=np.arange(4))
    d['a']['x', 1] += d['a']['x', 2]
    assert sc.identical(
        d['a'].data, sc.Variable(dims=['x'], values=np.array([0, 3, 2, 3]))
    )


def test_iadd_range():
    d = sc.Dataset(coords={'x': sc.Variable(dims=['x'], values=np.arange(4, 8))})
    d['a'] = sc.Variable(dims=['x'], values=np.arange(4))
    with pytest.raises(RuntimeError):
        d['a']['x', 2:4] += d['a']['x', 1:3]
    d['a']['x', 2:4] += d['a']['x', 2:4]
    assert sc.identical(
        d['a'].data, sc.Variable(dims=['x'], values=np.array([0, 1, 4, 6]))
    )


def test_contains():
    d = sc.Dataset()
    assert 'a' not in d
    d['a'] = sc.scalar(1.0)
    assert 'a' in d
    assert 'b' not in d
    d['b'] = sc.scalar(1.0)
    assert 'a' in d
    assert 'b' in d


def test_slice():
    d = sc.Dataset(
        data={
            'a': sc.Variable(dims=['x'], values=np.arange(10.0)),
            'b': sc.scalar(1.0),
        },
        coords={'x': sc.Variable(dims=['x'], values=np.arange(10.0))},
    )
    expected = sc.Dataset(
        data={
            'a': sc.DataArray(1.0 * sc.units.one, attrs={'x': 1.0 * sc.units.one}),
            'b': sc.scalar(1.0),
        }
    )
    assert sc.identical(d['x', 1], expected)


def test_chained_slicing():
    x = sc.Variable(dims=['x'], values=np.arange(11.0))
    y = sc.Variable(dims=['y'], values=np.arange(11.0))
    z = sc.Variable(dims=['z'], values=np.arange(11.0))
    d = sc.Dataset(
        data={
            'a': sc.Variable(
                dims=['z', 'y', 'x'], values=np.arange(1000.0).reshape(10, 10, 10)
            ),
            'b': sc.Variable(
                dims=['x', 'z'], values=np.arange(0.0, 10.0, 0.1).reshape(10, 10)
            ),
        },
        coords={'x': x, 'y': y, 'z': z},
    )

    expected = sc.Dataset()
    expected['a'] = sc.Variable(dims=['y'], values=np.arange(501.0, 600.0, 10.0))
    expected['b'] = sc.scalar(1.5)
    expected['a'].attrs['x'] = x['x', 1:3]
    expected['b'].attrs['x'] = x['x', 1:3]
    expected['a'].attrs['z'] = z['z', 5:7]
    expected['b'].attrs['z'] = z['z', 5:7]
    expected.coords['y'] = sc.Variable(dims=['y'], values=np.arange(11.0))

    assert sc.identical(d['x', 1]['z', 5], expected)


def test_dataset_merge():
    a = sc.Dataset(data={'d1': sc.Variable(dims=['x'], values=np.array([1, 2, 3]))})
    b = sc.Dataset(data={'d2': sc.Variable(dims=['x'], values=np.array([4, 5, 6]))})
    c = sc.merge(a, b)
    assert sc.identical(a['d1'], c['d1'])
    assert sc.identical(b['d2'], c['d2'])


def test_dataset_concat():
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


def test_dataset_set_data():
    d1 = sc.Dataset(
        data={
            'a': sc.Variable(dims=['x', 'y'], values=np.random.rand(2, 3)),
            'b': sc.scalar(1.0),
        },
        coords={
            'x': sc.Variable(dims=['x'], values=np.arange(2.0), unit=sc.units.m),
            'y': sc.Variable(dims=['y'], values=np.arange(3.0), unit=sc.units.m),
            'aux': sc.Variable(dims=['y'], values=np.arange(3)),
        },
    )

    d2 = sc.Dataset(
        data={
            'a': sc.Variable(dims=['x', 'y'], values=np.random.rand(2, 3)),
            'b': sc.scalar(1.0),
        },
        coords={
            'x': sc.Variable(dims=['x'], values=np.arange(2.0), unit=sc.units.m),
            'y': sc.Variable(dims=['y'], values=np.arange(3.0), unit=sc.units.m),
            'aux': sc.Variable(dims=['y'], values=np.arange(3)),
        },
    )

    d3 = sc.Dataset()
    d3['b'] = d1['a']
    assert sc.identical(d3['b'].data, d1['a'].data)
    assert d3['b'].coords == d1['a'].coords
    d1['a'] = d2['a']
    d1['c'] = d2['a']
    assert sc.identical(d2['a'].data, d1['a'].data)
    assert sc.identical(d2['a'].data, d1['c'].data)

    d = sc.Dataset()
    d['a'] = sc.Variable(
        dims=['row'], values=np.arange(10.0), variances=np.arange(10.0)
    )
    d['b'] = sc.Variable(dims=['row'], values=np.arange(10.0, 20.0))
    d.coords['row'] = sc.Variable(dims=['row'], values=np.arange(10.0))
    d1 = d['row', 0:1]
    d2 = sc.Dataset(data={'a': d1['a'].data}, coords={'row': d1['a'].coords['row']})
    d2['b'] = d1['b']
    expected = sc.Dataset()
    expected['a'] = sc.Variable(
        dims=['row'], values=np.arange(1.0), variances=np.arange(1.0)
    )
    expected['b'] = sc.Variable(dims=['row'], values=np.arange(10.0, 11.0))
    expected.coords['row'] = sc.Variable(dims=['row'], values=np.arange(1.0))
    assert sc.identical(d2, expected)


def test_add_sum_of_columns():
    d = sc.Dataset(
        data={
            'a': sc.Variable(dims=['x'], values=np.arange(10.0)),
            'b': sc.Variable(dims=['x'], values=np.ones(10)),
        }
    )
    d['sum'] = d['a'] + d['b']
    d['a'] += d['b']
    assert sc.identical(d['sum'], d['a'])


def test_name():
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
            'a': sc.Variable(dims=[dim1, dim2], values=np.random.rand(2, 3)),
            'b': sc.scalar(1.0),
        },
        coords={
            dim1: sc.Variable(dims=[dim1], values=np.arange(2.0), unit=sc.units.m),
            dim2: sc.Variable(dims=[dim2], values=np.arange(3.0), unit=sc.units.m),
            'aux': sc.Variable(dims=[dim2], values=np.random.rand(3)),
        },
    )


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
        data={
            'a': sc.Variable(dims=['x', 'y'], values=np.arange(6).reshape(2, 3)),
            'b': sc.Variable(dims=['x'], values=['b', 'a']),
        },
        coords={
            'x': sc.Variable(dims=['x'], values=np.arange(2.0), unit=sc.units.m),
            'y': sc.Variable(dims=['y'], values=np.arange(3.0), unit=sc.units.m),
            'aux': sc.Variable(dims=['x'], values=np.arange(2.0)),
        },
    )
    expected = sc.Dataset(
        data={
            'a': sc.Variable(
                dims=['x', 'y'], values=np.flip(np.arange(6).reshape(2, 3), axis=0)
            ),
            'b': sc.Variable(dims=['x'], values=['a', 'b']),
        },
        coords={
            'x': sc.Variable(dims=['x'], values=[1.0, 0.0], unit=sc.units.m),
            'y': sc.Variable(dims=['y'], values=np.arange(3.0), unit=sc.units.m),
            'aux': sc.Variable(dims=['x'], values=[1.0, 0.0]),
        },
    )
    assert sc.identical(sc.sort(d, d['b'].data), expected)


def test_rename_dims():
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


def test_rename():
    d = make_simple_dataset('x', 'y', seed=0)
    original = d.copy()
    renamed = d.rename({'y': 'z'})
    assert sc.identical(d, original)
    assert sc.identical(renamed, make_simple_dataset('x', 'z', seed=0))
    renamed = renamed.rename({'x': 'y', 'z': 'x'})
    assert sc.identical(renamed, make_simple_dataset('y', 'x', seed=0))


def test_rename_intersection_of_dims():
    d = make_simple_dataset('x', 'y', seed=0)
    d['c'] = sc.Variable(dims=['time', 'y'], values=np.random.rand(4, 3))
    renamed = d.rename({'x': 'u', 'time': 'v'})
    expected = make_simple_dataset('u', 'y', seed=0)
    expected['c'] = sc.Variable(dims=['v', 'y'], values=np.random.rand(4, 3))
    assert sc.identical(renamed, expected)


def test_rename_coords_only():
    d = sc.Dataset(
        coords={
            'x': sc.arange('x', 5.0),
            'y': sc.arange('y', 6.0),
            'z': sc.arange('z', 7.0),
        }
    )
    expected = sc.Dataset(
        coords={
            'u': sc.arange('u', 5.0),
            'y': sc.arange('y', 6.0),
            'z': sc.arange('z', 7.0),
        }
    )
    assert sc.identical(d.rename({'x': 'u'}), expected)


def test_rename_dataset_with_coords_not_belonging_to_any_item():
    d = sc.Dataset(
        data={'a': sc.arange('x', 5.0)},
        coords={
            'x': sc.arange('x', 5.0),
            'y': sc.arange('y', 6.0),
            'z': sc.arange('z', 7.0),
        },
    )
    expected = sc.Dataset(
        data={'a': sc.arange('u', 5.0)},
        coords={
            'u': sc.arange('u', 5.0),
            'y': sc.arange('y', 6.0),
            'z': sc.arange('z', 7.0),
        },
    )
    assert sc.identical(d.rename({'x': 'u'}), expected)


def test_rename_kwargs():
    d = make_simple_dataset('x', 'y', seed=0)
    renamed = d.rename(y='z')
    assert sc.identical(renamed, make_simple_dataset('x', 'z', seed=0))
    renamed = renamed.rename(x='y', z='x')
    assert sc.identical(renamed, make_simple_dataset('y', 'x', seed=0))


def test_replace():
    v1 = sc.Variable(dims=['x'], values=np.array([1, 2, 3]))
    d = sc.Dataset(data={'a': v1})
    assert sc.identical(d['a'].data, v1)
    v2 = sc.Variable(dims=['x'], values=np.array([4, 5, 6]))
    d['a'].data = v2
    assert sc.identical(d['a'].data, v2)


def test_rebin():
    dataset = sc.Dataset()
    dataset['data'] = sc.Variable(
        dims=['x'], values=np.array(10 * [1.0]), unit=sc.units.counts
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


def test_copy():
    import copy

    a = sc.Dataset()
    a['x'] = sc.scalar(1)
    _is_copy_of(a, a.copy(deep=False))
    _is_deep_copy_of(a, a.copy())
    _is_copy_of(a, copy.copy(a))
    _is_deep_copy_of(a, copy.deepcopy(a))


def test_correct_temporaries():
    N = 6
    M = 4
    d1 = sc.Dataset()
    d1['x'] = sc.Variable(dims=['x'], values=np.arange(N).astype(np.float64))
    d1['y'] = sc.Variable(dims=['y'], values=np.arange(M).astype(np.float64))
    arr1 = np.arange(N * M).reshape(N, M).astype(np.float64) + 1
    d1['A'] = sc.Variable(dims=['x', 'y'], values=arr1)
    d1 = d1['x', 1:2]
    assert d1['A'].values.tolist() == [[5.0, 6.0, 7.0, 8.0]]
    d1 = d1['y', 2:3]
    assert list(d1['A'].values) == [7]


def test_iteration():
    var = sc.scalar(1)
    d = sc.Dataset(data={'a': var, 'b': var})
    expected = ['a', 'b']
    for k in d:
        assert k in expected


def test_many_independent_dims_are_supported():
    ds = sc.Dataset()
    for i in range(100):
        dim = f'dim{i}'
        ds[dim] = sc.linspace(dim=dim, start=0, stop=1, num=i)
    assert 'dim45' in ds
    assert sc.identical(ds.copy(), ds)
    ds + ds


def test_is_edges():
    da = sc.DataArray(
        sc.zeros(sizes={'a': 2, 'b': 3}),
        coords={'coord': sc.zeros(sizes={'a': 3})},
        attrs={'attr': sc.zeros(sizes={'a': 2, 'b': 4})},
        masks={'mask': sc.zeros(sizes={'b': 3})},
    )
    assert da.coords.is_edges('coord')
    assert da.coords.is_edges('coord', 'a')
    assert da.meta.is_edges('coord')
    assert not da.attrs.is_edges('attr', 'a')
    assert da.attrs.is_edges('attr', 'b')
    assert not da.masks.is_edges('mask', 'b')


def test_drop_coords():
    coord0 = sc.linspace('x', start=0.2, stop=1.61, num=4)
    coord1 = sc.linspace('y', start=1, stop=4, num=3)
    coord2 = sc.linspace('z', start=-12, stop=0, num=5)
    data0 = sc.array(dims=['x', 'y', 'z'], values=np.random.rand(4, 3, 5))
    data1 = sc.array(dims=['x'], values=np.random.rand(4))
    ds = sc.Dataset(
        data={'data0': data0, 'data1': data1},
        coords={'coord0': coord0, 'coord1': coord1, 'coord2': coord2},
    )

    assert 'coord0' not in ds.drop_coords('coord0').coords
    assert 'coord1' in ds.drop_coords('coord0').coords
    assert 'coord2' in ds.drop_coords('coord0').coords
    assert 'coord0' in ds.drop_coords(['coord1', 'coord2']).coords
    assert 'coord1' not in ds.drop_coords(['coord1', 'coord2']).coords
    assert 'coord2' not in ds.drop_coords(['coord1', 'coord2']).coords
    expected_ds = sc.Dataset(
        data={'data0': data0, 'data1': data1},
        coords={'coord0': coord0, 'coord2': coord2},
    )
    assert sc.identical(ds.drop_coords('coord1'), expected_ds)


def test_assign_coords():
    data0 = sc.array(dims=['x', 'y', 'z'], values=np.random.rand(4, 3, 5))
    data1 = sc.array(dims=['x'], values=np.random.rand(4))
    ds_o = sc.Dataset(data={'data0': data0, 'data1': data1})

    coord0 = sc.linspace('x', start=0.2, stop=1.61, num=4)
    coord1 = sc.linspace('y', start=1, stop=4, num=3)
    ds_n = ds_o.assign_coords({'coord0': coord0, 'coord1': coord1})
    assert sc.identical(ds_o, sc.Dataset(data={'data0': data0, 'data1': data1}))
    assert sc.identical(
        ds_n,
        sc.Dataset(
            data={'data0': data0, 'data1': data1},
            coords={'coord0': coord0, 'coord1': coord1},
        ),
    )
    assert sc.identical(
        ds_n['data0'],
        sc.DataArray(data=data0, coords={'coord0': coord0, 'coord1': coord1}),
    )
    assert sc.identical(
        ds_n['data1'], sc.DataArray(data=data1, coords={'coord0': coord0})
    )


def test_assign_coords_kwargs():
    data0 = sc.array(dims=['x', 'y', 'z'], values=np.random.rand(4, 3, 5))
    data1 = sc.array(dims=['x'], values=np.random.rand(4))
    ds_o = sc.Dataset(data={'data0': data0, 'data1': data1})

    coord0 = sc.linspace('x', start=0.2, stop=1.61, num=4)
    coord1 = sc.linspace('y', start=1, stop=4, num=3)
    ds_n = ds_o.assign_coords(coord0=coord0, coord1=coord1)
    assert sc.identical(ds_o, sc.Dataset(data={'data0': data0, 'data1': data1}))
    assert sc.identical(
        ds_n,
        sc.Dataset(
            data={'data0': data0, 'data1': data1},
            coords={'coord0': coord0, 'coord1': coord1},
        ),
    )
    assert sc.identical(
        ds_n['data0'],
        sc.DataArray(data=data0, coords={'coord0': coord0, 'coord1': coord1}),
    )
    assert sc.identical(
        ds_n['data1'], sc.DataArray(data=data1, coords={'coord0': coord0})
    )


def test_assign_coords_overlapping_names():
    data = sc.array(dims=['x', 'y', 'z'], values=np.random.rand(4, 3, 5))
    ds = sc.Dataset(data={'data0': data})
    coord0 = sc.linspace('x', start=0.2, stop=1.61, num=4)

    with pytest.raises(ValueError):
        ds.assign_coords({'coord0': coord0}, coord0=coord0)


def test_assign_update_coords():
    data0 = sc.array(dims=['x', 'y', 'z'], values=np.random.rand(4, 3, 5))
    data1 = sc.array(dims=['x'], values=np.random.rand(4))
    coord0_o = sc.linspace('x', start=0.2, stop=1.61, num=4)
    coord1_o = sc.linspace('y', start=1, stop=4, num=3)
    ds_o = sc.Dataset(
        data={'data0': data0, 'data1': data1},
        coords={'coord0': coord0_o, 'coord1': coord1_o},
    )

    coord0_n = sc.linspace('x', start=0.2, stop=1.61, num=4)
    coord1_n = sc.linspace('y', start=1, stop=4, num=3)
    ds_n = ds_o.assign_coords({'coord0': coord0_n, 'coord1': coord1_n})

    assert sc.identical(
        ds_o,
        sc.Dataset(
            data={'data0': data0, 'data1': data1},
            coords={'coord0': coord0_o, 'coord1': coord1_o},
        ),
    )
    assert sc.identical(
        ds_n,
        sc.Dataset(
            data={'data0': data0, 'data1': data1},
            coords={'coord0': coord0_n, 'coord1': coord1_n},
        ),
    )
    assert sc.identical(
        ds_n['data0'],
        sc.DataArray(data=data0, coords={'coord0': coord0_n, 'coord1': coord1_n}),
    )
    assert sc.identical(
        ds_n['data1'], sc.DataArray(data=data1, coords={'coord0': coord0_n})
    )
