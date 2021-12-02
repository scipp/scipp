# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen

import scipp.coords.graph as scgraph
from scipp.coords.rule import ComputeRule, FetchRule, RenameRule
import scipp as sc


def graph_0():
    r"""
    a b c
    |╳| |
    f e |
    |  \|
    h   g
    """
    def fd(a, b, c):
        pass

    def fe(a, b):
        pass

    def ff(a, b):
        pass

    def fg(c, e):
        pass

    return scgraph.Graph({'d': fd, 'e': fe, 'f': ff, 'g': fg, 'h': 'f'})


def make_data(coords, dims=('x', )):
    return sc.DataArray(sc.ones(dims=dims, shape=[1] * len(dims)),
                        coords={
                            name: sc.ones(dims=[name] if name in dims else dims[:1],
                                          shape=[1])
                            for name in coords
                        })


def test_children_of():
    graph = graph_0()
    assert set(graph.children_of('a')) == {'d', 'e', 'f'}
    assert set(graph.children_of('b')) == {'d', 'e', 'f'}
    assert set(graph.children_of('c')) == {'d', 'g'}
    assert set(graph.children_of('d')) == set()
    assert set(graph.children_of('e')) == {'g'}
    assert set(graph.children_of('f')) == {'h'}
    assert set(graph.children_of('g')) == set()
    assert set(graph.children_of('h')) == set()


def test_parents_of():
    graph = graph_0()
    assert set(graph.parents_of('a')) == set()
    assert set(graph.parents_of('b')) == set()
    assert set(graph.parents_of('c')) == set()
    assert set(graph.parents_of('d')) == {'a', 'b', 'c'}
    assert set(graph.parents_of('e')) == {'a', 'b'}
    assert set(graph.parents_of('f')) == {'a', 'b'}
    assert set(graph.parents_of('g')) == {'c', 'e'}
    assert set(graph.parents_of('h')) == {'f'}


def test_neighbors_of():
    graph = graph_0()
    for node in ('a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'):
        assert set(graph.neighbors_of(node)) == (set(graph.children_of(node))
                                                 | set(graph.parents_of(node)))


def test_graph_for():
    base_graph = graph_0()
    da = make_data(('a', 'b', 'c'))

    graph = base_graph.graph_for(da, {'f'})
    assert isinstance(graph['a'], FetchRule)
    assert isinstance(graph['b'], FetchRule)
    assert isinstance(graph['f'], ComputeRule)
    assert set(graph['f'].dependencies) == {'a', 'b'}

    graph = base_graph.graph_for(da, {'g'})
    assert isinstance(graph['a'], FetchRule)
    assert isinstance(graph['b'], FetchRule)
    assert isinstance(graph['c'], FetchRule)
    assert isinstance(graph['e'], ComputeRule)
    assert set(graph['e'].dependencies) == {'a', 'b'}
    assert isinstance(graph['g'], ComputeRule)
    assert set(graph['g'].dependencies) == {'c', 'e'}

    graph = base_graph.graph_for(da, {'h', 'e'})
    assert isinstance(graph['a'], FetchRule)
    assert isinstance(graph['b'], FetchRule)
    assert isinstance(graph['f'], ComputeRule)
    assert set(graph['f'].dependencies) == {'a', 'b'}
    assert isinstance(graph['e'], ComputeRule)
    assert set(graph['e'].dependencies) == {'a', 'b'}
    assert isinstance(graph['h'], RenameRule)
    assert set(graph['h'].dependencies) == {'f'}
