# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
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
    assert da.dims == ('x', 'y3')
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
    assert da.dims == ('x', 'y')  # y4 also depends on y so dim not renamed
    assert sc.identical(da.coords['y4'], original.coords['y'])
    assert sc.identical(da.coords['y3'], original.coords['y'])
    assert sc.identical(da.attrs['y2'], original.coords['y'])
    assert sc.identical(da.attrs['y'], original.coords['y'])

    da = original.copy().transform_coords(['y3', 'y4'], graph=graph, keep_aliases=False)
    assert da.dims == ('x', 'y')
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


@pytest.fixture
def a():
    return sc.arange(dim='a', start=0, stop=4)


@pytest.fixture
def b():
    return sc.arange(dim='b', start=2, stop=6)


@pytest.fixture
def c():
    return sc.arange(dim='c', start=4, stop=8)


def test_diamond_graph(a):
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


def test_avoid_consume_of_requested_outputs(a):
    original = sc.DataArray(data=a, coords={'a': a})
    graph = {('b', 'c'): split, 'ab': ab}
    da = original.transform_coords(['ab', 'b'], graph=graph)
    assert 'b' in da.coords
    # Second requested output must not consume first
    da = original.transform_coords(['b', 'ab'], graph=graph)
    assert 'b' in da.coords


def test_multi_output_produced_regardless_of_targets(a):
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


def test_convert_attr_to_coord_with_empty_graph(a):
    original = sc.DataArray(data=a, attrs={'a': a})
    da = original.transform_coords('a', graph={})
    assert da.dims == ('a', )
    assert sc.identical(da.coords['a'], a)
    assert 'a' not in da.attrs


def test_dim_rename_merge_single_dim_coord(a):
    # *a    b
    #   \  /
    #    *ab
    original = sc.DataArray(data=a, coords={'a': a, 'b': 2 * a})
    da = original.transform_coords(['ab'], graph={'ab': ab})
    assert da.dims == ('ab', )
    assert sc.identical(da.coords['ab'], (a + 2 * a).rename_dims({'a': 'ab'}))


def test_dim_rename_split_dim_coord(a, b):
    # a   *b    c
    #  \  / \  /
    #   ab   bc
    original = sc.DataArray(data=b, coords={'a': b, 'b': 2 * b, 'c': 4 * b})
    # Only b is dimension-coord, but it is split => no rename of b
    da = original.transform_coords(['ab', 'bc'], graph={'ab': ab, 'bc': bc})
    assert da.dims == ('b', )
    assert sc.identical(da.coords['ab'], b + 2 * b)
    assert sc.identical(da.coords['bc'], (2 * b) * (4 * b))


def test_dim_rename_merge_two_dim_coords(a, b):
    # *a   *b
    #   \  /
    #    ab
    original = sc.DataArray(data=a + b, coords={'a': a, 'b': b})
    # ab depends on two dimension coords => no rename of a
    da = original.transform_coords(['ab'], graph={'ab': ab})
    assert da.dims == ('a', 'b')
    # Split combined with merge: b is split into ab and b2 and cannot
    # participate in renaming, so a is renamed to ab.
    # *a   *b
    #   \  / \
    #    ab   b2
    da = original.transform_coords(['ab', 'b2'], graph={'ab': ab, 'b2': 'b'})
    assert da.dims == ('ab', 'b')
    da = original.transform_coords(['b2', 'ab'], graph={'ab': ab, 'b2': 'b'})
    assert da.dims == ('ab', 'b')


def test_dim_rename_multi_level_merge(a, b):
    # *b   *a
    #  |   / \
    #  |  c   a2
    #   \ |
    #    *bc
    original = sc.DataArray(data=a + b, coords={'a': a, 'b': b})
    da = original.transform_coords(['bc', 'a2'], graph={'bc': bc, 'c': 'a', 'a2': 'a'})
    # Contributions from a are spread out over c and a2.
    # b is the only full dimension-coord contribution to bc which triggers a rename
    assert da.dims == ('a', 'bc')


def test_dim_rename_multi_level_merge_multi_output(a, b):
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
    assert da.dims == ('a', 'bc')
    da = original.transform_coords(['bc'], graph={'bc': bc, 'c': split_a})
    # a2 is not part of the graph and dropped, a is thus free to be renamed to c.
    # bc then has two dim-coords as input so unlike above, b cannot be renamed to bc.
    assert da.dims == ('c', 'b')


