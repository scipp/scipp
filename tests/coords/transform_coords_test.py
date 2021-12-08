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
    da = original.copy().transform_coords(['y3'], graph=graph, keep_aliases=True)
    assert da.dims == ['x', 'y3']
    original = original.rename_dims({'y': 'y3'})
    assert sc.identical(da.coords['y3'], original.coords['y'])
    assert sc.identical(da.attrs['y2'], original.coords['y'])
    assert sc.identical(da.attrs['y'], original.coords['y'])

    da = original.copy().transform_coords(['y3'], graph=graph, keep_aliases=False)
    assert 'y3' in da.coords  # alias, but requested explicitly
    assert 'y2' not in da.attrs  # alias for y => removed
    assert 'y' in da.attrs


def test_rename_multi_output():
    original = _make_xy()
    graph = {'y4': 'y', 'y3': 'y2', 'y2': 'y'}
    da = original.copy().transform_coords(['y3', 'y4'], graph=graph, keep_aliases=True)
    assert da.dims == ['x', 'y']  # y4 also depends on y so dim not renamed
    assert sc.identical(da.coords['y4'], original.coords['y'])
    assert sc.identical(da.coords['y3'], original.coords['y'])
    assert sc.identical(da.attrs['y2'], original.coords['y'])
    assert sc.identical(da.attrs['y'], original.coords['y'])

    da = original.copy().transform_coords(['y3', 'y4'], graph=graph, keep_aliases=False)
    assert da.dims == ['x', 'y']
    assert sc.identical(da.coords['y4'], original.coords['y'])
    assert sc.identical(da.coords['y3'], original.coords['y'])
    assert 'y2' not in da.meta
    assert sc.identical(da.attrs['y'], original.coords['y'])


def ab(*, a, b):
    return a + b


def bc(*, b, c):
    return b * c


def ac(a, c):
    return a + c


def bd(b, d):
    return b + d


def split(*, a):
    return {'b': a, 'c': 2 * a}


a = sc.arange(dim='a', start=0, stop=4)
b = sc.arange(dim='b', start=2, stop=6)
c = sc.arange(dim='c', start=4, stop=8)


def test_diamond_graph():
    #   *a
    #  /  \
    # b    c
    #  \  /
    #   *d
    original = sc.DataArray(data=a, coords={'a': a})
    graph = {('b', 'c'): split, 'd': bc}
    da = original.transform_coords(['d'], graph=graph)
    expected = (a * (2 * a)).rename_dims({'a': 'd'})
    assert sc.identical(da.coords['d'], expected)


def test_avoid_consume_of_requested_outputs():
    original = sc.DataArray(data=a, coords={'a': a})
    graph = {('b', 'c'): split, 'ab': ab}
    da = original.transform_coords(['ab', 'b'], graph=graph)
    assert 'b' in da.coords
    # Second requested output must not consume first
    da = original.transform_coords(['b', 'ab'], graph=graph)
    assert 'b' in da.coords


def test_multi_output_produced_regardless_of_targets():
    #   *a
    #  /  \
    # a2  a3
    def split_a(a):
        return {'a2': a, 'a3': a}

    graph = {('a2', 'a3'): split_a}
    original = sc.DataArray(data=a, coords={'a': a})
    expected = sc.DataArray(data=a, coords={'a2': a, 'a3': a}, attrs={'a': a})
    # a3 is computed regardless of whether it is requested.
    assert sc.identical(original.transform_coords(['a2', 'a3'], graph=graph), expected)
    assert sc.identical(original.transform_coords(['a2'], graph=graph), expected)

    graph = {'a2': split_a}
    expected = sc.DataArray(data=a, coords={'a2': a}, attrs={'a': a})
    expected = expected.rename_dims({'a': 'a2'})
    # a3 is not computed because it is not part of the graph.
    assert sc.identical(original.transform_coords(['a2'], graph=graph), expected)


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
    # Split combined with merge: Ensure that b2 does not cause rename of b
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
    # Similar to test_dim_rename_multi_level_merge above,
    # but a2 and c are produced by the same node
    # *b   *a
    #  |   / \
    #  |  c   a2
    #   \ |
    #    *bc
    def split_a(*, a):
        return {'a2': a, 'c': a}

    original = sc.DataArray(data=a + b, coords={'a': a, 'b': b})
    da = original.transform_coords(['bc'], graph={'bc': bc, ('c', 'a2'): split_a})
    assert da.dims == ['a', 'bc']
    da = original.transform_coords(['bc'], graph={'bc': bc, 'c': split_a})
    # a2 is not part of the graph and dropped, a is thus free to be renamed to c.
    assert da.dims == ['c', 'b']


