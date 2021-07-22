# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
import pytest
import numpy as np
import scipp as sc


def _make_xy():
    x = sc.linspace(dim='x', unit='m', start=0.1, stop=0.2, num=3)
    y = sc.linspace(dim='y', unit='m', start=0.1, stop=0.3, num=4)
    return sc.DataArray(data=sc.ones(dims=['x', 'y'], shape=[3, 4]),
                        coords={
                            'x': x,
                            'y': y
                        })


def test_rename_2_steps():
    original = _make_xy()
    graph = {'y4': 'y', 'y3': 'y2', 'y2': 'y'}
    da = original.copy().transform_coords(['y3'], graph=graph, include_aliases=True)
    assert da.dims == ['x', 'y3']
    original = original.rename_dims({'y': 'y3'})
    assert sc.identical(da.coords['y3'], original.coords['y'])
    assert sc.identical(da.attrs['y2'], original.coords['y'])
    assert sc.identical(da.attrs['y'], original.coords['y'])

    da = original.copy().transform_coords(['y3'], graph=graph, include_aliases=False)
    assert 'y3' in da.coords  # alias, but requested explicitly
    assert 'y2' not in da.attrs  # alias for y => removed
    assert 'y' in da.attrs


def test_rename_multi_output():
    original = _make_xy()
    da = original.copy().transform_coords(['y3', 'y4'],
                                          graph={
                                              'y4': 'y',
                                              'y3': 'y2',
                                              'y2': 'y'
                                          },
                                          include_aliases=True)
    assert da.dims == ['x', 'y']  # y4 also depends on y so dim not renamed
    assert sc.identical(da.coords['y4'], original.coords['y'])
    assert sc.identical(da.coords['y3'], original.coords['y'])
    assert sc.identical(da.attrs['y2'], original.coords['y'])
    assert sc.identical(da.attrs['y'], original.coords['y'])


def ab(*, a, b):
    return a + b


def bc(*, b, c):
    return b * c


def split(*, a):
    return {'b': a, 'c': 2 * a}


a = sc.arange(dim='a', start=0, stop=4)
b = sc.arange(dim='b', start=2, stop=6)
c = sc.arange(dim='c', start=4, stop=8)


def test_diamond_graph():
    original = sc.DataArray(data=a, coords={'a': a})
    graph = {('b', 'c'): split, 'd': bc}
    da = original.transform_coords(['d'], graph=graph)
    assert sc.identical(da.coords['d'], a * (2 * a))


def test_avoid_consume_of_requested_outputs():
    original = sc.DataArray(data=a, coords={'a': a})
    graph = {('b', 'c'): split, 'ab': ab}
    da = original.transform_coords(['ab', 'b'], graph=graph)
    assert 'b' in da.coords
    # Second requested output must not consume first
    da = original.transform_coords(['b', 'ab'], graph=graph)
    assert 'b' in da.coords


def test_dim_rename_merge_single_dim_coord():
    # *a    b
    #   \  /
    #    *ab
    original = sc.DataArray(data=a, coords={'a': a, 'b': 2 * a})
    da = original.transform_coords(['ab'], graph={'ab': ab})
    assert da.dims == ['ab']
    assert sc.identical(da.coords['ab'], (a + 2 * a).rename_dims({'a': 'ab'}))


def test_dim_rename_split_dim_coord():
    # a   *b    c
    #  \  / \  /
    #   ab   bc
    original = sc.DataArray(data=b, coords={'a': b, 'b': 2 * b, 'c': 4 * b})
    # Only b is dimension-coord, but it is split => no rename of b
    da = original.transform_coords(['ab', 'bc'], graph={'ab': ab, 'bc': bc})
    assert da.dims == ['b']
    assert sc.identical(da.coords['ab'], b + 2 * b)
    assert sc.identical(da.coords['bc'], (2 * b) * (4 * b))


