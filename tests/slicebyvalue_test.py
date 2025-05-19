# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Matthew Andrew
from itertools import product

import numpy as np
import pytest

import scipp as sc
from scipp.core import label_based_index_to_positional_index
from scipp.testing import assert_identical


class TestSliceByValue:
    def setup_method(self):
        var = sc.Variable(dims=['x'], values=np.arange(5, dtype=float) + 0.5)
        values_a = sc.Variable(
            dims=['x'], values=[1.0, 1.1, 1.2, 1.3, 1.4], unit=sc.units.m
        )
        values_b = sc.Variable(
            dims=['x'], values=[1.0, 2.0, 3.0, 4.0, 5.0], unit=sc.units.m
        )
        self._d = sc.Dataset(data={'a': values_a, 'b': values_b}, coords={'x': var})

    def test_slice_by_single_value(self) -> None:
        def test(sliceable):
            by_value = sliceable['x', 1.5 * sc.units.dimensionless]
            by_index = sliceable['x', 1]
            assert sc.identical(by_value, by_index)

        test(self._d['a'])
        test(self._d)

    def test_assigning_to_slice_by_value_dataarray(self) -> None:
        self._d['a']['x', 1.5 * sc.units.dimensionless] = 5.7 * sc.units.m
        slice = self._d['a']['x', 1.5 * sc.units.dimensionless].values
        assert slice == np.array(5.7)

    def test_assigning_to_slice_by_value_dataset(self) -> None:
        self._d['x', 0.5 * sc.units.dimensionless] = self._d[
            'x', 1.5 * sc.units.dimensionless
        ]
        assert self._d['x', 0]['a'].value == self._d['x', 1]['a'].value

    def test_modifying_slice_in_place(self) -> None:
        self._d['a']['x', 1.5 * sc.units.dimensionless] *= 2.5
        slice = self._d['a']['x', 1.5 * sc.units.dimensionless].values
        assert slice == np.array(2.75)
        slice = self._d['x', 1.5 * sc.units.dimensionless]['a'].values
        assert slice == np.array(2.75)

        self._d['a']['x', 1.5 * sc.units.dimensionless] += 2.25 * sc.units.m
        slice = self._d['a']['x', 1.5 * sc.units.dimensionless].values
        assert slice == np.array(5.0)
        slice = self._d['x', 1.5 * sc.units.dimensionless]['a'].values
        assert slice == np.array(5.0)

        self._d['a']['x', 1.5 * sc.units.dimensionless] -= 3.0 * sc.units.m
        slice = self._d['a']['x', 1.5 * sc.units.dimensionless].values
        assert slice == np.array(2.0)
        slice = self._d['x', 1.5 * sc.units.dimensionless]['a'].values
        assert slice == np.array(2.0)

        self._d['a']['x', 1.5 * sc.units.dimensionless] /= 2.0
        slice = self._d['a']['x', 1.5 * sc.units.dimensionless].values
        assert slice == np.array(1.0)
        slice = self._d['x', 1.5 * sc.units.dimensionless]['a'].values
        assert slice == np.array(1.0)

    def test_slice_with_range(self) -> None:
        def test(sliceable):
            by_value = sliceable[
                'x', 1.5 * sc.units.dimensionless : 4.5 * sc.units.dimensionless
            ]
            by_index = sliceable['x', 1:-1]
            assert sc.identical(by_value, by_index)

        test(self._d['a'])
        test(self._d)

    def test_assign_variable_to_range_dataarray_fails(self) -> None:
        with pytest.raises(sc.DataArrayError):  # readonly
            self._d['a'][
                'x', 1.5 * sc.units.dimensionless : 4.5 * sc.units.dimensionless
            ].data = sc.Variable(dims=['x'], values=[6.0, 6.0, 6.0], unit=sc.units.m)

    def test_assign_variable_to_range_dataset_fails(self) -> None:
        with pytest.raises(sc.DataArrayError):  # readonly
            self._d['x', 1.5 * sc.units.dimensionless : 4.5 * sc.units.dimensionless][
                'a'
            ].data = sc.Variable(dims=['x'], values=[6.0, 6.0, 6.0], unit=sc.units.m)

    def test_on_dataarray_modify_range_in_place_from_variable(self) -> None:
        self._d['a'][
            'x', 1.5 * sc.units.dimensionless : 4.5 * sc.units.dimensionless
        ].data += sc.Variable(dims=['x'], values=[2.0, 2.0, 2.0], unit=sc.units.m)

        assert self._d['a'].data.values.tolist() == [1.0, 3.1, 3.2, 3.3, 1.4]

    def test_on_dataset_modify_range_in_place_from_variable(self) -> None:
        self._d['x', 1.5 * sc.units.dimensionless : 4.5 * sc.units.dimensionless][
            'a'
        ].data += sc.Variable(dims=['x'], values=[2.0, 2.0, 2.0], unit=sc.units.m)

        assert self._d['a'].data.values.tolist() == [1.0, 3.1, 3.2, 3.3, 1.4]

    def test_on_dataarray_assign_dataarray_to_range(self) -> None:
        self._d['a'][
            'x', 1.5 * sc.units.dimensionless : 4.5 * sc.units.dimensionless
        ] = self._d['b']['x', 1:-1]

        assert self._d['a'].data.values.tolist() == [1.0, 2.0, 3.0, 4.0, 1.4]

    def test_on_dataset_assign_dataarray_to_range_fails(self) -> None:
        with pytest.raises(sc.DatasetError):  # readonly
            self._d['x', 1.5 * sc.units.dimensionless : 4.5 * sc.units.dimensionless][
                'a'
            ] = self._d['b']['x', 1:-1]

    def test_on_dataarray_modify_range_in_place_from_dataarray(self) -> None:
        self._d['a'][
            'x', 1.5 * sc.units.dimensionless : 4.5 * sc.units.dimensionless
        ] += self._d['b']['x', 1:-1]

        assert self._d['a'].data.values.tolist() == [1.0, 3.1, 4.2, 5.3, 1.4]

    def test_on_dataset_modify_range_in_place_from_dataarray(self) -> None:
        self._d['x', 1.5 * sc.units.dimensionless : 4.5 * sc.units.dimensionless][
            'a'
        ] += self._d['b']['x', 1:-1]
        assert self._d['a'].data.values.tolist() == [1.0, 3.1, 4.2, 5.3, 1.4]

    def test_slice_by_incorrect_unit_throws(self) -> None:
        with pytest.raises(sc.UnitError) as e_info:
            _ = self._d['a']['x', 1.5 * sc.units.m]
        assert '(m)' in str(e_info.value)
        assert '(dimensionless)' in str(e_info.value)

    def test_out_of_range_throws(self) -> None:
        with pytest.raises(IndexError):
            _ = self._d['a']['x', 5.0 * sc.units.dimensionless]

    def test_assign_incompatible_variable_throws(self) -> None:
        with pytest.raises(sc.DimensionError) as e_info:
            self._d['a'][
                'x', 1.5 * sc.units.dimensionless : 4.5 * sc.units.dimensionless
            ] = sc.Variable(dims=['x'], values=[6.0, 6.0], unit=sc.units.m)
        assert str(e_info.value) == 'Expected (x: 3) to include (x: 2).'

    def test_assign_incompatible_dataarray(self) -> None:
        with pytest.raises(RuntimeError):
            self._d['a'][
                'x', 1.5 * sc.units.dimensionless : 4.5 * sc.units.dimensionless
            ] = self._d['b']['x', 0:-2]

    def test_range_slice_with_step_throws(self) -> None:
        with pytest.raises(RuntimeError) as e_info:
            self._d['a']['x', 1.5 * sc.units.m : 4.5 * sc.units.m : 4]
        assert str(e_info.value) == "Step cannot be specified for value based slicing."

    def test_range_start_only(self) -> None:
        by_value = self._d['a']['x', 1.5 * sc.units.dimensionless :]
        by_index = self._d['a']['x', 1:]
        assert sc.identical(by_value, by_index)

        by_value = self._d['x', 1.5 * sc.units.dimensionless :]
        by_index = self._d['x', 1:]
        assert sc.identical(by_value, by_index)

    def test_range_end_only(self) -> None:
        by_value = self._d['a']['x', : 2.5 * sc.units.dimensionless]
        by_index = self._d['a']['x', :2]
        assert sc.identical(by_value, by_index)

        by_value = self._d['x', : 2.5 * sc.units.dimensionless]
        by_index = self._d['x', :2]
        assert sc.identical(by_value, by_index)


