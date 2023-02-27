# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen
"""Custom assertions for pytest-based tests.

To get the best error messages, tell pytest to rewrite assertions in this module.
Place the following code in your ``conftest.py``:

.. code-block:: python

    pytest.register_assert_rewrite('scipp.testing.assertions')

"""

from typing import Any, Mapping, TypeVar

import numpy as np

from ..core import DataArray, DataGroup, Dataset, Variable

T = TypeVar('T')


def assert_identical(a: T, b: T) -> None:
    """Raise an AssertionError if two objects are not identical.

    For Scipp objects, ``assert_identical(a, b)`` is equivalent to
    ``assert sc.identical(a, b)`` but produces a more precise error message in pytest.
    If this function is called with arguments that are not supported by
    :func:`scipp.identical`, it calls ``assert a == b``.

    This function requires exact equality including equal types.
    For example, ``assert_identical(1, 1.0)`` will raise.

    NaN elements of Scipp variables are treated as equal.

    Parameters
    ----------
    a:
        The actual object to check.
    b:
        The desired, expected object.

    Raises
    ------
    AssertionError
        If the objects are not identical.
    """
    assert type(a) == type(b)
    if isinstance(a, Variable):
        _assert_identical_variable(a, b)
    elif isinstance(a, DataArray):
        _assert_identical_data_array(a, b)
    elif isinstance(a, Dataset):
        _assert_identical_dataset(a, b)
    elif isinstance(a, DataGroup):
        _assert_identical_datagroup(a, b)
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
    _assert_mapping_eq(a.coords, b.coords)
    _assert_mapping_eq(a.attrs, b.attrs)
    _assert_mapping_eq(a.masks, b.masks)


def _assert_identical_dataset(a: Dataset, b: Dataset) -> None:
    _assert_mapping_eq(a, b)


def _assert_identical_datagroup(a: DataGroup, b: DataGroup) -> None:
    _assert_mapping_eq(a, b)


def _assert_mapping_eq(a: Mapping[str, Any], b: Mapping[str, Any]) -> None:
    assert a.keys() == b.keys()
    for name, var_a in a.items():
        assert_identical(var_a, b[name])
