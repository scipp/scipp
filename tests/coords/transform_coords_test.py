# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
import functools
from graphlib import CycleError

import numpy as np
import pytest

import scipp as sc


def _make_xy():
    x = sc.linspace(dim='x', unit='m', start=0.1, stop=0.2, num=3)
    y = sc.linspace(dim='y', unit='m', start=0.1, stop=0.3, num=4)
    return sc.DataArray(
        data=sc.ones(dims=['x', 'y'], shape=[3, 4]), coords={'x': x, 'y': y}
    )


def test_rename_2_steps() -> None:
    original = _make_xy()
    graph = {'y4': 'y', 'y3': 'y2', 'y2': 'y'}
    da = original.copy().transform_coords(['y3'], graph=graph, keep_aliases=True)
    assert da.dims == ('x', 'y3')
    original = original.rename_dims({'y': 'y3'})
    assert sc.identical(da.coords['y3'], original.coords['y'])
    assert sc.identical(da.coords['y2'], original.coords['y'])
    assert sc.identical(da.coords['y'], original.coords['y'])

    da = original.copy().transform_coords(['y3'], graph=graph, keep_aliases=False)
    assert 'y3' in da.coords  # alias, but requested explicitly
    assert 'y2' not in da.coords  # alias for y => removed
    assert 'y' in da.coords


def test_rename_multi_output() -> None:
    original = _make_xy()
    graph = {'y4': 'y', 'y3': 'y2', 'y2': 'y'}
    da = original.copy().transform_coords(['y3', 'y4'], graph=graph, keep_aliases=True)
    assert da.dims == ('x', 'y')  # y4 also depends on y so dim not renamed
    assert sc.identical(da.coords['y4'], original.coords['y'])
    assert sc.identical(da.coords['y3'], original.coords['y'])
    assert sc.identical(da.coords['y2'], original.coords['y'])
    assert sc.identical(da.coords['y'], original.coords['y'])

    da = original.copy().transform_coords(['y3', 'y4'], graph=graph, keep_aliases=False)
    assert da.dims == ('x', 'y')
    assert sc.identical(da.coords['y4'], original.coords['y'])
    assert sc.identical(da.coords['y3'], original.coords['y'])
    assert 'y2' not in da.coords
    assert sc.identical(da.coords['y'], original.coords['y'])


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


def test_diamond_graph(a) -> None:
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


def test_avoid_consume_of_requested_outputs(a) -> None:
    original = sc.DataArray(data=a, coords={'a': a})
    graph = {('b', 'c'): split, 'ab': ab}
    da = original.transform_coords(['ab', 'b'], graph=graph)
    assert 'b' in da.coords
    # Second requested output must not consume first
    da = original.transform_coords(['b', 'ab'], graph=graph)
    assert 'b' in da.coords


def test_multi_output_produced_regardless_of_targets(a) -> None:
    #   *a
    #  /  \
    # a2  a3
    def split_a(a):
        return {'a2': a, 'a3': a}

    graph = {('a2', 'a3'): split_a}
    original = sc.DataArray(data=a, coords={'a': a})
    expected = sc.DataArray(data=a, coords={'a': a, 'a2': a, 'a3': a})
    expected.coords.set_aligned('a', False)
    # a3 is computed regardless of whether it is requested.
    assert sc.identical(original.transform_coords(['a2', 'a3'], graph=graph), expected)
    assert sc.identical(original.transform_coords(['a2'], graph=graph), expected)

    graph = {'a2': split_a}
    expected = sc.DataArray(data=a, coords={'a': a, 'a2': a})
    expected = expected.rename_dims({'a': 'a2'})
    expected.coords.set_aligned('a', False)
    # a3 is not computed because it is not part of the graph.
    assert sc.identical(original.transform_coords(['a2'], graph=graph), expected)


