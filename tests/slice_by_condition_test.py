# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import pytest
import scipp as sc


def test_all_false_gives_empty_slice(sliceable):
    condition = sc.array(dims=['xx'], values=[False, False, False, False])
    assert sc.identical(sliceable[condition], sliceable['xx', 0:0])


def test_all_true_gives_copy(sliceable):
    condition = sc.array(dims=['xx'], values=[True, True, True, True])
    assert sc.identical(sliceable[condition], sliceable)


def test_true_and_false_concats_slices(sliceable):
    condition = sc.array(dims=['xx'], values=[True, False, True, True])
    assert sc.identical(sliceable[condition],
                        sc.concat([sliceable['xx', 0], sliceable['xx', 2:]], 'xx'))


def test_non_dimension_coords_are_preserved(data_array_xx4_yy3):
    da = data_array_xx4_yy3.copy()
    da.coords['xx2'] = da.coords['xx']
    condition = sc.array(dims=['xx'], values=[True, False, True, True])
    assert sc.identical(da[condition].coords['xx2'],
                        sc.array(dims=['xx'], dtype='int64', values=[0, 2, 3]))


def test_bin_edges_are_dropped(data_array_xx4_yy3):
    da = data_array_xx4_yy3.copy()
    base = da.copy()
    da.coords['edges'] = sc.concat([da.coords['xx'], da.coords['xx'][-1] + 1], 'xx')
    condition = sc.array(dims=['xx'], values=[True, False, True, True])
    assert sc.identical(da[condition], sc.concat([base['xx', 0], base['xx', 2:]], 'xx'))


def test_dataset_item_independent_of_condition_dim_preserved_unchanged(dataset_xx4_yy3):
    condition = sc.array(dims=['yy'], values=[True, False, True])
    assert sc.identical(dataset_xx4_yy3[condition]['x'], dataset_xx4_yy3['x'])


def test_non_boolean_condition_raises_DTypeError(variable_xx4_yy3):
    with pytest.raises(sc.DTypeError):
        condition = (variable_xx4_yy3 < 3).to(dtype='int32')
        variable_xx4_yy3[condition]


def test_2d_condition_raises_DimensionError(variable_xx4_yy3):
    with pytest.raises(sc.DimensionError):
        condition = variable_xx4_yy3 < 3
        variable_xx4_yy3[condition]


def test_0d_condition_raises_DimensionError(variable_xx4_yy3):
    with pytest.raises(sc.DimensionError):
        condition = sc.scalar(False)
        variable_xx4_yy3[condition]