def test_dim_rename_multi_level_multi_merge(a, b, c):
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
    assert da.dims == ('ac', 'bd')

    # c is also a dim coord
    original = sc.DataArray(data=a + b + c, coords={'a': a, 'b': b, 'c': c, 'd': b})
    da = original.transform_coords(['abcd'], graph=graph)
    assert da.dims == ('a', 'bd', 'c')


def test_dim_rename_multi_level_multi_merge_long(a, b, c):
    # *x
    #  |
    # *a    *c
    #  \   /
    #    ac     *b
    #     \    /
    #      abc
    def abc(ac, b):
        return ac + b

    graph = {'a': 'x', 'ac': ac, 'abc': abc}
    x = a.rename_dims({'a': 'x'})
    original = sc.DataArray(data=x + b + c, coords={'x': x, 'b': b, 'c': c})
    da = original.transform_coords(['abc'], graph=graph)
    assert da.dims == ('a', 'b', 'c')


def test_rename_dims_cycle(a, c):
    #   a
    #   |
    #   b   c
    #  / \ /
    # g   d  e
    #  \ /  /
    #   f __
    def f_d(b, c):
        return b + c

    def f_f(g, d, e):
        return g + d + e

    graph = {'b': 'a', 'g': 'b', 'd': f_d, 'f': f_f}
    e = a.rename_dims({'a': 'e'})

    # dim coords: a
    original = sc.DataArray(data=a + c + e, coords={'a': a, 'c': a, 'e': a})
    da = original.transform_coords(['f'], graph=graph)
    assert da.dims == ('f', 'c', 'e')

    # dim coords: a, c
    original = sc.DataArray(data=a + c + e, coords={'a': a, 'c': c, 'e': a})
    da = original.transform_coords(['f'], graph=graph)
    assert da.dims == ('b', 'd', 'e')

    # dim coords: a, e
    original = sc.DataArray(data=a + c + e, coords={'a': a, 'c': a, 'e': e})
    da = original.transform_coords(['f'], graph=graph)
    assert da.dims == ('b', 'c', 'e')


def test_dim_rename_produced_dim_coord(a, b, c):
    # c is a dimension coordinate even though it is computed from a and b.
    #  a    b
    #   \  /
    #     c
    #     |
    #     d
    def f_c(a, b):
        return c

    graph = {'c': f_c, 'd': 'c'}
    # dim coords: a, c
    original = sc.DataArray(data=a + c, coords={'a': a, 'b': a})
    da = original.transform_coords(['d'], graph=graph)
    assert da.dims == ('a', 'd')

    # dim coords: a, b, c
    graph = {'c': f_c, 'd': 'c'}
    original = sc.DataArray(data=a + b + c, coords={'a': a, 'b': b})
    da = original.transform_coords(['d'], graph=graph)
    assert da.dims == ('a', 'b', 'd')


def test_dim_rename_produced_dim_coord_cycle(a, c):
    # c is a dimension coordinate even though it is computed from a.
    #    a
    #   / \
    #  c   b
    #   \ /
    #    d
    def f_c(a):
        return c

    def f_d(b, c):
        return b + c

    graph = {'c': f_c, 'b': 'a', 'd': f_d}
    # dim coords: c
    original = sc.DataArray(data=c, coords={'a': a.rename_dims({'a': 'c'})})
    da = original.transform_coords(['d'], graph=graph)
    assert da.dims == ('d', )

    # dim coords: a, c
    # a's contribution to d is only 1/2 because it gets blocked by c.
    original = sc.DataArray(data=a + c, coords={'a': a})
    da = original.transform_coords(['d'], graph=graph)
    assert da.dims == ('a', 'd')


@pytest.mark.parametrize('keep_inputs', (True, False))
@pytest.mark.parametrize('keep_intermediates', (True, False))
@pytest.mark.parametrize('keep_aliases', (True, False))
def test_dim_rename_keep_arguments_have_no_effect(a, b, keep_inputs, keep_intermediates,
                                                  keep_aliases):
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
                                   keep_intermediate=keep_intermediates,
                                   keep_aliases=keep_aliases,
                                   keep_inputs=keep_inputs)
    assert da.dims == ('a', 'b')


