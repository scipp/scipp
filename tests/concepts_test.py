# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import scipp as sc
from scipp.core import concepts


def test_concrete_dims_given_single_dim_returns_dim_as_tuple() -> None:
    var = sc.empty(dims=('xx', 'yy'), shape=(2, 2))
    assert concepts.concrete_dims(var, 'xx') == ('xx',)


def test_concrete_dims_given_dim_list_returns_dim_tuple() -> None:
    var = sc.empty(dims=('xx', 'yy'), shape=(2, 2))
    assert concepts.concrete_dims(var, ['yy', 'xx']) == ('yy', 'xx')


def test_concrete_dims_given_none_returns_obj_dims() -> None:
    var = sc.empty(dims=('xx', 'yy'), shape=(2, 2))
    assert concepts.concrete_dims(var, None) == ('xx', 'yy')


def test_irreducible_mask_returns_union_of_relevant_masks() -> None:
    da = sc.DataArray(sc.empty(dims=('xx', 'yy'), shape=(2, 3)))
    da.masks['x'] = sc.array(dims=['xx'], values=[False, True])
    da.masks['y'] = sc.array(dims=['yy'], values=[False, True, False])
    da.masks['xy'] = sc.array(
        dims=['xx', 'yy'], values=[[False, False, False], [False, False, True]]
    )
    assert sc.identical(
        concepts.irreducible_mask(da, 'xx'), da.masks['x'] | da.masks['xy']
    )
    assert sc.identical(
        concepts.irreducible_mask(da, 'yy'), da.masks['y'] | da.masks['xy']
    )
    assert sc.identical(
        concepts.irreducible_mask(da, ('xx', 'yy')),
        da.masks['x'] | da.masks['y'] | da.masks['xy'],
    )


def test_irreducible_mask_returns_None_if_all_masks_unrelated() -> None:
    da = sc.DataArray(sc.empty(dims=('xx', 'yy'), shape=(2, 3)))
    da.masks['x'] = sc.array(dims=['xx'], values=[False, True])
    assert concepts.irreducible_mask(da, 'yy') is None


def test_irreducible_mask_returns_copy_if_single_mask_matches() -> None:
    da = sc.DataArray(sc.empty(dims=('xx', 'yy'), shape=(2, 3)))
    da.masks['x'] = sc.array(dims=['xx'], values=[False, True])
    mask = concepts.irreducible_mask(da, 'xx')
    mask.values[0] = True
    assert not da.masks['x'].values[0]


def test_irreducible_mask_does_not_include_0d_mask() -> None:
    da = sc.DataArray(sc.empty(dims=('xx', 'yy'), shape=(2, 3)))
    da.masks['0d'] = sc.scalar(True)
    assert concepts.irreducible_mask(da, 'xx') is None
    assert concepts.irreducible_mask(da, 'yy') is None
    assert concepts.irreducible_mask(da, ('xx', 'yy')) is None
    assert concepts.irreducible_mask(da, None) is None


def test_irreducible_mask_returns_mask_with_dim_order_matching_data() -> None:
    da = sc.DataArray(sc.empty(dims=('xx', 'yy'), shape=(2, 3)))
    da.masks['x'] = sc.ones(dims=['yy', 'xx'], shape=(3, 2), dtype=bool)
    assert concepts.irreducible_mask(da, 'yy').dims == ('xx', 'yy')


def test_irreducible_mask_returns_multiple_masks_with_dim_order_matching_data() -> None:
    da = sc.DataArray(sc.empty(dims=('xx', 'yy'), shape=(2, 3)))
    da.masks['m1'] = sc.ones(dims=['yy', 'xx'], shape=(3, 2), dtype=bool)
    da.masks['m2'] = sc.ones(dims=['yy', 'xx'], shape=(3, 2), dtype=bool)
    assert concepts.irreducible_mask(da, 'yy').dims == ('xx', 'yy')


def test_reduced_masks_copies_preserved_masks() -> None:
    da = sc.DataArray(sc.empty(dims=('xx', 'yy'), shape=(2, 3)))
    da.masks['x'] = sc.array(dims=['xx'], values=[False, True])
    da.masks['y'] = sc.array(dims=['yy'], values=[False, True, False])
    masks = concepts.reduced_masks(da, 'xx')
    masks['y'].values[0] = True
    assert not da.masks['y'].values[0]