def test_dim_rename_merge_single_dim_coord(a) -> None:
    # *a    b
    #   \  /
    #    *ab
    original = sc.DataArray(data=a, coords={'a': a, 'b': 2 * a})
    da = original.transform_coords(['ab'], graph={'ab': ab})
    assert da.dims == ('ab',)
    assert sc.identical(da.coords['ab'], (a + 2 * a).rename_dims({'a': 'ab'}))


def test_dim_rename_split_dim_coord(a, b) -> None:
    # a   *b    c
    #  \  / \  /
    #   ab   bc
    original = sc.DataArray(data=b, coords={'a': b, 'b': 2 * b, 'c': 4 * b})
    # Only b is dimension-coord, but it is split => no rename of b
    da = original.transform_coords(['ab', 'bc'], graph={'ab': ab, 'bc': bc})
    assert da.dims == ('b',)
    assert sc.identical(da.coords['ab'], b + 2 * b)
    assert sc.identical(da.coords['bc'], (2 * b) * (4 * b))


def test_dim_rename_merge_two_dim_coords(a, b) -> None:
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


def test_dim_rename_multi_level_merge(a, b) -> None:
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


def test_dim_rename_multi_level_merge_multi_output(a, b) -> None:
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


def test_dim_rename_multi_level_multi_merge(a, b, c) -> None:
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


def test_dim_rename_multi_level_multi_merge_long(a, b, c) -> None:
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


def test_rename_dims_cycle(a, c) -> None:
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


def test_dim_rename_produced_dim_coord(a, b, c) -> None:
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


def test_dim_rename_produced_dim_coord_cycle(a, c) -> None:
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
    assert da.dims == ('d',)

    # dim coords: a, c
    # a's contribution to d is only 1/2 because it gets blocked by c.
    original = sc.DataArray(data=a + c, coords={'a': a})
    da = original.transform_coords(['d'], graph=graph)
    assert da.dims == ('a', 'd')


@pytest.mark.parametrize('keep_inputs', [True, False])
@pytest.mark.parametrize('keep_intermediates', [True, False])
@pytest.mark.parametrize('keep_aliases', [True, False])
def test_dim_rename_keep_arguments_have_no_effect(
    a, b, keep_inputs, keep_intermediates, keep_aliases
):
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

    da = original.transform_coords(
        ['ab'],
        graph=graph,
        keep_intermediate=keep_intermediates,
        keep_aliases=keep_aliases,
        keep_inputs=keep_inputs,
    )
    assert da.dims == ('a', 'b')


def test_rename_dims_param(a) -> None:
    original = sc.DataArray(data=a, coords={'a': a})
    da = original.transform_coords(['b'], graph={'b': 'a'})
    assert da.dims == ('b',)
    da = original.transform_coords(['b'], graph={'b': 'a'}, rename_dims=False)
    assert da.dims == ('a',)


@pytest.fixture
def binned_in_a_b():
    events = sc.DataArray(
        data=sc.arange('event', 10, unit='counts'),
        coords={
            'a': sc.array(dims=['event'], values=np.random.rand(10), unit='m'),
            'b': sc.array(dims=['event'], values=np.random.rand(10), unit='m'),
        },
    )
    binned = events.bin(a=2, b=2)
    # use non-bin-edge coord
    binned.coords['b'] = sc.arange('b', 2, unit='m')
    return binned


@pytest.mark.parametrize('keep_inputs', [True, False])
@pytest.mark.parametrize('keep_intermediate', [True, False])
def test_keep_aliases_dense(a, b, keep_inputs, keep_intermediate) -> None:
    original = sc.DataArray(data=a + b, coords={'a': a, 'b': b})
    graph = {'d': 'c', 'c': 'ab', 'ab': ab}
    da = original.transform_coords(
        ['d'],
        graph=graph,
        keep_aliases=True,
        keep_inputs=keep_inputs,
        keep_intermediate=keep_intermediate,
    )
    assert 'c' in da.coords
    assert not da.coords['c'].aligned
    assert 'd' in da.coords


