# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen
"""Custom assertions for pytest-based tests.

To get the best error messages, tell pytest to rewrite assertions in this module.
Place the following code in your ``conftest.py``:

.. code-block:: python

    pytest.register_assert_rewrite('scipp.testing.assertions')

"""

from typing import TypeVar

import numpy as np

from ..core import DataArray, Variable

T = TypeVar('T')


def assert_identical(a: T, b: T) -> None:
    """TODO"""
    assert type(a) == type(b)
    if isinstance(a, Variable):
        _assert_identical_variable(a, b)
    elif isinstance(a, DataArray):
        _assert_identical_data_array(a, b)
    else:
        assert a == b


def _assert_identical_variable(a: Variable, b: Variable) -> None:
    assert a.sizes == b.sizes
    assert a.unit == b.unit
    assert a.dtype == b.dtype
    np.testing.assert_array_equal(a.values, b.values, err_msg='when comparing values')
    if a.variances is not None:
        assert b.variances is not None, 'a has variances but b does not'
        np.testing.assert_array_equal(a.variances,
                                      b.variances,
                                      err_msg='when comparing variances')
    else:
        assert b.variances is None, 'a has no variances but b does'


def _assert_identical_data_array(a: DataArray, b: DataArray) -> None:
    _assert_identical_variable(a.data, b.data)

    assert a.coords.keys() == b.coords.keys()
    for name, coord_a in a.coords.items():
        _assert_identical_variable(coord_a, b.coords[name])

    assert a.attrs.keys() == b.attrs.keys()
    for name, attr_a in a.attrs.items():
        _assert_identical_variable(attr_a, b.attrs[name])

    assert a.masks.keys() == b.masks.keys()
    for name, mask_a in a.masks.items():
        _assert_identical_variable(mask_a, b.masks[name])
