# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen
"""Custom assertions for pytest-based tests.

To get the best error messages, tell pytest to rewrite assertions in this module.
Place the following code in your ``conftest.py``:

.. code-block:: python

    pytest.register_assert_rewrite('scipp.testing.assertions')

"""

from collections.abc import Callable, Iterator, Mapping
from contextlib import contextmanager
from typing import Any, TypeVar

import numpy as np

from ..core import DataArray, DataGroup, Dataset, Variable

# Exception notes are formatted as 'PREPOSITION {loc}',
# where 'loc' is set by the concrete assertion functions to indicate coords and masks.
# 'PREPOSITION' is replaced at the top level to produce exception messages like:
#
# [...]
# in coord 'x'
# of data group item 'b'
# of data group item 'a'

_T = TypeVar('_T', Variable, DataArray, Dataset)


def assert_allclose(
    a: _T,
    b: _T,
    rtol: Variable | None = None,
    atol: Variable | None = None,
    **kwargs: Any,
) -> None:
    """Raise an AssertionError if two objects don't have similar values
    or if their other properties are not identical.

    Parameters
    ----------
    a:
        The actual object to check.
    b:
        The desired, expected object.
    rtol:
        Tolerance value relative (to b).
        Can be a scalar or non-scalar.
        Cannot have variances.
        Defaults to scalar 1e-7 if unset.
    atol:
        Tolerance value absolute.
        Can be a scalar or non-scalar.
        Cannot have variances.
        Defaults to scalar 0 if unset and takes units from y arg.
    kwargs:
        Additional arguments to pass to :func:`numpy.testing.assert_allclose`
        which is used for comparing data.

    Raises
    ------
    AssertionError
        If the objects are not identical.
    """
    return _assert_similar(_assert_allclose_impl, a, b, rtol=rtol, atol=atol, **kwargs)


def assert_identical(a: _T, b: _T) -> None:
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
    return _assert_similar(_assert_identical_impl, a, b)


def _assert_similar(impl: Callable[..., None], *args: Any, **kwargs: Any) -> None:
    try:
        impl(*args, **kwargs)
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


def _assert_identical_impl(
    a: _T,
    b: _T,
) -> None:
    _assert_identical_structure(a, b)
    _assert_identical_data(a, b)


def _assert_allclose_impl(
    a: _T,
    b: _T,
    **kwargs: Any,
) -> None:
    _assert_identical_structure(a, b)
    _assert_allclose_data(a, b, **kwargs)


def _assert_identical_structure(
    a: _T,
    b: _T,
) -> None:
    assert type(a) is type(b)
    if isinstance(a, Variable):
        _assert_identical_variable_structure(a, b)
    elif isinstance(a, DataArray):
        _assert_identical_data_array_structure(a, b)


def _assert_identical_variable_structure(a: Variable, b: Variable) -> None:
    assert a.sizes == b.sizes
    assert a.unit == b.unit
    assert a.dtype == b.dtype
    assert a.is_binned == b.is_binned
    if a.is_binned:
        assert b.is_binned
        assert a.bins.unit == b.bins.unit
    else:
        if a.variances is not None:
            assert b.variances is not None, 'a has variances but b does not'
        else:
            assert b.variances is None, 'a has no variances but b does'


def _assert_identical_data_array_structure(a: DataArray, b: DataArray) -> None:
    _assert_mapping_eq(a.coords, b.coords, 'coord')
    _assert_mapping_eq(a.masks, b.masks, 'mask')


def _assert_identical_dataset(a: Dataset, b: Dataset) -> None:
    _assert_mapping_eq(a, b, 'dataset item')


def _assert_identical_datagroup(a: DataGroup[Any], b: DataGroup[Any]) -> None:
    _assert_mapping_eq(a, b, 'data group item')


def _assert_identical_alignment(a: Any, b: Any) -> None:
    if isinstance(a, Variable) and isinstance(b, Variable):
        assert a.aligned == b.aligned


def _assert_mapping_eq(
    a: Mapping[str, Any],
    b: Mapping[str, Any],
    map_name: str,
) -> None:
    with _add_note(map_name + 's'):
        assert a.keys() == b.keys()
    for name, val_a in a.items():
        with _add_note("{} '{}'", map_name, name):
            val_b = b[name]
            _assert_identical_impl(val_a, val_b)
            _assert_identical_alignment(val_a, val_b)


def _assert_identical_data(
    a: _T,
    b: _T,
) -> None:
    if isinstance(a, Variable):
        _assert_identical_variable_data(a, b)
    elif isinstance(a, DataArray):
        _assert_identical_variable_data(a.data, b.data)
    elif isinstance(a, Dataset):
        _assert_identical_dataset(a, b)
    elif isinstance(a, DataGroup):
        _assert_identical_datagroup(a, b)
    else:
        assert a == b


def _assert_identical_variable_data(a: Variable, b: Variable) -> None:
    if a.is_binned:
        _assert_identical_binned_variable_data(a, b)
    else:
        _assert_identical_dense_variable_data(a, b)


def _assert_identical_dense_variable_data(a: Variable, b: Variable) -> None:
    with _add_note('values'):
        np.testing.assert_array_equal(
            a.values, b.values, err_msg='when comparing values'
        )
        if a.variances is not None:
            with _add_note('variances'):
                np.testing.assert_array_equal(
                    a.variances, b.variances, err_msg='when comparing variances'
                )


def _assert_identical_binned_variable_data(a: Variable, b: Variable) -> None:
    assert a.is_binned
    assert b.is_binned
    _assert_identical_impl(a.bins.concat().value, b.bins.concat().value)


def _assert_allclose_data(
    a: _T,
    b: _T,
    **kwargs: Any,
) -> None:
    if isinstance(a, Variable):
        _assert_allclose_variable_data(a, b, **kwargs)
    elif isinstance(a, DataArray):
        _assert_allclose_variable_data(a.data, b.data, **kwargs)
    else:
        raise NotImplementedError


def _assert_allclose_variable_data(a: Variable, b: Variable, **kwargs: Any) -> None:
    if a.is_binned:
        _assert_allclose_binned_variable_data(a, b, **kwargs)
    else:
        _assert_allclose_dense_variable_data(a, b, **kwargs)


def _assert_allclose_dense_variable_data(
    a: Variable,
    b: Variable,
    rtol: Variable | None = None,
    atol: Variable | None = None,
    **kwargs: Any,
) -> None:
    if rtol is not None:
        kwargs['rtol'] = rtol.to(unit='dimensionless').value
    if atol is not None:
        if hasattr(a, 'unit'):
            atol = atol.to(unit=a.unit)
        else:
            atol = atol.to(unit='dimensionless')
        kwargs['atol'] = atol.value

    with _add_note('values'):
        np.testing.assert_allclose(
            a.values, b.values, err_msg='when comparing values', **kwargs
        )
        if a.variances is not None:
            with _add_note('variances'):
                np.testing.assert_allclose(
                    a.variances,
                    b.variances,
                    err_msg='when comparing variances',
                    **kwargs,
                )


def _assert_allclose_binned_variable_data(
    a: Variable, b: Variable, **kwargs: Any
) -> None:
    assert a.is_binned
    assert b.is_binned
    _assert_allclose_impl(a.bins.concat().value, b.bins.concat().value, **kwargs)


@contextmanager
def _add_note(loc: str, *args: str) -> Iterator[None]:
    try:
        yield
    except AssertionError as exc:
        if hasattr(exc, 'add_note'):
            # Needs Python >= 3.11
            exc.add_note(f'PREPOSITION {loc.format(*args)}')
        raise


__all__ = ['assert_allclose', 'assert_identical']