@pytest.mark.parametrize('keep_inputs', [True, False])
@pytest.mark.parametrize('keep_intermediate', [True, False])
def test_not_keep_aliases_dense(a, b, keep_inputs, keep_intermediate) -> None:
    original = sc.DataArray(data=a + b, coords={'a': a, 'b': b})
    graph = {'d': 'c', 'c': 'ab', 'ab': ab}
    da = original.transform_coords(
        ['d'],
        graph=graph,
        keep_aliases=False,
        keep_inputs=keep_inputs,
        keep_intermediate=keep_intermediate,
    )
    assert 'c' not in da.coords
    assert 'd' in da.coords

    # Requesting input as target preserves it.
    da = original.transform_coords(
        ['d', 'c'],
        graph=graph,
        keep_aliases=False,
        keep_inputs=keep_inputs,
        keep_intermediate=keep_intermediate,
    )
    assert 'c' in da.coords
    assert 'd' in da.coords


@pytest.mark.parametrize('keep_inputs', [True, False])
@pytest.mark.parametrize('keep_intermediate', [True, False])
def test_keep_aliases_binned(binned_in_a_b, keep_inputs, keep_intermediate) -> None:
    graph = {'d': 'c', 'c': 'ab', 'ab': ab}
    da = binned_in_a_b.transform_coords(
        ['d'],
        graph=graph,
        keep_aliases=True,
        keep_inputs=keep_inputs,
        keep_intermediate=keep_intermediate,
    )
    assert 'c' in da.coords
    assert not da.coords['c'].aligned
    assert 'c' in da.bins.coords
    assert not da.bins.coords['c'].bins.aligned
    assert 'd' in da.coords
    assert 'd' in da.bins.coords


@pytest.mark.parametrize('keep_inputs', [True, False])
@pytest.mark.parametrize('keep_intermediate', [True, False])
def test_not_keep_aliases_binned(binned_in_a_b, keep_inputs, keep_intermediate) -> None:
    graph = {'d': 'c', 'c': 'ab', 'ab': ab}
    da = binned_in_a_b.transform_coords(
        ['d'],
        graph=graph,
        keep_aliases=False,
        keep_inputs=keep_inputs,
        keep_intermediate=keep_intermediate,
    )
    assert 'c' not in da.coords
    assert 'c' not in da.bins.coords
    assert 'd' in da.coords
    assert 'd' in da.bins.coords

    # Requesting input as target preserves it.
    da = binned_in_a_b.transform_coords(
        ['d', 'c'],
        graph=graph,
        keep_aliases=False,
        keep_inputs=keep_inputs,
        keep_intermediate=keep_intermediate,
    )
    assert 'c' in da.coords
    assert 'c' in da.bins.coords
    assert 'd' in da.coords
    assert 'd' in da.bins.coords


@pytest.mark.parametrize('keep_aliases', [True, False])
@pytest.mark.parametrize('keep_intermediate', [True, False])
def test_keep_inputs_dense(a, b, keep_aliases, keep_intermediate) -> None:
    original = sc.DataArray(data=a + b, coords={'a': a, 'b': b})
    graph = {'c': 'ab', 'ab': ab}
    da = original.transform_coords(
        ['c'],
        graph=graph,
        keep_aliases=keep_aliases,
        keep_inputs=True,
        keep_intermediate=keep_intermediate,
    )
    assert 'a' in da.coords
    assert not da.coords['a'].aligned
    assert 'b' in da.coords
    assert not da.coords['b'].aligned


