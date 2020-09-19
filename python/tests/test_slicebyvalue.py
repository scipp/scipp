# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Matthew Andrew
import scipp as sc
import numpy as np
from .common import assert_export

class TestSliceByValue:
    def setup_method(self):
        var = sc.Variable(['x'], values=np.arange(5, dtype=np.float) + 0.5)
        values_a = sc.Variable(dims=['x'], values=[1.0,1.1,1.2,1.3,1.4], unit=sc.units.m)
        self._d = sc.Dataset(data={'a': values_a, 'b': var}, coords={'x': var})

    def test_slice_by_single_value(self):
        by_value = self._d['a']['x', 1.5*sc.units.dimensionless]
        by_index = self._d['a']['x', 1]
        assert sc.is_equal(by_value, by_index)

    def test_assigning_to_slice_by_value(self):
        self._d['a']['x', 1.5*sc.units.dimensionless] = 5.7 * sc.units.m
        slice = self._d['a']['x', 1.5*sc.units.dimensionless].values
        assert slice == np.array(5.7)

    def test_modifying_slice_in_place(self):
        self._d['a']['x', 1.5*sc.units.dimensionless] *= 2.5
        slice = self._d['a']['x', 1.5*sc.units.dimensionless].values
        assert slice == np.array(2.75)

        self._d['a']['x', 1.5*sc.units.dimensionless] += 2.25 * sc.units.m
        slice = self._d['a']['x', 1.5*sc.units.dimensionless].values
        assert slice == np.array(5.0)

        self._d['a']['x', 1.5*sc.units.dimensionless] -= 3.0 * sc.units.m
        slice = self._d['a']['x', 1.5*sc.units.dimensionless].values
        assert slice == np.array(2.0)

        self._d['a']['x', 1.5*sc.units.dimensionless] /= 2.0
        slice = self._d['a']['x', 1.5*sc.units.dimensionless].values
        assert slice == np.array(1.0)