def test_dim_rename_merge_two_dim_coords():
    # *a   *b
    #   \  /
    #    ab
    original = sc.DataArray(data=a + b, coords={'a': a, 'b': b})
    # ab depends on two dimension coords => no rename of a
    da = original.transform_coords(['ab'], graph={'ab': ab})
    assert da.dims == ['a', 'b']
    # Split combined with merge: Ensure the b2 does not cause rename of b
    # which in turn would enable rename of a to ab
    # *a   *b
    #   \  / \
    #    ab   b2
    da = original.transform_coords(['ab', 'b2'], graph={'ab': ab, 'b2': 'b'})
    assert da.dims == ['a', 'b']
    da = original.transform_coords(['b2', 'ab'], graph={'ab': ab, 'b2': 'b'})
    assert da.dims == ['a', 'b']


def test_dim_rename_multi_level_merge():
    # *b   *a
    #  |   / \
    #  |  c   a2
    #   \ |
    #    *bc
    original = sc.DataArray(data=a + b, coords={'a': a, 'b': b})
    da = original.transform_coords(['bc', 'a2'], graph={'bc': bc, 'c': 'a', 'a2': 'a'})
    # a2 prevents conversion of a to c
    # => c is not a dimension coord
    # => bc depends on single dimension coord
    # => rename b to bc
    assert da.dims == ['a', 'bc']


def test_dim_rename_multi_level_merge_multi_output():
    def split_a(*, a):
        return {'a2': a, 'c': a}

    original = sc.DataArray(data=a + b, coords={'a': a, 'b': b})
    da = original.transform_coords(['bc'], graph={'bc': bc, 'c': split_a})
    # Similar to test_dim_rename_multi_level_merge above, but now an implicit
    # intermediate result prevents conversion of a to c
    assert da.dims == ['a', 'bc']


def test_rename_dims_param():
    original = sc.DataArray(data=a, coords={'a': a})
    da = original.transform_coords(['b'], graph={'b': 'a'})
    assert da.dims == ['b']
    da = original.transform_coords(['b'], graph={'b': 'a'}, rename_dims=False)
    assert da.dims == ['a']


def test_keep_intermediate_keep_inputs():
    original = sc.DataArray(data=1 * c, coords={'a': 2 * c, 'b': 3 * c})

    def func(*, ab):
        return 2 * ab

    graph = {'c': func, 'ab': ab}
    da = original.transform_coords(['c'], graph=graph)
    assert 'ab' in da.attrs
    assert 'a' in da.attrs
    assert 'b' in da.attrs
    da = original.transform_coords(['c'], graph=graph, keep_intermediate=False)
    assert 'ab' not in da.meta
    assert 'a' in da.attrs
    assert 'b' in da.attrs
    da = original.transform_coords(['c'], graph=graph, keep_inputs=False)
    assert 'ab' in da.meta
    assert 'a' not in da.attrs
    assert 'b' not in da.attrs
    da = original.transform_coords(['c'],
                                   graph=graph,
                                   keep_intermediate=False,
                                   keep_inputs=False)
    assert 'ab' not in da.meta
    assert 'a' not in da.attrs
    assert 'b' not in da.attrs


def test_inplace():
    original = sc.DataArray(data=1 * c, coords={'a': 2 * c, 'b': 3 * c})

    # This is maybe not recommended usage, but it works.
    def ab_inplace(*, a, b):
        a += b
        return a

    da = original.copy().transform_coords(['ab'], graph={'ab': ab_inplace})
    assert da.dims == ['c']
    assert sc.identical(da.coords['ab'], 2 * c + 3 * c)
    assert sc.identical(da.attrs['a'], 2 * c + 3 * c)


def test_dataset():
    da = sc.DataArray(data=a.copy(), coords={'a': a.copy()})
    ds = sc.Dataset(data={'item1': da.copy(), 'item2': da + da})
    transformed = ds.transform_coords('b', graph={'b': 'a'})
    assert sc.identical(transformed['item1'].attrs['a'], a.rename_dims({'a': 'b'}))
    assert sc.identical(transformed.coords['b'], a.rename_dims({'a': 'b'}))