@pytest.mark.parametrize('keep_aliases', [True, False])
@pytest.mark.parametrize('keep_intermediate', [True, False])
def test_not_keep_inputs_dense(a, b, keep_aliases, keep_intermediate) -> None:
    original = sc.DataArray(data=a + b, coords={'a': a, 'b': b})
    graph = {'c': 'ab', 'ab': ab}
    da = original.transform_coords(
        ['c'],
        graph=graph,
        keep_aliases=keep_aliases,
        keep_inputs=False,
        keep_intermediate=keep_intermediate,
    )
    assert 'a' not in da.coords
    assert 'b' not in da.coords

    # Requesting input as target preserves it.
    da = original.transform_coords(
        ['c', 'a'],
        graph=graph,
        keep_aliases=keep_aliases,
        keep_inputs=False,
        keep_intermediate=keep_intermediate,
    )
    assert 'a' in da.coords
    assert da.coords['a'].aligned
    assert 'b' not in da.coords


@pytest.mark.parametrize('keep_aliases', [True, False])
@pytest.mark.parametrize('keep_intermediate', [True, False])
def test_keep_inputs_binned(binned_in_a_b, keep_aliases, keep_intermediate) -> None:
    graph = {'c': 'ab', 'ab': ab}
    da = binned_in_a_b.transform_coords(
        ['c'],
        graph=graph,
        keep_aliases=keep_aliases,
        keep_inputs=True,
        keep_intermediate=keep_intermediate,
    )
    assert 'a' in da.coords
    assert not da.coords['a'].aligned
    assert 'a' in da.bins.coords
    assert not da.bins.coords['a'].bins.aligned
    assert 'b' in da.coords
    assert not da.coords['b'].aligned
    assert 'b' in da.bins.coords
    assert not da.bins.coords['b'].bins.aligned


@pytest.mark.parametrize('keep_aliases', [True, False])
@pytest.mark.parametrize('keep_intermediate', [True, False])
def test_not_keep_inputs_binned(binned_in_a_b, keep_aliases, keep_intermediate) -> None:
    graph = {'c': 'ab', 'ab': ab}
    da = binned_in_a_b.transform_coords(
        ['c'],
        graph=graph,
        keep_aliases=keep_aliases,
        keep_inputs=False,
        keep_intermediate=keep_intermediate,
    )
    assert 'a' not in da.coords
    assert 'a' not in da.bins.coords
    assert 'b' not in da.coords
    assert 'b' not in da.bins.coords

    # Requesting input as target preserves it.
    da = binned_in_a_b.transform_coords(
        ['c', 'a'],
        graph=graph,
        keep_aliases=keep_aliases,
        keep_inputs=False,
        keep_intermediate=keep_intermediate,
    )
    assert 'a' in da.coords
    assert 'a' in da.bins.coords
    assert 'b' not in da.coords
    assert 'b' not in da.bins.coords


@pytest.mark.parametrize('keep_aliases', [True, False])
@pytest.mark.parametrize('keep_inputs', [True, False])
def test_keep_intermediate_dense(a, b, keep_aliases, keep_inputs) -> None:
    original = sc.DataArray(data=a + b, coords={'a': a, 'b': b})
    graph = {'c': 'ab', 'ab': ab}
    da = original.transform_coords(
        ['c'],
        graph=graph,
        keep_aliases=keep_aliases,
        keep_inputs=keep_inputs,
        keep_intermediate=True,
    )
    assert 'ab' in da.coords
    assert not da.coords['ab'].aligned


@pytest.mark.parametrize('keep_aliases', [True, False])
@pytest.mark.parametrize('keep_inputs', [True, False])
def test_not_keep_intermediate_dense(a, b, keep_aliases, keep_inputs) -> None:
    original = sc.DataArray(data=a + b, coords={'a': a, 'b': b})
    graph = {'c': 'ab', 'ab': ab}
    da = original.transform_coords(
        ['c'],
        graph=graph,
        keep_aliases=keep_aliases,
        keep_inputs=keep_inputs,
        keep_intermediate=False,
    )
    assert 'ab' not in da.coords

    # Requesting intermediate as target preserves it.
    da = original.transform_coords(
        ['c', 'ab'],
        graph=graph,
        keep_aliases=keep_aliases,
        keep_inputs=keep_inputs,
        keep_intermediate=False,
    )
    assert 'ab' in da.coords
    assert da.coords['ab'].aligned


