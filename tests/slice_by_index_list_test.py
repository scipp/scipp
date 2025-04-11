# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import numpy as np
import pytest

import scipp as sc


def make_var() -> sc.Variable:
    v = sc.arange('dummy', 12, dtype='int64')
    return v.fold(dim='dummy', sizes={'xx': 4, 'yy': 3})


def make_array() -> sc.DataArray:
    da = sc.DataArray(make_var())
    da.coords['xx'] = sc.arange('xx', 4, dtype='int64')
    da.coords['yy'] = sc.arange('yy', 3, dtype='int64')
    return da


def make_dataset() -> sc.Dataset:
    return sc.Dataset({'xy': make_array()})


@pytest.fixture(
    params=[make_var(), make_array(), make_dataset()],
    ids=['Variable', 'DataArray', 'Dataset'],
)
def sliceable(
    request: pytest.FixtureRequest,
) -> sc.Variable | sc.DataArray | sc.Dataset:
    return request.param  # type: ignore[no-any-return]


@pytest.mark.parametrize("pos", [0, 1, 2, 3, -2, -3, -4])
def test_length_1_list_gives_corresponding_length_1_slice(
    sliceable: sc.Variable | sc.DataArray | sc.Dataset, pos: int
) -> None:
    assert sc.identical(sliceable['xx', [pos]], sliceable['xx', pos : pos + 1])


def test_slicing_with_numpy_array_works_and_gives_equivalent_result() -> None:
    var = make_var()
    assert sc.identical(var['xx', np.array([2, 3, 0])], var['xx', [2, 3, 0]])


def test_omitting_dim_when_slicing_2d_sliceableect_raises_DimensionError(
    sliceable: sc.Variable | sc.DataArray | sc.Dataset,
) -> None:
    with pytest.raises(sc.DimensionError):
        sliceable[[3, 1]]


def test_omitted_dim_is_equivalent_to_unique_dim(
    sliceable: sc.Variable | sc.DataArray | sc.Dataset,
) -> None:
    sliceable = sliceable['yy', 0]
    assert sc.identical(sliceable[[3, 1]], sliceable[sliceable.dim, [3, 1]])


def test_every_other_index_gives_stride_2_slice(
    sliceable: sc.Variable | sc.DataArray | sc.Dataset,
) -> None:
    assert sc.identical(sliceable['xx', [0, 2]], sliceable['xx', 0::2])
    assert sc.identical(sliceable['xx', [1, 3]], sliceable['xx', 1::2])


def test_unordered_outer_indices_yields_result_with_reordered_slices(
    sliceable: sc.Variable | sc.DataArray | sc.Dataset,
) -> None:
    assert sc.identical(
        sliceable['xx', [2, 3, 0]],
        sc.concat([sliceable['xx', 2:4], sliceable['xx', 0:1]], 'xx'),  # type: ignore[type-var]
    )


def test_unordered_inner_indices_yields_result_with_reordered_slices(
    sliceable: sc.Variable | sc.DataArray | sc.Dataset,
) -> None:
    s0 = sliceable['yy', 0:1]
    s1 = sliceable['yy', 1:2]
    assert sc.identical(
        sliceable['yy', [1, 1, 0, 1]],
        sc.concat([s1, s1, s0, s1], 'yy'),  # type: ignore[type-var]
    )


def test_duplicate_indices_duplicate_slices_in_output(
    sliceable: sc.Variable | sc.DataArray | sc.Dataset,
) -> None:
    s1 = sliceable['xx', 1:2]
    s2 = sliceable['xx', 2:3]
    assert sc.identical(
        sliceable['xx', [2, 1, 1, 2]],
        sc.concat([s2, s1, s1, s2], 'xx'),  # type: ignore[type-var]
    )


def test_reversing_twice_gives_original(
    sliceable: sc.Variable | sc.DataArray | sc.Dataset,
) -> None:
    assert sc.identical(sliceable['xx', [3, 2, 1, 0]]['xx', [3, 2, 1, 0]], sliceable)


@pytest.mark.parametrize("sliceable", [make_array(), make_dataset()])
@pytest.mark.parametrize("what", ["coords", "masks"])
def test_bin_edges_are_dropped(sliceable: sc.DataArray | sc.Dataset, what: str) -> None:
    sliceable = sliceable.copy()
    base = sliceable.copy()
    edges = sc.concat([sliceable.coords['xx'], sliceable.coords['xx'][-1] + 1], 'xx')
    da = (
        sliceable
        if isinstance(sliceable, sc.DataArray) or what == 'coords'
        else sliceable['xy']
    )
    getattr(da, what)['edges'] = edges
    assert sc.identical(
        sliceable['xx', [0, 2, 3]],
        sc.concat([base['xx', 0], base['xx', 2:]], 'xx'),  # type: ignore[type-var]
    )
    da = (
        sliceable
        if isinstance(sliceable, sc.DataArray) or what == 'coords'
        else sliceable['xy']
    )
    assert 'edges' in getattr(da, what)


def test_2d_list_raises_TypeError() -> None:
    var = make_var()
    with pytest.raises(TypeError):
        var['xx', [[0], [2]]]  # type: ignore[index]


@pytest.mark.parametrize("pos", [-6, -5, 4, 5])
def test_out_of_range_index_raises_IndexError(pos: int) -> None:
    var = sc.arange('xx', 4)
    with pytest.raises(IndexError):
        var['xx', [pos]]