def test_rename_dims_param(a):
    original = sc.DataArray(data=a, coords={'a': a})
    da = original.transform_coords(['b'], graph={'b': 'a'})
    assert da.dims == ('b', )
    da = original.transform_coords(['b'], graph={'b': 'a'}, rename_dims=False)
    assert da.dims == ('a', )


@pytest.fixture(params=[('coord', 'coord'), ('coord', 'attr'), ('attr', 'coord'),
                        ('attr', 'attr')])
def binned_in_a_b(request):
    events = sc.DataArray(data=sc.arange('event', 10, unit='counts'),
                          coords={
                              'a':
                              sc.array(dims=['event'],
                                       values=np.random.rand(10),
                                       unit='m'),
                              'b':
                              sc.array(dims=['event'],
                                       values=np.random.rand(10),
                                       unit='m')
                          })
    binned = sc.bin(events,
                    edges=[
                        sc.linspace('a', 0, 1, 3, unit='m'),
                        sc.linspace('b', 0, 1, 3, unit='m')
                    ])
    # use non-bin-edge coord
    binned.coords['b'] = sc.arange('b', 2, unit='m')

    bin_src, event_src = request.param
    if bin_src == 'attr':
        binned.attrs['a'] = binned.coords.pop('a')
    if event_src == 'attr':
        binned.bins.attrs['a'] = binned.bins.coords.pop('a')

    return binned


@pytest.mark.parametrize('keep_inputs', (True, False))
@pytest.mark.parametrize('keep_intermediate', (True, False))
def test_keep_aliases_dense(a, b, keep_inputs, keep_intermediate):
    original = sc.DataArray(data=a + b, coords={'a': a}, attrs={'b': b})
    graph = {'d': 'c', 'c': 'ab', 'ab': ab}
    da = original.transform_coords(['d'],
                                   graph=graph,
                                   keep_aliases=True,
                                   keep_inputs=keep_inputs,
                                   keep_intermediate=keep_intermediate)
    assert 'c' in da.attrs
    assert 'd' in da.coords


@pytest.mark.parametrize('keep_inputs', (True, False))
@pytest.mark.parametrize('keep_intermediate', (True, False))
def test_not_keep_aliases_dense(a, b, keep_inputs, keep_intermediate):
    original = sc.DataArray(data=a + b, coords={'a': a}, attrs={'b': b})
    graph = {'d': 'c', 'c': 'ab', 'ab': ab}
    da = original.transform_coords(['d'],
                                   graph=graph,
                                   keep_aliases=False,
                                   keep_inputs=keep_inputs,
                                   keep_intermediate=keep_intermediate)
    assert 'c' not in da.meta
    assert 'd' in da.coords


@pytest.mark.parametrize('keep_inputs', (True, False))
@pytest.mark.parametrize('keep_intermediate', (True, False))
def test_keep_aliases_binned(binned_in_a_b, keep_inputs, keep_intermediate):
    graph = {'d': 'c', 'c': 'ab', 'ab': ab}
    da = binned_in_a_b.transform_coords(['d'],
                                        graph=graph,
                                        keep_aliases=True,
                                        keep_inputs=keep_inputs,
                                        keep_intermediate=keep_intermediate)
    assert 'c' in da.attrs
    assert 'c' in da.bins.attrs
    assert 'd' in da.coords
    assert 'd' in da.bins.coords


@pytest.mark.parametrize('keep_inputs', (True, False))
@pytest.mark.parametrize('keep_intermediate', (True, False))
def test_not_keep_aliases_binned(binned_in_a_b, keep_inputs, keep_intermediate):
    graph = {'d': 'c', 'c': 'ab', 'ab': ab}
    da = binned_in_a_b.transform_coords(['d'],
                                        graph=graph,
                                        keep_aliases=False,
                                        keep_inputs=keep_inputs,
                                        keep_intermediate=keep_intermediate)
    assert 'c' not in da.meta
    assert 'c' not in da.bins.meta
    assert 'd' in da.coords
    assert 'd' in da.bins.coords


