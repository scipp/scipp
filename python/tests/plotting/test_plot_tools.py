# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Jan-Lukas Wynen

import numpy as np
import pytest
import scipp.plotting.tools as spt


def test_plot_linspace_float():
    def compare_sc_np_linspace(*args, **kwargs):
        np.testing.assert_array_equal(spt.linspace(*args, **kwargs),
                                      np.linspace(*args, **kwargs))

    compare_sc_np_linspace(0.0, 10.0, 1)
    compare_sc_np_linspace(1, 11, 1)
    compare_sc_np_linspace(0, 5.0, 3)
    compare_sc_np_linspace(-3.1, 4.0, 7)
    compare_sc_np_linspace(-4, 7, 12)
    compare_sc_np_linspace(6, -3.12, 9)
    compare_sc_np_linspace(2, -1, 4, endpoint=False)
    compare_sc_np_linspace(-6, 2, 4, endpoint=False)
    compare_sc_np_linspace(6, 2, 4, endpoint=False)


def test_plot_linspace_float_invalid():
    with pytest.raises(ValueError):
        spt.linspace(0, 1, -1)


def test_plot_linspace_datetime():
    def to_datetime(a, unit):
        return np.array([np.datetime64(x, unit) for x in a])

    np.testing.assert_array_equal(
        spt.linspace(np.datetime64(0, 's'), np.datetime64(5, 's'), 6),
        to_datetime([0, 1, 2, 3, 4, 5], 's'))
    np.testing.assert_array_equal(
        spt.linspace(np.datetime64(0, 'ms'), np.datetime64(3, 'ms'), 3),
        to_datetime([0, int(1.5), 3], 'ms'))
    np.testing.assert_array_equal(
        spt.linspace(np.datetime64(4, 'ns'), np.datetime64(1, 'ns'), 3),
        to_datetime([4, int(2.5), 1], 'ns'))


def test_plot_linspace_datetime_dtype_mismatch():
    with pytest.raises(TypeError):
        spt.linspace(np.datetime64(0, 's'), 13, 2)
    with pytest.raises(TypeError):
        spt.linspace(2, np.datetime64(5, 's'), 2)
    with pytest.raises(TypeError):
        spt.linspace(np.datetime64(2, 's'), np.datetime64(5, 'ms'), 3)
