# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen
import scipp as sc


def test_find_bin_edge_dims_coord_of_same_length():
    da = sc.DataArray(sc.zeros(sizes={'a': 2, 'b': 3}))
    a = sc.ones(sizes={'a': 2})
    assert sc.utils.find_bin_edge_dims(da, coord=a) == set()
    b = sc.ones(sizes={'b': 3})
    assert sc.utils.find_bin_edge_dims(da, coord=b) == set()


def test_find_bin_edge_dims_coord_of_length_plut_1():
    da = sc.DataArray(sc.zeros(sizes={'a': 2, 'b': 3}))
    a = sc.ones(sizes={'a': 3})
    assert sc.utils.find_bin_edge_dims(da, coord=a) == {'a'}
    b = sc.ones(sizes={'b': 4})
    assert sc.utils.find_bin_edge_dims(da, coord=b) == {'b'}


def test_find_bin_edge_dims_2d_coord_without_bin_edge():
    da = sc.DataArray(sc.zeros(sizes={'a': 2, 'b': 3}))
    a = sc.ones(sizes={'a': 2, 'b': 3})
    assert sc.utils.find_bin_edge_dims(da, coord=a) == set()


def test_find_bin_edge_dims_2d_coord_with_bin_edge():
    da = sc.DataArray(sc.zeros(sizes={'a': 2, 'b': 3}))
    x = sc.ones(sizes={'a': 3, 'b': 3})
    assert sc.utils.find_bin_edge_dims(da, coord=x) == {'a'}
    x = sc.ones(sizes={'a': 2, 'b': 4})
    assert sc.utils.find_bin_edge_dims(da, coord=x) == {'b'}


# Such a coord is not supported by DataArray and Dataset,
# but we can still find both bin edges.
def test_find_bin_edge_dims_2d_coord_with_2_bin_edges():
    da = sc.DataArray(sc.zeros(sizes={'a': 2, 'b': 3}))
    x = sc.ones(sizes={'a': 3, 'b': 4})
    assert sc.utils.find_bin_edge_dims(da, coord=x) == {'a', 'b'}


def test_find_bin_edge_dims_dim_not_in_da():
    da = sc.DataArray(sc.zeros(sizes={'a': 2, 'b': 3}))
    x = sc.ones(sizes={'c': 3})
    assert sc.utils.find_bin_edge_dims(da, coord=x) == set()


def test_find_bin_edge_dims_scalar_da():
    da = sc.DataArray(sc.scalar(0))
    x = sc.ones(sizes={'a': 2})
    assert sc.utils.find_bin_edge_dims(da, coord=x) == {'a'}
    x = sc.ones(sizes={'a': 1})
    assert sc.utils.find_bin_edge_dims(da, coord=x) == set()
    x = sc.ones(sizes={'a': 0})
    assert sc.utils.find_bin_edge_dims(da, coord=x) == set()