@pytest.mark.parametrize('keep_aliases', (True, False))
@pytest.mark.parametrize('keep_intermediate', (True, False))
def test_keep_inputs_dense(a, b, keep_aliases, keep_intermediate):
    original = sc.DataArray(data=a + b, coords={'a': a}, attrs={'b': b})
    graph = {'c': 'ab', 'ab': ab}
    da = original.transform_coords(['c'],
                                   graph=graph,
                                   keep_aliases=keep_aliases,
                                   keep_inputs=True,
                                   keep_intermediate=keep_intermediate)
    assert 'a' in da.attrs
    assert 'b' in da.attrs


@pytest.mark.parametrize('keep_aliases', (True, False))
@pytest.mark.parametrize('keep_intermediate', (True, False))
def test_not_keep_inputs_dense(a, b, keep_aliases, keep_intermediate):
    original = sc.DataArray(data=a + b, coords={'a': a}, attrs={'b': b})
    graph = {'c': 'ab', 'ab': ab}
    da = original.transform_coords(['c'],
                                   graph=graph,
                                   keep_aliases=keep_aliases,
                                   keep_inputs=False,
                                   keep_intermediate=keep_intermediate)
    assert 'a' not in da.meta
    assert 'b' not in da.meta


@pytest.mark.parametrize('keep_aliases', (True, False))
@pytest.mark.parametrize('keep_intermediate', (True, False))
def test_keep_inputs_binned(binned_in_a_b, keep_aliases, keep_intermediate):
    graph = {'c': 'ab', 'ab': ab}
    da = binned_in_a_b.transform_coords(['c'],
                                        graph=graph,
                                        keep_aliases=keep_aliases,
                                        keep_inputs=True,
                                        keep_intermediate=keep_intermediate)
    assert 'a' in da.attrs
    assert 'a' in da.bins.attrs
    assert 'b' in da.attrs
    assert 'b' in da.bins.attrs


@pytest.mark.parametrize('keep_aliases', (True, False))
@pytest.mark.parametrize('keep_intermediate', (True, False))
def test_not_keep_inputs_binned(binned_in_a_b, keep_aliases, keep_intermediate):
    graph = {'c': 'ab', 'ab': ab}
    da = binned_in_a_b.transform_coords(['c'],
                                        graph=graph,
                                        keep_aliases=keep_aliases,
                                        keep_inputs=False,
                                        keep_intermediate=keep_intermediate)
    assert 'a' not in da.meta
    assert 'a' not in da.bins.meta
    assert 'b' not in da.meta
    assert 'b' not in da.bins.meta


@pytest.mark.parametrize('keep_aliases', (True, False))
@pytest.mark.parametrize('keep_inputs', (True, False))
def test_keep_intermediate_dense(a, b, keep_aliases, keep_inputs):
    original = sc.DataArray(data=a + b, coords={'a': a}, attrs={'b': b})
    graph = {'c': 'ab', 'ab': ab}
    da = original.transform_coords(['c'],
                                   graph=graph,
                                   keep_aliases=keep_aliases,
                                   keep_inputs=keep_inputs,
                                   keep_intermediate=True)
    assert 'ab' in da.attrs


@pytest.mark.parametrize('keep_aliases', (True, False))
@pytest.mark.parametrize('keep_inputs', (True, False))
def test_not_keep_intermediate_dense(a, b, keep_aliases, keep_inputs):
    original = sc.DataArray(data=a + b, coords={'a': a}, attrs={'b': b})
    graph = {'c': 'ab', 'ab': ab}
    da = original.transform_coords(['c'],
                                   graph=graph,
                                   keep_aliases=keep_aliases,
                                   keep_inputs=keep_inputs,
                                   keep_intermediate=False)
    assert 'ab' not in da.meta


@pytest.mark.parametrize('keep_aliases', (True, False))
@pytest.mark.parametrize('keep_inputs', (True, False))
def test_keep_intermediate_binned(binned_in_a_b, keep_aliases, keep_inputs):
    graph = {'c': 'ab', 'ab': ab}
    da = binned_in_a_b.transform_coords(['c'],
                                        graph=graph,
                                        keep_aliases=keep_aliases,
                                        keep_inputs=keep_inputs,
                                        keep_intermediate=True)
    assert 'ab' in da.attrs
    assert 'ab' in da.bins.attrs


