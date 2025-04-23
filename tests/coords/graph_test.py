# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen

import scipp as sc
import scipp.coords.graph as scgraph
from scipp.coords.rule import ComputeRule, FetchRule, RenameRule


def graph_0():
    r"""
    a b c
    |X| |
    e d |
    |  \|
    g   f
    """

    def fd(a, b):
        pass

    def fe(b, a):
        pass

    def ff(c, d):
        pass

    return scgraph.Graph({'d': fd, 'e': fe, 'f': ff, 'g': 'e'})


def graph_1():
    r"""
      a
     / \
    b   c
     \ /
      d
    """

    def fd(b, c):
        pass

    return scgraph.Graph({'d': fd, 'b': 'a', 'c': 'a'})


def graph_2():
    r"""
    a
    |
    b   c d
    |\ / \|
    | e   f
    g_____|
    """

    def fe(b, d, c):
        pass

    def ff(c, d):
        pass

    def fg(b, f):
        pass

    return scgraph.Graph({'b': 'a', 'e': fe, 'f': ff, 'g': fg})


def graph_3():
    r"""
        a
        |
        b
       / \
    e c   f
     \|  /|
      d  ||
      |\ /|
      h g |
       \ /
        i
    """

    def fcf(b):
        pass

    def fd(c, e):
        pass

    def fg(d, f):
        pass

    def fh(d):
        pass

    def fi(h, f):
        pass

    return scgraph.Graph(
        {'b': 'a', ('c', 'f'): fcf, 'd': fd, 'g': fg, 'h': fh, 'i': fi}
    )


def graph_4():
    r"""
      a
      | \
      b  |
     /|  |
    e |  |
     \|  |
      c  |
      \ /
       d
    """

    def fb(a):
        pass

    def fc(b, e):
        pass

    def fd(a, c):
        pass

    def fe(b):
        pass

    return scgraph.Graph({'b': fb, 'c': fc, 'd': fd, 'e': fe})


def make_data(coords, dims=('x',)):
    return sc.DataArray(
        sc.ones(dims=dims, shape=[1] * len(dims)),
        coords={
            name: sc.ones(dims=[name] if name in dims else dims[:1], shape=[1])
            for name in coords
        },
    )


def test_children_of() -> None:
    graph = graph_0()
    assert set(graph.children_of('a')) == {'d', 'e'}
    assert set(graph.children_of('b')) == {'d', 'e'}
    assert set(graph.children_of('c')) == {'f'}
    assert set(graph.children_of('d')) == {'f'}
    assert set(graph.children_of('e')) == {'g'}
    assert set(graph.children_of('f')) == set()
    assert set(graph.children_of('g')) == set()


def test_parents_of() -> None:
    graph = graph_0()
    assert set(graph.parents_of('a')) == set()
    assert set(graph.parents_of('b')) == set()
    assert set(graph.parents_of('c')) == set()
    assert set(graph.parents_of('d')) == {'a', 'b'}
    assert set(graph.parents_of('e')) == {'a', 'b'}
    assert set(graph.parents_of('f')) == {'d', 'c'}
    assert set(graph.parents_of('g')) == {'e'}


def assert_rule(graph, node, rule_type, dependencies):
    rule = graph[node]
    assert isinstance(rule, rule_type)
    assert set(rule.dependencies) == dependencies


def test_graph_for_graph_0() -> None:
    base_graph = graph_0()
    da = make_data(('a', 'b', 'c'))

    graph = base_graph.graph_for(da, {'e'})
    assert_rule(graph, 'a', FetchRule, set())
    assert_rule(graph, 'b', FetchRule, set())
    assert_rule(graph, 'e', ComputeRule, {'a', 'b'})

    graph = base_graph.graph_for(da, {'f'})
    assert_rule(graph, 'a', FetchRule, set())
    assert_rule(graph, 'b', FetchRule, set())
    assert_rule(graph, 'c', FetchRule, set())
    assert_rule(graph, 'd', ComputeRule, {'a', 'b'})
    assert_rule(graph, 'f', ComputeRule, {'c', 'd'})

    graph = base_graph.graph_for(da, {'d', 'g'})
    assert_rule(graph, 'a', FetchRule, set())
    assert_rule(graph, 'b', FetchRule, set())
    assert_rule(graph, 'd', ComputeRule, {'a', 'b'})
    assert_rule(graph, 'e', ComputeRule, {'a', 'b'})
    assert_rule(graph, 'g', RenameRule, {'e'})


def test_graph_for_graph_0_finds_intermediates() -> None:
    base_graph = graph_0()
    da = make_data(('c', 'd'))

    graph = base_graph.graph_for(da, {'f'})
    assert_rule(graph, 'c', FetchRule, set())
    assert_rule(graph, 'd', FetchRule, set())
    assert_rule(graph, 'f', ComputeRule, {'c', 'd'})
    assert 'a' not in graph.nodes()
    assert 'b' not in graph.nodes()


def test_graph_for_graph_1() -> None:
    base_graph = graph_1()
    da = make_data(('a',))

    graph = base_graph.graph_for(da, {'b'})
    assert_rule(graph, 'a', FetchRule, set())
    assert_rule(graph, 'b', RenameRule, {'a'})

    graph = base_graph.graph_for(da, {'d'})
    assert_rule(graph, 'a', FetchRule, set())
    assert_rule(graph, 'b', RenameRule, {'a'})
    assert_rule(graph, 'c', RenameRule, {'a'})
    assert_rule(graph, 'd', ComputeRule, {'b', 'c'})


def test_graph_for_graph_3() -> None:
    base_graph = graph_3()
    da = make_data(('a', 'e'))

    graph = base_graph.graph_for(da, {'d'})
    assert_rule(graph, 'a', FetchRule, set())
    assert_rule(graph, 'b', RenameRule, {'a'})
    assert_rule(graph, 'c', ComputeRule, {'b'})
    # f is produced by the same rule as c and thus always produced even if not required.
    assert_rule(graph, 'f', ComputeRule, {'b'})
    assert_rule(graph, 'd', ComputeRule, {'c', 'e'})

    graph = base_graph.graph_for(da, {'i', 'g'})
    assert_rule(graph, 'a', FetchRule, set())
    assert_rule(graph, 'b', RenameRule, {'a'})
    assert_rule(graph, 'c', ComputeRule, {'b'})
    assert_rule(graph, 'd', ComputeRule, {'c', 'e'})
    assert_rule(graph, 'e', FetchRule, set())
    assert_rule(graph, 'f', ComputeRule, {'b'})
    assert_rule(graph, 'g', ComputeRule, {'d', 'f'})
    assert_rule(graph, 'h', ComputeRule, {'d'})
    assert_rule(graph, 'i', ComputeRule, {'f', 'h'})