def test_dim_rename_multi_level_multi_merge():
    # *a    c  *b   d
    #  \   /    \  /
    #   *ac     *bd
    #     \     /
    #      abcd
    def abcd(ac, bd):
        return ac + bd

    graph = {'ac': ac, 'bd': bd, 'abcd': abcd}
    original = sc.DataArray(data=a + b, coords={'a': a, 'b': b, 'c': a, 'd': b})
    da = original.transform_coords(['abcd'], graph=graph)
    assert da.dims == ['ac', 'bd']

    # c is also a dim coord
    original = sc.DataArray(data=a + b + c, coords={'a': a, 'b': b, 'c': c, 'd': b})
    da = original.transform_coords(['abcd'], graph=graph)
    assert da.dims == ['a', 'abcd', 'c']


def test_dim_rename_multi_level_multi_merge_long():
    # *x
    #  |
    # *a    *c
    #  \   /
    #    ac      *b
    #     \     /
    #      *abc
    def abc(ac, b):
        return ac + b

    graph = {'a': 'x', 'ac': ac, 'abc': abc}
    x = a.rename_dims({'a': 'x'})
    original = sc.DataArray(data=x + b + c, coords={'x': x, 'b': b, 'c': c})
    da = original.transform_coords(['abc'], graph=graph)
    assert da.dims == ['a', 'abc', 'c']


def test_rename_dims_cycle():
    #   a
    #   |
    #   b   c
    #  / \ /
    # c   d  e
    #  \ /  /
    #   f __
    def f_d(b, c):
        return b + c

    def f_f(c, d, e):
        return c + d + e

    graph = {'b': 'a', 'c': 'b', 'd': f_d, 'f': f_f}
    original = sc.DataArray(data=a, coords={'a': a, 'c': a, 'e': a})
    da = original.transform_coords(['f'], graph=graph)
    assert da.dims == ['f']

    # c is a dimension coord
    original = sc.DataArray(data=a + c, coords={'a': a, 'c': c, 'e': a})
    da = original.transform_coords(['f'], graph=graph)
    assert da.dims == ['b', 'c']

    # e is a dimension coord
    e = a.rename_dims({'a': 'e'})
    original = sc.DataArray(data=a + e, coords={'a': a, 'c': a, 'e': e})
    da = original.transform_coords(['f'], graph=graph)
    assert da.dims == ['b', 'e']


@pytest.mark.parametrize('keep_inputs', (True, False))
def test_dim_rename_keep_arguments(keep_inputs):
    # *x
    #  |
    #  y
    #  |
    #  a   *b
    #   \  |
    #    ab
    def y(x):
        return x

    graph = {'y': y, 'a': 'y', 'ab': ab}
    x = a.rename_dims({'a': 'x'})
    original = sc.DataArray(data=x + b, coords={'x': x, 'b': b})

    da = original.transform_coords(['ab'],
                                   graph=graph,
                                   keep_intermediate=True,
                                   keep_aliases=True,
                                   keep_inputs=keep_inputs)
    assert da.dims == ['a', 'b']
    da = original.transform_coords(['ab'],
                                   graph=graph,
                                   keep_intermediate=False,
                                   keep_aliases=True,
                                   keep_inputs=keep_inputs)
    assert da.dims == ['a', 'b']
    da = original.transform_coords(['ab'],
                                   graph=graph,
                                   keep_intermediate=True,
                                   keep_aliases=False,
                                   keep_inputs=keep_inputs)
    assert da.dims == ['y', 'b']
    da = original.transform_coords(['ab'],
                                   graph=graph,
                                   keep_intermediate=False,
                                   keep_aliases=False,
                                   keep_inputs=keep_inputs)
    assert da.dims == ['x', 'b']


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