@pytest.mark.parametrize('keep_aliases', [True, False])
@pytest.mark.parametrize('keep_inputs', [True, False])
def test_keep_intermediate_binned(binned_in_a_b, keep_aliases, keep_inputs) -> None:
    graph = {'c': 'ab', 'ab': ab}
    da = binned_in_a_b.transform_coords(
        ['c'],
        graph=graph,
        keep_aliases=keep_aliases,
        keep_inputs=keep_inputs,
        keep_intermediate=True,
    )
    assert 'ab' in da.coords
    assert not da.coords['ab'].aligned
    assert 'ab' in da.bins.coords
    assert not da.bins.coords['ab'].bins.aligned


@pytest.mark.parametrize('keep_aliases', [True, False])
@pytest.mark.parametrize('keep_inputs', [True, False])
def test_not_keep_intermediate_binned(binned_in_a_b, keep_aliases, keep_inputs) -> None:
    graph = {'c': 'ab', 'ab': ab}
    da = binned_in_a_b.transform_coords(
        ['c'],
        graph=graph,
        keep_aliases=keep_aliases,
        keep_inputs=keep_inputs,
        keep_intermediate=False,
    )
    assert 'ab' not in da.coords
    assert 'ab' not in da.bins.coords

    # Requesting intermediate as target preserves it.
    da = binned_in_a_b.transform_coords(
        ['c', 'ab'],
        graph=graph,
        keep_aliases=keep_aliases,
        keep_inputs=keep_inputs,
        keep_intermediate=False,
    )
    assert 'ab' in da.coords
    assert da.coords['ab'].aligned
    assert 'ab' in da.bins.coords
    assert da.bins.coords['ab'].bins.aligned


def test_inplace(c) -> None:
    original = sc.DataArray(data=1 * c, coords={'a': 2 * c, 'b': 3 * c})

    # This is maybe not recommended usage, but it works.
    def ab_inplace(*, a, b):
        a += b
        return a

    da = original.copy().transform_coords(['ab'], graph={'ab': ab_inplace})
    assert da.dims == ('c',)
    assert sc.identical(da.coords['ab'], 2 * c + 3 * c)
    assert sc.identical(da.coords['a'], 2 * c + 3 * c)
    assert not da.coords['a'].aligned


def test_dataset_works_with_matching_coord(a) -> None:
    da = sc.DataArray(data=a.copy(), coords={'a': a.copy()})
    ds = sc.Dataset({'item1': da.copy(), 'item2': da + da})
    transformed = ds.transform_coords('b', graph={'b': 'a'})
    assert sc.identical(transformed['item1'].coords['a'], a.rename_dims({'a': 'b'}))
    assert sc.identical(transformed.coords['b'], a.rename_dims({'a': 'b'}))


def test_binned_does_not_modify_inputs(binned_in_a_b) -> None:
    _ = binned_in_a_b.transform_coords(['b2'], graph={'b2': 'b'})
    assert 'b' in binned_in_a_b.coords
    assert 'b' in binned_in_a_b.bins.coords
    assert 'b2' not in binned_in_a_b.coords
    assert 'b2' not in binned_in_a_b.bins.coords


def test_binned_with_range_slice_does_not_modify_inputs(binned_in_a_b) -> None:
    # If input is sliced, transform_coords has to copy the buffer
    original = binned_in_a_b['a', 0:1]
    _ = original.transform_coords(['b2'], graph={'b2': 'b'})
    assert 'b' in original.coords
    assert 'b' in original.bins.coords
    assert 'b2' not in original.coords
    assert 'b2' not in original.bins.coords


