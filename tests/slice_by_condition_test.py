# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import pytest

import scipp as sc


def make_var(xx: int = 4) -> sc.Variable:
    return sc.arange('dummy', 12, dtype='int64').fold(
        dim='dummy', sizes={'xx': xx, 'yy': 3}
    )


def make_array() -> sc.DataArray:
    da = sc.DataArray(make_var())
    da.coords['xx'] = sc.arange('xx', 4, dtype='int64')
    da.coords['yy'] = sc.arange('yy', 3, dtype='int64')
    return da


def make_binned_array() -> sc.DataArray:
    da = sc.data.table_xyz(100).bin(x=4, y=3).rename(x='xx', y='yy')
    da.coords['xx'] = da.coords['xx'][:-1]
    da.coords['yy'] = da.coords['yy'][:-1]
    return da


def make_binned_array_variable_buffer() -> sc.DataArray:
    da = make_binned_array()
    da.data = da.bins.data  # type: ignore[union-attr]
    return da


def make_dataset() -> sc.Dataset:
    return sc.Dataset({'xy': make_array()})


def make_binned_dataset() -> sc.Dataset:
    return sc.Dataset(
        {  # type: ignore[arg-type]
            'xy': make_array().data,
            'binned': make_binned_array(),
            'binned-variable': make_binned_array_variable_buffer(),
        }
    )


@pytest.fixture(
    params=[
        make_var(),
        make_array(),
        make_dataset(),
        make_binned_array(),
        make_binned_array_variable_buffer(),
        make_binned_dataset(),
    ],
    ids=[
        'Variable',
        'DataArray',
        'Dataset',
        'binned-DataArray',
        'binned-DataArray-buffer-Variable',
        'binned-dataset',
    ],
)
def sliceable(
    request: pytest.FixtureRequest,
) -> sc.Variable | sc.DataArray | sc.Dataset:
    return request.param  # type: ignore[no-any-return]


def test_all_false_gives_empty_slice(
    sliceable: sc.Variable | sc.DataArray | sc.Dataset,
) -> None:
    condition = sc.array(dims=['xx'], values=[False, False, False, False])
    assert sc.identical(sliceable[condition], sliceable['xx', 0:0])


def test_all_true_gives_copy(
    sliceable: sc.Variable | sc.DataArray | sc.Dataset,
) -> None:
    condition = sc.array(dims=['xx'], values=[True, True, True, True])
    original = sliceable.copy()
    sliced = sliceable[condition]
    assert sc.identical(sliced, sliceable)
    sliced *= 2
    assert sc.identical(sliceable, original)


def test_true_and_false_concats_slices(
    sliceable: sc.Variable | sc.DataArray | sc.Dataset,
) -> None:
    condition = sc.array(dims=['xx'], values=[True, False, True, True])
    assert sc.identical(
        sliceable[condition],
        sc.concat([sliceable['xx', 0], sliceable['xx', 2:]], 'xx'),  # type: ignore[type-var]
    )


def test_non_dimension_coords_are_preserved() -> None:
    da = make_array()
    da.coords['xx2'] = da.coords['xx']
    condition = sc.array(dims=['xx'], values=[True, False, True, True])
    assert sc.identical(
        da[condition].coords['xx2'],
        sc.array(dims=['xx'], dtype='int64', values=[0, 2, 3]),
    )


def test_bin_edges_are_dropped() -> None:
    da = make_array()
    base = da.copy()
    da.coords['edges'] = sc.concat([da.coords['xx'], da.coords['xx'][-1] + 1], 'xx')
    condition = sc.array(dims=['xx'], values=[True, False, True, True])
    assert sc.identical(da[condition], sc.concat([base['xx', 0], base['xx', 2:]], 'xx'))


def test_non_boolean_condition_raises_DTypeError() -> None:
    var = make_var()
    condition = (var < 3).to(dtype='int32')
    with pytest.raises(sc.DTypeError):
        var[condition]


def test_all_false_condition_with_wrong_dims_raises_DimensionError() -> None:
    var = make_var()
    condition = sc.array(dims=['not-in-var'], values=[False, False, False])
    with pytest.raises(sc.DimensionError):
        var[condition]


def test_all_true_condition_with_wrong_dims_raises_DimensionError() -> None:
    var = make_var()
    condition = sc.array(dims=['not-in-var'], values=[True, True, True])
    with pytest.raises(sc.DimensionError):
        var[condition]


def test_condition_with_wrong_dims_raises_DimensionError() -> None:
    var = make_var()
    condition = sc.array(dims=['not-in-var'], values=[True, False, True])
    with pytest.raises(sc.DimensionError):
        var[condition]


def test_all_false_condition_with_wrong_shape_raises_DimensionError() -> None:
    var = make_var(xx=4)
    condition = sc.array(dims=['xx'], values=[False, False, False])
    with pytest.raises(sc.DimensionError):
        var[condition]


def test_all_true_condition_with_wrong_shape_raises_DimensionError() -> None:
    var = make_var(xx=4)
    condition = sc.array(dims=['xx'], values=[True, True, True])
    with pytest.raises(sc.DimensionError):
        var[condition]


def test_condition_with_wrong_shape_raises_DimensionError() -> None:
    var = make_var(xx=4)
    condition = sc.array(dims=['xx'], values=[False, True, False])
    with pytest.raises(sc.DimensionError):
        var[condition]


def test_all_false_2d_condition_raises_DimensionError() -> None:
    var = make_var()
    condition = var != var
    with pytest.raises(sc.DimensionError):
        var[condition]


def test_all_true_2d_condition_raises_DimensionError() -> None:
    # Strictly speaking this could be supported, but having this odd special case
    # work would likely add more confusion and bugs surfacing at surprising times.
    var = make_var()
    condition = var == var
    with pytest.raises(sc.DimensionError):
        var[condition]


def test_2d_condition_raises_DimensionError() -> None:
    var = make_var()
    condition = var < 3
    with pytest.raises(sc.DimensionError):
        var[condition]


def test_0d_false_condition_raises_DimensionError() -> None:
    var = make_var()
    condition = sc.scalar(False)
    with pytest.raises(sc.DimensionError):
        var[condition]


def test_0d_true_condition_raises_DimensionError() -> None:
    # Strictly speaking this could be supported, but having this odd special case
    # work would likely add more confusion and bugs surfacing at surprising times.
    var = make_var()
    condition = sc.scalar(True)
    with pytest.raises(sc.DimensionError):
        var[condition]
