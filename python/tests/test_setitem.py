# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import scipp as sc


def test_setitem_required_for_inplace_ops():
    # Test that all required __setitem__ overloads for in-place operations
    # are available.

    var = sc.Variable(dims=['x', 'y'], shape=[2, 3])
    var *= 1.5  # not setitem, just assigns python variable
    var['x', 1:] *= 1.5  # Variable.__setitem__
    var['x', 1:]['y', 1:] *= 1.5  # VariableView.__setitem__

    a = sc.DataArray(data=var)
    a *= 1.5  # not setitem, just assigns python variable
    a['x', 1:] *= 1.5  # DataArray.__setitem__
    a['x', 1:]['y', 1:] *= 1.5  # DataArrayView.__setitem__

    d = sc.Dataset(data={'a': var})
    d *= 1.5  # not setitem, just assigns python variable
    d['a'] *= 1.5  # Dataset.__setitem__(string)
    d['x', 1:] *= 1.5  # Dataset.__setitem__(slice)
    d['x', 1:]['a'] *= 1.5  # DatasetView.__setitem__(string)
    d['a']['x', 1:] *= 1.5  # DatasetView.__setitem__(slice)
    d['x', 1:]['y', 1:] *= 1.5  # DatasetView.__setitem__(slice)


def test_setitem_coords_required_for_inplace_ops():
    import numpy as np
    var = sc.Variable(['x'], shape=(4, ))
    d = sc.Dataset()
    d.coords['x'] = var
    d['x', 2:].coords['x'] += 1.0
    # assignment
    expected_values = np.array([0, 0, 1, 1])
    assert np.allclose(d['x', :].coords['x'].values, expected_values)