def test_binned_with_range_slice_does_not_modify_inputs_copy(binned_in_a_b) -> None:
    # If input is sliced, transform_coords has to copy the buffer.
    # The copy introduces a mismatch in bin indices of the new coord and DataArray.
    original = binned_in_a_b['a', 1:2]
    _ = original.transform_coords(['b2'], graph={'b2': lambda b: b.copy()})
    assert 'b' in original.coords
    assert 'b' in original.bins.coords
    assert 'b2' not in original.coords
    assert 'b2' not in original.bins.coords


def test_binned_with_point_slice_does_not_modify_inputs(binned_in_a_b) -> None:
    # If input is sliced, transform_coords has to copy the buffer
    original = binned_in_a_b['a', 1]
    _ = original.transform_coords(['b2'], graph={'b2': 'b'})
    assert 'b' in original.coords
    assert 'b' in original.bins.coords
    assert 'b2' not in original.coords
    assert 'b2' not in original.bins.coords


def test_binned_computes_correct_results(binned_in_a_b) -> None:
    def convert(*, a2, b):
        return a2 * b

    graph = {'a*b': convert, 'a2': 'a'}
    converted = binned_in_a_b.transform_coords(['a*b'], graph=graph)
    renamed = binned_in_a_b.rename_dims({'a': 'a2'})

    # `a` was renamed to `a2`
    assert sc.identical(converted.coords['a2'], renamed.coords['a'])
    assert sc.identical(converted.bins.coords['a2'], renamed.bins.coords['a'])

    # `a*b` is indeed the product of `a` and `b`
    assert sc.identical(
        converted.coords['a*b'], renamed.coords['a'] * renamed.coords['b']
    )
    assert sc.identical(
        converted.bins.coords['a*b'],
        renamed.bins.coords['a'] * renamed.bins.coords['b'],
    )


@pytest.mark.parametrize('slice_', [slice(0, 2), slice(2, 4)])
def test_binned_slice_computes_correct_results(slice_) -> None:
    events = sc.DataArray(
        sc.ones(dims=['event'], shape=[10]), coords={'x': sc.arange('event', 10.0)}
    )
    da = sc.bin(events, x=sc.arange('x', 0.0, 10.0, 2.0))
    sliced = da['x', slice_]

    assert sc.identical(
        sliced.transform_coords('y', graph={'y': lambda x: 2 * x}),
        sliced.copy().transform_coords('y', graph={'y': lambda x: 2 * x}),
    )


def test_binned_without_bin_coord_computes_correct_results(binned_in_a_b) -> None:
    def convert(*, a, b2):
        return a * b2

    graph = {'a*b': convert, 'b2': 'b'}
    del binned_in_a_b.coords['b']
    converted = binned_in_a_b.transform_coords(['a*b'], graph=graph)
    renamed = binned_in_a_b.rename_dims({'b': 'b2'})

    # `b` was renamed to `b2`
    assert sc.identical(converted.bins.coords['b2'], renamed.bins.coords['b'])

    # `a*b` is indeed the product of `a` and `b`
    assert sc.identical(
        converted.bins.coords['a*b'],
        renamed.bins.coords['a'] * renamed.bins.coords['b'],
    )


def test_binned_without_event_coord_computes_correct_results(binned_in_a_b) -> None:
    def convert(*, a, b2):
        return a * b2

    graph = {'a*b': convert, 'b2': 'b'}
    del binned_in_a_b.bins.coords['b']
    converted = binned_in_a_b.transform_coords(['a*b'], graph=graph)
    renamed = binned_in_a_b.rename_dims({'b': 'b2'})

    # `b` was renamed to `b2`
    assert sc.identical(converted.coords['b2'], renamed.coords['b'])

    # `a*b` is indeed the product of `a` and `b`
    assert sc.identical(
        converted.coords['a*b'], renamed.coords['a'] * renamed.coords['b']
    )