def make_binned():
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
    del binned.bins.coords['y']
    del binned.coords['y']
    binned.coords['y'] = sc.arange(dim='y', start=0, stop=2)
    return binned


def check_binned(da, original):
    y = original.coords['y']
    assert 'xy' not in original.bins.coords  # Buffer was copied
    assert 'x' in original.bins.coords  # Buffer was copied for consume
    assert sc.identical(da.bins.coords['xy'],
                        (y * original.bins.coords['x']).rename_dims({'x': 'x2'}))
    assert 'yy' not in da.bins.coords
    assert sc.identical(da.coords['yy'], y * y)


def test_binned():
    binned = make_binned()

    def convert(*, x2, y):
        return {'yy': y * y, 'xy': x2 * y}

    graph = {('xy', 'yy'): convert, 'x2': 'x'}
    da = binned.transform_coords(['xy', 'yy'], graph=graph)
    check_binned(da, binned)
    assert sc.identical(da.coords['xy'], (binned.coords['x'] *
                                          binned.coords['y']).rename_dims({'x': 'x2'}))
    # If input is sliced, transform_coords has to copy the buffer
    da = binned['y', 0:1].transform_coords(['xy', 'yy'], graph=graph)
    check_binned(da, binned['y', 0:1])
    da = binned['y', 1:2].transform_coords(['xy', 'yy'], graph=graph)
    check_binned(da, binned['y', 1:2])


def test_binned_no_dense_coord():
    binned = make_binned()
    del binned.coords['x']

    def convert(*, x2, y):
        return {'yy': y * y, 'xy': x2 * y}

    graph = {('xy', 'yy'): convert, 'x2': 'x'}
    da = binned.transform_coords(['xy', 'yy'], graph=graph)
    check_binned(da, binned)


def test_binned_request_existing_consumed():
    binned = make_binned()

    def xy(*, x, y):
        return x * y

    def unused(x):
        pass

    graph = {'xy': xy, 'x': unused}
    # Regression test, this used to throw due to an untested branch when
    # requesting a previously consumed event coord
    binned.transform_coords(['xy', 'x'], graph=graph)


def test_only_outputs_in_graph_are_stored():
    original = sc.DataArray(data=a, coords={'a': a})
    graph = {'b': split}
    da = original.transform_coords(['b'], graph=graph)
    assert 'c' not in da.meta  # c is not stored
    with pytest.raises(sc.NotFoundError):
        # c is not computable
        original.transform_coords(['c'], graph=graph)


def test_targets_arg_types():
    original = sc.DataArray(data=a + b, coords={'a': a, 'b': b})
    graph = {'ab': ab}
    da = original.transform_coords(('ab', ), graph=graph)
    assert 'ab' in da.coords
    da = original.transform_coords('ab', graph=graph)
    assert 'ab' in da.coords


def test_inaccessible_coord():
    original = sc.DataArray(data=a + b, coords={'a': a})
    graph = {'ab': ab}
    with pytest.raises(sc.NotFoundError):
        original.transform_coords(['ab'], graph)
    with pytest.raises(sc.NotFoundError):
        original.transform_coords(['c'], graph)

    def abc(a, b, c):
        return a + b + c

    original = sc.DataArray(data=a + b, coords={'a': a, 'b': b})
    graph = {'ab': ab, 'abc': abc}
    with pytest.raises(sc.NotFoundError):
        original.transform_coords(['ab', 'abc'], graph)


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
    # b only named once in graph
    graph = {'b': to_bd, 'c': to_bc}
    da = original.transform_coords(['b'], graph=graph)
    assert 'b' in da.coords


def test_prioritize_coords_attrs_conflict():
    original = sc.DataArray(data=a, coords={'a': a}, attrs={'a': -1 * a})

    with pytest.raises(sc.DataArrayError):
        original.transform_coords(['b'], graph={'b': 'a'})
