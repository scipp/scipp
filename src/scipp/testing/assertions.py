# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen
"""Custom assertions for pytest-based tests.

To get the best error messages, tell pytest to rewrite assertions in this module.
Place the following code in your ``conftest.py``:

.. code-block:: python

    pytest.register_assert_rewrite('scipp.testing.assertions')

"""
from contextlib import contextmanager
from typing import Any, Iterator, Mapping, TypeVar

import numpy as np

from ..core import DataArray, DataGroup, Dataset, Variable
from ..core.comparison import identical

# Exception notes are formatted as 'PREPOSITION {loc}',
# where 'loc' is set by the concrete assertion functions to indicate coords, attrs, etc.
# 'PREPOSITION' is replaced at the top level to produce exception messages like:
#
# [...]
# in coord 'x'
# of data group item 'b'
# of data group item 'a'

T = TypeVar('T')


def assert_identical(a: T, b: T) -> None:
    """Raise an AssertionError if two objects are not identical.

    For Scipp objects, ``assert_identical(a, b)`` is equivalent to
    ``assert sc.identical(a, b, equal_nan=True)`` but produces a more precise
    error message in pytest.
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
    try:
        _assert_identical_impl(a, b)
    except AssertionError as exc:
        if hasattr(exc, '__notes__'):
            # See comment above.
            notes = []
            rest = -1
            for i, note in enumerate(exc.__notes__):
                if 'PREPOSITION' in note:
                    notes.append(note.replace('PREPOSITION', 'in'))
                    rest = i
                    break
            notes.extend(
                note.replace('PREPOSITION', 'of') for note in exc.__notes__[rest + 1 :]
            )
            exc.__notes__ = notes
        raise


def _assert_identical_impl(a: T, b: T) -> None:
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
    assert (a.bins is None) == (b.bins is None)
    if a.bins is None:
        _assert_identical_dense_variable_data(a, b)
    else:
        _assert_identical_binned_variable_data(a, b)


def _assert_identical_binned_variable_data(a: Variable, b: Variable) -> None:
    # Support for iterating over bin contents is limited in Python.
    # So, simply use `identical` even though it does not produce good error messages.
    assert a.bins.unit == b.bins.unit
    assert identical(a, b)


def _assert_identical_dense_variable_data(a: Variable, b: Variable) -> None:
    with _add_note('values'):
        np.testing.assert_array_equal(
            a.values, b.values, err_msg='when comparing values'
        )
    if a.variances is not None:
        assert b.variances is not None, 'a has variances but b does not'
        with _add_note('variances'):
            np.testing.assert_array_equal(
                a.variances, b.variances, err_msg='when comparing variances'
            )
    else:
        assert b.variances is None, 'a has no variances but b does'


def _assert_identical_data_array(a: DataArray, b: DataArray) -> None:
    _assert_identical_variable(a.data, b.data)
    _assert_mapping_eq(a.coords, b.coords, 'coord')
    _assert_mapping_eq(a.attrs, b.attrs, 'attr')
    _assert_mapping_eq(a.masks, b.masks, 'mask')


def _assert_identical_dataset(a: Dataset, b: Dataset) -> None:
    _assert_mapping_eq(a, b, 'dataset item')


def _assert_identical_datagroup(a: DataGroup, b: DataGroup) -> None:
    _assert_mapping_eq(a, b, 'data group item')


def _assert_mapping_eq(
    a: Mapping[str, Any], b: Mapping[str, Any], map_name: str
) -> None:
    with _add_note(map_name + 's'):
        assert a.keys() == b.keys()
    for name, var_a in a.items():
        with _add_note("{} '{}'", map_name, name):
            _assert_identical_impl(var_a, b[name])


@contextmanager
def _add_note(loc: str, *args: str) -> Iterator[None]:
    try:
        yield
    except AssertionError as exc:
        if hasattr(exc, 'add_note'):
            # Needs Python >= 3.11
            exc.add_note(f'PREPOSITION {loc.format(*args)}')
        raise