def make_binned():
    N = 50
    data = sc.DataArray(
        data=sc.ones(dims=['event'], unit=sc.units.counts, shape=[N]),
        coords={
            'x': sc.array(dims=['event'], unit=sc.units.m, values=np.random.rand(N)),
            'y': sc.array(dims=['event'], unit=sc.units.m, values=np.random.rand(N)),
        },
    )
    xbins = sc.Variable(dims=['x'], unit=sc.units.m, values=[0.1, 0.5, 0.9])
    ybins = sc.Variable(dims=['y'], unit=sc.units.m, values=[0.1, 0.5, 0.9])
    binned = data.bin(x=xbins, y=ybins)
    del binned.bins.coords['y']
    del binned.coords['y']
    binned.coords['y'] = sc.arange(dim='y', start=0, stop=2)
    return binned


def test_binned_request_existing_consumed() -> None:
    binned = make_binned()

    def xy(*, x, y):
        return x * y

    def unused(x):
        pass

    graph = {'xy': xy, 'x': unused}
    # Regression test, this used to throw due to an untested branch when
    # requesting a previously consumed event coord
    binned.transform_coords(['xy', 'x'], graph=graph)


def test_only_outputs_in_graph_are_stored(a) -> None:
    original = sc.DataArray(data=a, coords={'a': a})
    graph = {'b': split}
    da = original.transform_coords(['b'], graph=graph)
    assert 'c' not in da.coords  # c is not stored
    with pytest.raises(KeyError):
        # c is not computable
        original.transform_coords(['c'], graph=graph)


def test_targets_arg_types(a, b) -> None:
    original = sc.DataArray(data=a + b, coords={'a': a, 'b': b})
    graph = {'ab': ab}
    da = original.transform_coords(('ab',), graph=graph)
    assert 'ab' in da.coords
    da = original.transform_coords('ab', graph=graph)
    assert 'ab' in da.coords


def test_raises_when_requested_coord_not_computed(a) -> None:
    def foo(a):
        return {'actual': a}

    original = sc.DataArray(data=a, coords={'a': a})
    with pytest.raises(TypeError):
        original.transform_coords('expected', graph={'expected': foo})


def test_raises_when_expected_multiple_outputs_but_returned_non_dict(a) -> None:
    def foo(a):
        return a

    graph = {('b', 'c'): foo}
    original = sc.DataArray(data=a, coords={'a': a})
    with pytest.raises(TypeError):
        original.transform_coords(['b', 'c'], graph=graph)


def test_inaccessible_coord(a, b) -> None:
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


def test_cycles(a) -> None:
    original = sc.DataArray(data=a, coords={'a': a, 'b': a})
    with pytest.raises(CycleError):
        original.transform_coords(['c'], graph={'c': 'c'})
    with pytest.raises(CycleError):
        original.transform_coords(['c'], graph={'c': 'd', 'd': 'c'})
    with pytest.raises(CycleError):
        original.transform_coords(['c'], graph={'c': 'd', 'd': 'e', 'e': 'c'})
    with pytest.raises(CycleError):
        original.transform_coords(['c'], graph={'c': bc})
    with pytest.raises(CycleError):
        original.transform_coords(['c'], graph={'c': 'd', 'd': bc})


def test_new_dim_in_coord(a) -> None:
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


def test_vararg_fail(a) -> None:
    original = sc.DataArray(data=a, coords={'a': a})

    def func(*args):
        pass

    with pytest.raises(ValueError, match='variable arguments'):
        original.transform_coords(['b'], graph={'b': func})

    def func(**kwargs):
        pass

    with pytest.raises(ValueError, match='variable arguments'):
        original.transform_coords(['b'], graph={'b': func})


def test_arg_vs_kwarg_kwonly(a) -> None:
    def arg(a):
        return a

    def kwarg(a=None):
        return a

    def kwonly(*, a):
        return a

    original = sc.DataArray(data=a, coords={'a': a})
    da = original.transform_coords(
        ['b', 'c', 'd'], graph={'b': arg, 'c': kwarg, 'd': kwonly}
    )
    assert sc.identical(da.coords['b'], original.coords['a'])
    assert sc.identical(da.coords['c'], original.coords['a'])
    assert sc.identical(da.coords['d'], original.coords['a'])


