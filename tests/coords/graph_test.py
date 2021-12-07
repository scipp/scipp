# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen

import pytest
import scipp.coords.graph as scgraph
from scipp.coords.rule import ComputeRule, FetchRule, RenameRule
import scipp as sc


def graph_0():
    r"""
    a b c
    |╳| |
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

    return scgraph.RuleGraph({'d': fd, 'e': fe, 'f': ff, 'g': 'e'})


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

    return scgraph.RuleGraph({'d': fd, 'b': 'a', 'c': 'a'})


def cycles_1():
    return {
        'abcd': scgraph.Cycle(nodes={'a', 'b', 'c', 'd'}, inputs={'a'}, outputs={'d'})
    }


def graph_2():
    r"""
    a
    |
    b   c d
    |\ /╳ |
    | e   f
    g_____|
    """
    def fe(b, d, c):
        pass

    def ff(c, d):
        pass

    def fg(b, f):
        pass

    return scgraph.RuleGraph({'b': 'a', 'e': fe, 'f': ff, 'g': fg})


def cycles_2():
    return {
        'bcefg':
        scgraph.Cycle(nodes={'b', 'e', 'c', 'f', 'g'},
                      inputs={'b', 'c'},
                      outputs={'e', 'g'}),
        'bdefg':
        scgraph.Cycle(nodes={'b', 'e', 'd', 'f', 'g'},
                      inputs={'b', 'd'},
                      outputs={'e', 'g'})
    }


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

    return scgraph.RuleGraph({
        'b': 'a',
        ('c', 'f'): fcf,
        'd': fd,
        'g': fg,
        'h': fh,
        'i': fi
    })


def cycles_3():
    return {
        'bcdfg':
        scgraph.Cycle(nodes={'b', 'c', 'd', 'f', 'g'}, inputs={'b'}, outputs={'g'}),
        'bcdfhi':
        scgraph.Cycle(nodes={'b', 'c', 'd', 'f', 'h', 'i'}, inputs={'b'},
                      outputs={'i'}),
        'dfghi':
        scgraph.Cycle(nodes={'d', 'f', 'g', 'h', 'i'},
                      inputs={'d', 'f'},
                      outputs={'g', 'i'})
    }


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

    After contracting a,b,c,d, this graph produces a cycle between 2 nodes:
        a
        |
    e<->bc
        |
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

    return scgraph.RuleGraph({'b': fb, 'c': fc, 'd': fd, 'e': fe})


def cycles_4():
    return {
        'abcd': scgraph.Cycle(nodes={'a', 'b', 'c', 'd'}, inputs={'a'}, outputs={'d'}),
        'bce': scgraph.Cycle(nodes={'b', 'c', 'e'}, inputs={'b'}, outputs={'c'}),
        'abcde': scgraph.Cycle(nodes={'a', 'b', 'c', 'd', 'e'},
                               inputs={'a'},
                               outputs={'d'})
    }


def make_data(coords, dims=('x', )):
    return sc.DataArray(sc.ones(dims=dims, shape=[1] * len(dims)),
                        coords={
                            name: sc.ones(dims=[name] if name in dims else dims[:1],
                                          shape=[1])
                            for name in coords
                        })


def test_children_of():
    graph = graph_0().dependency_graph
    assert set(graph.children_of('a')) == {'d', 'e'}
    assert set(graph.children_of('b')) == {'d', 'e'}
    assert set(graph.children_of('c')) == {'f'}
    assert set(graph.children_of('d')) == {'f'}
    assert set(graph.children_of('e')) == {'g'}
    assert set(graph.children_of('f')) == set()
    assert set(graph.children_of('g')) == set()


def test_parents_of():
    graph = graph_0().dependency_graph
    assert set(graph.parents_of('a')) == set()
    assert set(graph.parents_of('b')) == set()
    assert set(graph.parents_of('c')) == set()
    assert set(graph.parents_of('d')) == {'a', 'b'}
    assert set(graph.parents_of('e')) == {'a', 'b'}
    assert set(graph.parents_of('f')) == {'d', 'c'}
    assert set(graph.parents_of('g')) == {'e'}


def test_neighbors_of():
    graph = graph_0().dependency_graph
    for node in ('a', 'b', 'c', 'd', 'e', 'f', 'g'):
        assert set(graph.neighbors_of(node)) == (set(graph.children_of(node))
                                                 | set(graph.parents_of(node)))


def assert_rule(graph, node, rule_type, dependencies):
    rule = graph[node]
    assert isinstance(rule, rule_type)
    assert set(rule.dependencies) == dependencies


def test_graph_for_graph_0():
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


def test_graph_for_graph_1():
    base_graph = graph_1()
    da = make_data(('a', ))

    graph = base_graph.graph_for(da, {'b'})
    assert_rule(graph, 'a', FetchRule, set())
    assert_rule(graph, 'b', RenameRule, {'a'})

    graph = base_graph.graph_for(da, {'d'})
    assert_rule(graph, 'a', FetchRule, set())
    assert_rule(graph, 'b', RenameRule, {'a'})
    assert_rule(graph, 'c', RenameRule, {'a'})
    assert_rule(graph, 'd', ComputeRule, {'b', 'c'})


def test_graph_for_graph_3():
    base_graph = graph_3()
    da = make_data(('a', 'e'))

    graph = base_graph.graph_for(da, {'d'})
    assert_rule(graph, 'a', FetchRule, set())
    assert_rule(graph, 'b', RenameRule, {'a'})
    assert_rule(graph, 'c', ComputeRule, {'b'})
    # f is produced by the same rule as c but not required to make d.
    with pytest.raises(KeyError):
        assert graph['f']
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


def test_undirected_cycles_graph_0():
    da = make_data(('a', 'b', 'c'))
    graph = graph_0()
    assert not graph.dependency_graph.undirected_cycles()
    assert not graph.graph_for(da, {'f', 'g'}).dependency_graph.undirected_cycles()


def test_fully_contract_cycles_graph_0():
    graph = graph_0().dependency_graph
    assert graph.fully_contract_cycles() == graph


def test_undirected_cycles_graph_1():
    da = make_data(('a', ))
    graph = graph_1()
    expected = set(cycles_1().values())
    assert graph.dependency_graph.undirected_cycles() == expected
    assert graph.graph_for(da, {'d'}).dependency_graph.undirected_cycles() == expected


def test_contract_cycle_graph_1():
    # Contracted: a -> cycle_b_c -> d
    graph = graph_1().dependency_graph
    cycle = cycles_1()['abcd']
    contracted = graph.contract_cycle(cycle)
    assert len(contracted['d']) == 1
    assert contracted[next(iter(contracted['d']))] == {'a'}


def test_fully_contract_cycles_graph_1():
    graph = graph_1().dependency_graph
    contracted = graph.fully_contract_cycles()
    assert len(contracted['d']) == 1
    cycle_node = next(iter(contracted['d']))
    assert contracted[cycle_node] == {'a'}


def test_undirected_cycles_graph_2():
    da = make_data(('a', 'c', 'd'))
    graph = graph_2()
    expected = set(cycles_2().values())
    assert graph.dependency_graph.undirected_cycles() == expected
    assert not graph.graph_for(da, {'g'}).dependency_graph.undirected_cycles()
    assert graph.graph_for(da,
                           {'e', 'g'}).dependency_graph.undirected_cycles() == expected


def test_contract_cycle_graph_2():
    graph = graph_2().dependency_graph

    cycle = cycles_2()['bcefg']
    contracted = graph.contract_cycle(cycle)
    assert len(contracted['g']) == 1
    cycle_node = next(iter(contracted['g']))
    assert cycle_node != 'f'  # not the old node
    expected = scgraph.Graph({
        'b': {'a'},
        cycle_node: {'b', 'c', 'd'},
        'e': {cycle_node, 'd'},
        'g': {cycle_node}
    })
    assert contracted == expected


def test_fully_contract_cycles_graph_2():
    graph = graph_2().dependency_graph
    contracted = graph.fully_contract_cycles()
    assert len(contracted['g']) == 1
    cycle_node = next(iter(contracted['g']))
    expected = scgraph.Graph({
        'b': {'a'},
        cycle_node: {'b', 'c', 'd'},
        'e': {cycle_node},
        'g': {cycle_node}
    })
    assert contracted == expected


def test_undirected_cycles_graph_3():
    da = make_data(('a', 'e'))
    graph = graph_3()
    expected = set(cycles_3().values())
    assert graph.dependency_graph.undirected_cycles() == expected
    assert not graph.graph_for(da, {'d'}).dependency_graph.undirected_cycles()
    assert graph.graph_for(da,
                           {'g', 'i'}).dependency_graph.undirected_cycles() == expected


def test_contract_cycle_graph_3():
    graph = graph_3().dependency_graph

    cycle = cycles_3()['bcdfg']
    contracted = graph.contract_cycle(cycle)
    assert len(contracted['g']) == 1
    cycle_node = next(iter(contracted['g']))
    expected = scgraph.Graph({
        'b': {'a'},
        cycle_node: {'b', 'e'},
        'g': {cycle_node},
        'h': {cycle_node},
        'i': {cycle_node, 'h'},
    })
    assert contracted == expected


def test_fully_contract_cycles_graph_3():
    # There are 3 possible contractions. Test only whether the
    # resulting graph makes sense, not whether it is 'correct'.
    graph = graph_3().dependency_graph
    contracted = graph.fully_contract_cycles()
    assert len(contracted['g']) == 1
    assert len(contracted['i']) == 1
    assert not contracted.undirected_cycles()


def test_undirected_cycles_graph_4():
    da = make_data(('a', ))
    graph = graph_4()
    expected = cycles_4()
    assert graph.dependency_graph.undirected_cycles() == set(expected.values())
    assert graph.graph_for(da, {'d'}).dependency_graph.undirected_cycles() == set(
        expected.values())
    assert graph.graph_for(
        da, {'c'}).dependency_graph.undirected_cycles() == {expected['bce']}

    # Finds the cycle e <-> cycle_b_c after contraction.
    contracted = graph.dependency_graph.contract_cycle(expected['abcd'])
    cycle_node = next(iter(contracted['d']))
    assert contracted.undirected_cycles() == {
        scgraph.Cycle(nodes={cycle_node, 'e'}, inputs=set(), outputs=set())
    }


def test_contract_cycle_graph_4():
    graph = graph_4().dependency_graph

    # Contracted:
    #            a
    #            |
    #   e <-> cycle_b_c
    #            |
    #            d
    cycle = cycles_4()['abcd']
    contracted = graph.contract_cycle(cycle)
    assert len(contracted['d']) == 1
    cycle_node = next(iter(contracted['d']))
    assert contracted[cycle_node] == {'a', 'e'}
    assert contracted['e'] == {cycle_node}

    # Twice contracted:
    #        a
    #        |
    #   cycle_b_c_e
    #        |
    #        d
    cycle = scgraph.Cycle(nodes={cycle_node, 'e'}, inputs=set(), outputs=set())
    twice_contracted = contracted.contract_cycle(cycle)
    assert len(twice_contracted['d']) == 1
    cycle_node = next(iter(twice_contracted['d']))
    assert twice_contracted[cycle_node] == {'a'}


def test_fully_contract_cycles_graph_4():
    graph = graph_4().dependency_graph
    contracted = graph.fully_contract_cycles()
    assert len(contracted['d']) == 1
    cycle_node = next(iter(contracted['d']))
    expected = scgraph.Graph({'d': {cycle_node}, cycle_node: {'a'}})
    assert contracted == expected
