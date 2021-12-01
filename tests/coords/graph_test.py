# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen

import scipp.coords.graph as scgraph


def graph_0():
    def fd(a, b, c):
        pass

    def fe(a, b):
        pass

    def ff(a, b):
        pass

    def fg(c, e):
        pass

    return scgraph.Graph({'d': fd, 'e': fe, 'f': ff, 'g': fg})


def test_children_of():
    graph = graph_0()
    assert set(graph.children_of('a')) == {'d', 'e', 'f'}
    assert set(graph.children_of('b')) == {'d', 'e', 'f'}
    assert set(graph.children_of('c')) == {'d', 'g'}
    assert set(graph.children_of('d')) == set()
    assert set(graph.children_of('e')) == {'g'}
    assert set(graph.children_of('f')) == set()
    assert set(graph.children_of('g')) == set()


def test_parents_of():
    graph = graph_0()
    assert set(graph.parents_of('a')) == set()
    assert set(graph.parents_of('b')) == set()
    assert set(graph.parents_of('c')) == set()
    assert set(graph.parents_of('d')) == {'a', 'b', 'c'}
    assert set(graph.parents_of('e')) == {'a', 'b'}
    assert set(graph.parents_of('f')) == {'a', 'b'}
    assert set(graph.parents_of('g')) == {'c', 'e'}


def test_neighbors_of():
    graph = graph_0()
    for node in ('a', 'b', 'c', 'd', 'e', 'f', 'g'):
        assert set(graph.neighbors_of(node)) == (set(graph.children_of(node))
                                                 | set(graph.parents_of(node)))