def test_unconsumed_outputs(a) -> None:
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


def test_duplicate_output_keys(a) -> None:
    original = sc.DataArray(data=a, coords={'a': a})

    def to_bc(*, a):
        return {'b': a, 'c': a}

    def to_bd(*, a):
        return {'b': a, 'd': a}

    graph = {'b': 'a', ('b', 'c'): to_bc}
    with pytest.raises(ValueError, match='Duplicate output name'):
        original.transform_coords(['b'], graph=graph)
    graph = {('b', 'd'): to_bd, ('b', 'c'): to_bc}
    with pytest.raises(ValueError, match='Duplicate output name'):
        original.transform_coords(['b'], graph=graph)
    # b only named once in graph
    graph = {'b': to_bd, 'c': to_bc}
    da = original.transform_coords(['b'], graph=graph)
    assert 'b' in da.coords


def test_keyword_syntax_equivalent_to_explicit_syntax() -> None:
    da = sc.data.table_xyz(nrow=10)

    def a(x):
        return x + x

    def b(x, y):
        return x + y

    assert sc.identical(da.transform_coords(a=a), da.transform_coords('a', {'a': a}))
    graph = {'a': a, 'b': b}
    assert sc.identical(
        da.transform_coords(a=a, b=b), da.transform_coords(['a', 'b'], graph=graph)
    )


def test_keyword_syntax_without_entries_returns_unchanged() -> None:
    da = sc.data.table_xyz(nrow=10)
    assert sc.identical(da.transform_coords(), da)


def test_keyword_syntax_without_entries_and_graph_returns_unchanged() -> None:
    da = sc.data.table_xyz(nrow=10)
    assert sc.identical(da.transform_coords(graph={'b': 'y'}), da)


def test_raises_when_keyword_syntax_combined_with_targets() -> None:
    da = sc.data.table_xyz(nrow=10)
    with pytest.raises(ValueError, match='Explicit targets or graph'):
        da.transform_coords('a', a=lambda x: x)


def test_raises_when_keyword_syntax_combined_with_graph() -> None:
    da = sc.data.table_xyz(nrow=10)
    with pytest.raises(ValueError, match='Explicit targets or graph'):
        da.transform_coords(a=lambda x: x, graph={'b': 'y'})


def test_raises_when_keyword_syntax_clashes_with_graph_argument() -> None:
    da = sc.data.table_xyz(nrow=10)
    with pytest.raises(TypeError):
        da.transform_coords('x', graph=lambda x: x)


@pytest.mark.parametrize(
    'option',
    ['rename_dims', 'keep_aliases', 'keep_intermediate', 'keep_inputs', 'quiet'],
)
def test_raises_when_keyword_syntax_clashes_with_options(option) -> None:
    da = sc.data.table_xyz(nrow=10)
    with pytest.raises(TypeError):
        da.transform_coords(**{option: lambda x: x})


def test_works_with_partial() -> None:
    def f(a, x):
        return a * x

    g = functools.partial(f, sc.scalar(5))
    da = sc.data.table_xyz(nrow=10)
    assert 'ax' in da.transform_coords(ax=g).coords


def test_works_with_class_defining___call__() -> None:
    class A:
        def __call__(self, x):
            return x * x

    da = sc.data.table_xyz(nrow=10)
    assert 'xx' in da.transform_coords(xx=A()).coords


def test_input_coords_can_be_defined_via_property() -> None:
    def f(a, b):
        return a - b

    f.__transform_coords_input_keys__ = ['x', 'y']
    da = sc.data.table_xyz(nrow=10)
    assert sc.identical(
        da.transform_coords(diff=f), da.transform_coords(diff=lambda x, y: f(x, y))
    )