@pytest.mark.parametrize('keep_aliases', (True, False))
@pytest.mark.parametrize('keep_inputs', (True, False))
def test_not_keep_intermediate_binned(binned_in_a_b, keep_aliases, keep_inputs):
    graph = {'c': 'ab', 'ab': ab}
    da = binned_in_a_b.transform_coords(['c'],
                                        graph=graph,
                                        keep_aliases=keep_aliases,
                                        keep_inputs=keep_inputs,
                                        keep_intermediate=False)
    assert 'ab' not in da.meta
    assert 'ab' not in da.bins.meta


def test_inplace(c):
    original = sc.DataArray(data=1 * c, coords={'a': 2 * c, 'b': 3 * c})

    # This is maybe not recommended usage, but it works.
    def ab_inplace(*, a, b):
        a += b
        return a

    da = original.copy().transform_coords(['ab'], graph={'ab': ab_inplace})
    assert da.dims == ('c', )
    assert sc.identical(da.coords['ab'], 2 * c + 3 * c)
    assert sc.identical(da.attrs['a'], 2 * c + 3 * c)


def test_dataset(a):
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


def test_binned_does_not_modify_inputs(binned_in_a_b):
    _ = binned_in_a_b.transform_coords(['b2'], graph={'b2': 'b'})
    assert 'b' in binned_in_a_b.coords
    assert 'b' in binned_in_a_b.bins.coords
    assert 'b2' not in binned_in_a_b.meta
    assert 'b2' not in binned_in_a_b.bins.meta


def test_binned_with_range_slice_does_not_modify_inputs(binned_in_a_b):
    # If input is sliced, transform_coords has to copy the buffer
    original = binned_in_a_b['a', 0:1]
    _ = original.transform_coords(['b2'], graph={'b2': 'b'})
    assert 'b' in original.coords
    assert 'b' in original.bins.coords
    assert 'b2' not in original.meta
    assert 'b2' not in original.bins.meta


def test_binned_with_range_slice_does_not_modify_inputs_copy(binned_in_a_b):
    # If input is sliced, transform_coords has to copy the buffer.
    # The copy introduces a mismatch in bin indices of the new coord and DataArray.
    original = binned_in_a_b['a', 1:2]
    _ = original.transform_coords(['b2'], graph={'b2': lambda b: b.copy()})
    assert 'b' in original.coords
    assert 'b' in original.bins.coords
    assert 'b2' not in original.meta
    assert 'b2' not in original.bins.meta


def test_binned_with_point_slice_does_not_modify_inputs(binned_in_a_b):
    # If input is sliced, transform_coords has to copy the buffer
    original = binned_in_a_b['a', 1]
    _ = original.transform_coords(['b2'], graph={'b2': 'b'})
    assert 'b' in original.coords
    assert 'b' in original.bins.coords
    assert 'b2' not in original.meta
    assert 'b2' not in original.bins.meta


def test_binned_computes_correct_results(binned_in_a_b):
    def convert(*, a2, b):
        return a2 * b

    graph = {'a*b': convert, 'a2': 'a'}
    converted = binned_in_a_b.transform_coords(['a*b'], graph=graph)
    renamed = binned_in_a_b.rename_dims({'a': 'a2'})

    # `a` was renamed to `a2`
    assert sc.identical(converted.meta['a2'], renamed.meta['a'])
    assert sc.identical(converted.bins.meta['a2'], renamed.bins.meta['a'])

    # `a*b` is indeed the product of `a` and `b`
    assert sc.identical(converted.coords['a*b'], renamed.meta['a'] * renamed.meta['b'])
    assert sc.identical(converted.bins.coords['a*b'],
                        renamed.bins.meta['a'] * renamed.bins.meta['b'])


@pytest.mark.parametrize('slice_', (slice(0, 2), slice(2, 4)))
def test_binned_slice_computes_correct_results(slice_):
    events = sc.DataArray(sc.ones(dims=['event'], shape=[10]),
                          coords={'x': sc.arange('event', 10.0)})
    da = sc.bin(events, edges=[sc.arange('x', 0.0, 10.0, 2.0)])
    sliced = da['x', slice_]

    assert sc.identical(
        sliced.transform_coords('y', graph={'y': lambda x: 2 * x}),
        sliced.copy().transform_coords('y', graph={'y': lambda x: 2 * x}))