def test_raises_DimensionError_if_dim_not_given() -> None:
    var = sc.arange('x', 4)
    da = sc.DataArray(var, coords={'x': var})
    with pytest.raises(sc.DimensionError):
        da[sc.scalar(1) : sc.scalar(3)]


@pytest.mark.parametrize(
    ('da', 'coord'),
    [
        (sc.data.binned_x(8, 15), 'x'),
        *product(
            (
                sc.DataArray(
                    sc.array(dims=('x', 'y'), values=np.random.randn(5, 10)),
                    coords={
                        'x': sc.arange('x', 5),
                        'y': sc.linspace('y', -1, 1, 10),
                    },
                ),
            ),
            ('x', 'y'),
        ),
    ],
)
@pytest.mark.parametrize(
    's',
    [
        (-2, 0),
        (-2, 0.1),
        (-2, 3.5),
        (-2, 10),
        (0, 0.1),
        (0, 3.5),
        (0, 10),
        (0.1, 3.5),
        (0.1, 10),
        (3.5, 10),
        (1, None),
        (None, 0.6),
        (None, None),
    ],
)
def test_label_based_index_to_positional_index(da, coord, s) -> None:
    s = slice(
        *(
            sc.scalar(v, unit=da.coords[coord].unit) if v is not None else None
            for v in s
        )
    )
    ind = label_based_index_to_positional_index(da.sizes, da.coords[coord], s)
    assert_identical(da[coord, s], da[ind])


@pytest.mark.parametrize(
    'a',
    [-2, 0, 0.1, 3.5, 10, 0.6],
)
def test_label_based_index_to_positional_index_scalar(a) -> None:
    da = sc.DataArray(
        sc.array(dims=('x', 'y'), values=np.random.randn(6, 10)),
        coords={
            'x': sc.array(dims=['x'], values=[-2, 0, 0.1, 0.6, 3.5, 10]),
            'y': sc.linspace('y', -1, 1, 10),
        },
    )
    a = sc.scalar(a, unit=da.coords['x'].unit)
    ind = label_based_index_to_positional_index(da.sizes, da.coords['x'], a)
    assert_identical(da['x', a], da[ind])


def test_label_based_index_to_positional_index_bin_edge_coord_with_scalar() -> None:
    da = sc.data.binned_x(8, 15)
    assert ('x', 6) == label_based_index_to_positional_index(
        da.sizes, da.coords['x'], sc.scalar(0.5, unit='m')
    )
    with pytest.raises(IndexError):
        label_based_index_to_positional_index(
            da.sizes, da.coords['x'], da.coords['x'].min() - sc.scalar(1, unit='m')
        )
    with pytest.raises(IndexError):
        label_based_index_to_positional_index(
            da.sizes, da.coords['x'], da.coords['x'].max() + sc.scalar(1, unit='m')
        )
