# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Matthew Andrew
import scipp as sc
import numpy as np
import pytest


class TestSliceByValue:
    def setup_method(self):
        var = sc.Variable(['x'], values=np.arange(5, dtype=np.float) + 0.5)
        values_a = sc.Variable(dims=['x'],
                               values=[1.0, 1.1, 1.2, 1.3, 1.4],
                               unit=sc.units.m)
        values_b = sc.Variable(dims=['x'],
                               values=[1.0, 2.0, 3.0, 4.0, 5.0],
                               unit=sc.units.m)
        self._d = sc.Dataset(data={
            'a': values_a,
            'b': values_b
        },
                             coords={'x': var})

    def test_slice_by_single_value(self):
        def test(sliceable):
            by_value = sliceable['x', 1.5 * sc.units.dimensionless]
            by_index = sliceable['x', 1]
            assert sc.is_equal(by_value, by_index)

        test(self._d['a'])
        test(self._d)

    def test_assigning_to_slice_by_value(self):
        self._d['a']['x', 1.5 * sc.units.dimensionless] = 5.7 * sc.units.m
        slice = self._d['a']['x', 1.5 * sc.units.dimensionless].values
        assert slice == np.array(5.7)

    def test_modifying_slice_in_place(self):
        self._d['a']['x', 1.5 * sc.units.dimensionless] *= 2.5
        slice = self._d['a']['x', 1.5 * sc.units.dimensionless].values
        assert slice == np.array(2.75)

        self._d['a']['x', 1.5 * sc.units.dimensionless] += 2.25 * sc.units.m
        slice = self._d['a']['x', 1.5 * sc.units.dimensionless].values
        assert slice == np.array(5.0)

        self._d['a']['x', 1.5 * sc.units.dimensionless] -= 3.0 * sc.units.m
        slice = self._d['a']['x', 1.5 * sc.units.dimensionless].values
        assert slice == np.array(2.0)

        self._d['a']['x', 1.5 * sc.units.dimensionless] /= 2.0
        slice = self._d['a']['x', 1.5 * sc.units.dimensionless].values
        assert slice == np.array(1.0)

    def test_slice_with_range(self):
        def test(sliceable):

            by_value = sliceable['x', 1.5 * sc.units.dimensionless:4.5 *
                                 sc.units.dimensionless]
            by_index = sliceable['x', 1:-1]
            assert sc.is_equal(by_value, by_index)

        test(self._d['a'])
        test(self._d)

    def test_assign_variable_to_range(self):
        self._d['a']['x', 1.5 * sc.units.dimensionless:4.5 *
                     sc.units.dimensionless] = sc.Variable(
                         dims=['x'], values=[6.0, 6.0, 6.0], unit=sc.units.m)

        assert self._d['a'].data.values.tolist() == [1.0, 6.0, 6.0, 6.0, 1.4]

    def test_modify_range_in_place_from_variable(self):
        self._d['a']['x', 1.5 * sc.units.dimensionless:4.5 *
                     sc.units.dimensionless] += sc.Variable(
                         dims=['x'], values=[2.0, 2.0, 2.0], unit=sc.units.m)

        assert self._d['a'].data.values.tolist() == [1.0, 3.1, 3.2, 3.3, 1.4]

    def test_assign_dataarray_to_range(self):
        self._d['a']['x', 1.5 * sc.units.dimensionless:4.5 *
                     sc.units.dimensionless] = self._d['b']['x', 1:-1]

        assert self._d['a'].data.values.tolist() == [1.0, 2.0, 3.0, 4.0, 1.4]

    def test_modify_range_in_place_from_dataarray(self):
        self._d['a']['x', 1.5 * sc.units.dimensionless:4.5 *
                     sc.units.dimensionless] += self._d['b']['x', 1:-1]

        assert self._d['a'].data.values.tolist() == [1.0, 3.1, 4.2, 5.3, 1.4]

    def test_slice_by_incorrect_unit_throws(self):
        with pytest.raises(RuntimeError) as e_info:
            self._d['a']['x', 1.5 * sc.units.m]
        assert str(e_info.value) == 'dimensionless expected to be equal to m'

    def test_out_of_range_throws(self):
        with pytest.raises(IndexError):
            self._d['a']['x', 5.0 * sc.units.dimensionless]

    def test_assign_incompatable_variable_throws(self):
        with pytest.raises(RuntimeError) as e_info:
            self._d['a']['x', 1.5 * sc.units.dimensionless:4.5 *
                         sc.units.dimensionless] = sc.Variable(
                             dims=['x'], values=[6.0, 6.0], unit=sc.units.m)
        assert str(e_info.value) == '{{x, 3}} expected to be equal to {{x, 2}}'

    def test_assign_incompatible_dataarray(self):
        with pytest.raises(RuntimeError):
            self._d['a']['x', 1.5 * sc.units.dimensionless:4.5 *
                         sc.units.dimensionless] = self._d['b']['x', 0:-2]

    def test_range_slice_with_step_throws(self):
        with pytest.raises(RuntimeError) as e_info:
            self._d['a']['x', 1.5 * sc.units.m:4.5 * sc.units.m:4]
        assert str(e_info.value
                   ) == "Step cannot be specified for value based slicing."

    def test_range_start_only(self):
        by_value = self._d['a']['x', 1.5 * sc.units.dimensionless:]
        by_index = self._d['a']['x', 1:]
        assert sc.is_equal(by_value, by_index)

    def test_range_end_only(self):
        by_value = self._d['a']['x', :2.5 * sc.units.dimensionless]
        by_index = self._d['a']['x', :2]
        assert sc.is_equal(by_value, by_index)

    def test_range_all(self):
        by_value = self._d['a']['x', :]
        assert sc.is_equal(by_value, self._d['a'])