def test_binned_without_bin_coord_computes_correct_results(binned_in_a_b):
    def convert(*, a, b2):
        return a * b2

    graph = {'a*b': convert, 'b2': 'b'}
    del binned_in_a_b.coords['b']
    converted = binned_in_a_b.transform_coords(['a*b'], graph=graph)
    renamed = binned_in_a_b.rename_dims({'b': 'b2'})

    # `b` was renamed to `b2`
    assert sc.identical(converted.bins.meta['b2'], renamed.bins.meta['b'])

    # `a*b` is indeed the product of `a` and `b`
    assert sc.identical(converted.bins.coords['a*b'],
                        renamed.bins.meta['a'] * renamed.bins.meta['b'])


def test_binned_without_event_coord_computes_correct_results(binned_in_a_b):
    def convert(*, a, b2):
        return a * b2

    graph = {'a*b': convert, 'b2': 'b'}
    del binned_in_a_b.bins.coords['b']
    converted = binned_in_a_b.transform_coords(['a*b'], graph=graph)
    renamed = binned_in_a_b.rename_dims({'b': 'b2'})

    # `b` was renamed to `b2`
    assert sc.identical(converted.meta['b2'], renamed.meta['b'])

    # `a*b` is indeed the product of `a` and `b`
    assert sc.identical(converted.coords['a*b'], renamed.meta['a'] * renamed.meta['b'])


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


def test_only_outputs_in_graph_are_stored(a):
    original = sc.DataArray(data=a, coords={'a': a})
    graph = {'b': split}
    da = original.transform_coords(['b'], graph=graph)
    assert 'c' not in da.meta  # c is not stored
    with pytest.raises(KeyError):
        # c is not computable
        original.transform_coords(['c'], graph=graph)


def test_targets_arg_types(a, b):
    original = sc.DataArray(data=a + b, coords={'a': a, 'b': b})
    graph = {'ab': ab}
    da = original.transform_coords(('ab', ), graph=graph)
    assert 'ab' in da.coords
    da = original.transform_coords('ab', graph=graph)
    assert 'ab' in da.coords


def test_raises_when_requested_coord_not_computed(a):
    def foo(a):
        return {'actual': a}

    original = sc.DataArray(data=a, coords={'a': a})
    with pytest.raises(TypeError):
        original.transform_coords('expected', graph={'expected': foo})


def test_raises_when_expected_multiple_outputs_but_returned_non_dict(a):
    def foo(a):
        return a

    graph = {('b', 'c'): foo}
    original = sc.DataArray(data=a, coords={'a': a})
    with pytest.raises(TypeError):
        original.transform_coords(['b', 'c'], graph=graph)


def test_inaccessible_coord(a, b):
    original = sc.DataArray(data=a + b, coords={'a': a})
    graph = {'ab': ab}
    with pytest.raises(KeyError):
        original.transform_coords(['ab'], graph)
    with pytest.raises(KeyError):
        original.transform_coords(['c'], graph)

    def abc(a, b, c):
        return a + b + c

    original = sc.DataArray(data=a + b, coords={'a': a, 'b': b})
    graph = {'ab': ab, 'abc': abc}
    with pytest.raises(KeyError):
        original.transform_coords(['ab', 'abc'], graph)


def test_cycles(a):
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


def test_new_dim_in_coord(a):
    def x(a):
        return a.rename_dims({'a': 'x'})

    original = sc.DataArray(data=a, coords={'a': a})
    with pytest.raises(sc.DimensionError):
        original.transform_coords(['x'], graph={'x': x})

    def ax(a):
        return sc.concat([a, a], dim='x')

    original = sc.DataArray(data=a, coords={'a': a})
    with pytest.raises(sc.DimensionError):
        original.transform_coords(['x'], graph={'x': ax})


def test_vararg_fail(a):
    original = sc.DataArray(data=a, coords={'a': a})

    def func(*args):
        pass

    with pytest.raises(ValueError):
        original.transform_coords(['b'], graph={'b': func})

    def func(**kwargs):
        pass

    with pytest.raises(ValueError):
        original.transform_coords(['b'], graph={'b': func})


def test_arg_vs_kwarg_kwonly(a):
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


def test_unconsumed_outputs(a):
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


def test_duplicate_output_keys(a):
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


def test_prioritize_coords_attrs_conflict(a):
    original = sc.DataArray(data=a, coords={'a': a}, attrs={'a': -1 * a})

    with pytest.raises(sc.DataArrayError):
        original.transform_coords(['b'], graph={'b': 'a'})