def test_binned():
    N = 50
    data = sc.DataArray(data=sc.ones(dims=['event'], unit=sc.units.counts, shape=[N]),
                        coords={
                            'x':
                            sc.array(dims=['event'],
                                     unit=sc.units.m,
                                     values=np.random.rand(N)),
                            'y':
                            sc.array(dims=['event'],
                                     unit=sc.units.m,
                                     values=np.random.rand(N))
                        })
    xbins = sc.Variable(dims=['x'], unit=sc.units.m, values=[0.1, 0.5, 0.9])
    ybins = sc.Variable(dims=['y'], unit=sc.units.m, values=[0.1, 0.5, 0.9])
    binned = sc.bin(data, edges=[xbins, ybins])
    del binned.events.coords['y']
    del binned.coords['y']
    binned.coords['y'] = sc.arange(dim='y', start=0, stop=2)

    # TODO Avoid dimension error if we write `x2*y`, provided that it is
    # temporary before rename. May be fixed automatically via fix for #2057.
    def convert(*, x2, y):
        return {'yy': y * y, 'xy': y * x2}

    def check(da, original):
        y = original.coords['y']
        assert 'xy' not in original.events.coords  # Buffer was copied
        assert 'x' in original.events.coords  # Buffer was copied for consume
        assert sc.identical(da.bins.coords['xy'],
                            (y * original.bins.coords['x']).rename_dims({'x': 'x2'}))
        assert 'yy' not in da.events.coords
        assert sc.identical(da.coords['yy'], y * y)

    graph = {'xy': convert, 'x2': 'x'}
    da = binned.transform_coords(['xy', 'yy'], graph=graph)
    check(da, binned)
    # If input is sliced, transform_coords has to copy the buffer
    da = binned['y', 0:1].transform_coords(['xy', 'yy'], graph=graph)
    check(da, binned['y', 0:1])
    da = binned['y', 1:2].transform_coords(['xy', 'yy'], graph=graph)
    check(da, binned['y', 1:2])


def test_cycles():
    original = sc.DataArray(data=a, coords={'a': a, 'b': a})
    with pytest.raises(ValueError):
        original.transform_coords(['c'], graph={'c': 'c'})
    with pytest.raises(ValueError):
        original.transform_coords(['c'], graph={'c': 'd', 'd': 'c'})
    with pytest.raises(ValueError):
        original.transform_coords(['c'], graph={'c': 'd', 'd': 'e', 'e': 'c'})
    with pytest.raises(ValueError):
        original.transform_coords(['c'], graph={'c': bc})
    with pytest.raises(ValueError):
        original.transform_coords(['c'], graph={'c': 'd', 'd': bc})


def test_vararg_fail():
    original = sc.DataArray(data=a, coords={'a': a})

    def func(*args):
        pass

    with pytest.raises(ValueError):
        original.transform_coords(['b'], graph={'b': func})

    def func(**kwargs):
        pass

    with pytest.raises(ValueError):
        original.transform_coords(['b'], graph={'b': func})


def test_arg_vs_kwarg_kwonly():
    def arg(a):
        return a

    def kwarg(a=None):
        return a

    def kwonly(*, a):
        return a

    original = sc.DataArray(data=a, coords={'a': a})
    da = original.transform_coords(['b', 'c', 'd'],
                                   graph={
                                       'b': arg,
                                       'c': kwarg,
                                       'd': kwonly
                                   })
    assert sc.identical(da.coords['b'], original.coords['a'])
    assert sc.identical(da.coords['c'], original.coords['a'])
    assert sc.identical(da.coords['d'], original.coords['a'])


def test_unconsumed_outputs():
    def func(a):
        return {'b': a, 'c': a}

    original = sc.DataArray(data=a, coords={'a': a, 'aux': a})
    da = original.transform_coords(['b'], graph={'b': func})
    assert 'aux' in da.coords
    assert 'b' in da.coords
    assert 'c' not in da.coords
    # 'c' not explicitly requested but named as output in graph => preserved
    da = original.transform_coords(['b'], graph={('b', 'c'): func})
    assert 'aux' in da.coords
    assert 'b' in da.coords
    assert 'c' in da.coords


def test_duplicate_output_keys():
    original = sc.DataArray(data=a, coords={'a': a})

    def to_bc(*, a):
        return {'b': a, 'c': a}

    def to_bd(*, a):
        return {'b': a, 'd': a}

    with pytest.raises(ValueError):
        graph = {'b': 'a', ('b', 'c'): to_bc}
        original.transform_coords(['b'], graph=graph)
    with pytest.raises(ValueError):
        graph = {('b', 'd'): to_bd, ('b', 'c'): to_bc}
        original.transform_coords(['b'], graph=graph)
