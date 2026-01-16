# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen
from __future__ import annotations

import inspect
import types
from collections.abc import Iterable, Mapping
from typing import Any, TypeVar

from ._scipp.core import _BinsCoords, _BinsMasks
from .core.cpp_classes import (
    Coords,
    DataArray,
    Dataset,
    DType,
    DTypeError,
    Masks,
    UnitError,
    Variable,
    VariancesError,
)
from .units import dimensionless

_T = TypeVar('_T')

_dict_likes = [
    (Dataset, DataArray),
    (Coords, Variable),
    (Masks, Variable),
    (_BinsCoords, Variable),
    (_BinsMasks, Variable),
]


def _make_dict_accessor_signature(value_type: type) -> list[inspect.Signature]:
    base_params = [
        inspect.Parameter(name='self', kind=inspect.Parameter.POSITIONAL_OR_KEYWORD),
        inspect.Parameter(
            name='key',
            kind=inspect.Parameter.POSITIONAL_OR_KEYWORD,
            annotation=str,
        ),
    ]
    params_with_default = [
        *base_params,
        inspect.Parameter(
            name='default',
            kind=inspect.Parameter.POSITIONAL_OR_KEYWORD,
            annotation=_T,
        ),
    ]
    return [
        inspect.Signature(
            parameters=base_params,
            return_annotation=value_type,
        ),
        inspect.Signature(
            parameters=params_with_default,
            return_annotation=value_type | _T,
        ),
    ]


# Using type annotations here would lead to problems with Sphinx autodoc.
# Type checkers anyway use the stub file
# which is generated from a custom signature override.
def _get(self, key, default=None):  # type: ignore[no-untyped-def]
    """
    Return the value for key if key is in present, else default.
    """
    try:
        return self[key]
    except KeyError:
        return default


def bind_get() -> None:
    for cls, value_type in _dict_likes:
        method = _convert_to_method(name='get', func=_get, abbreviate_doc=False)
        method.__doc__ = """Get the value associated with the provided key or the default value.

Examples
--------
Access a coordinate with a default value:

  >>> import scipp as sc
  >>> da = sc.DataArray(sc.array(dims=['x'], values=[1, 2, 3]))
  >>> da.coords.get('x')  # returns None if 'x' does not exist
  >>> da.coords['x'] = sc.arange('x', 3)
  >>> da.coords.get('x')
  <scipp.Variable> (x: 3)      int64  [dimensionless]  [0, 1, 2]

Access a Dataset item with a default value:

  >>> ds = sc.Dataset({'a': sc.array(dims=['x'], values=[1, 2, 3])})
  >>> ds.get('b', sc.DataArray(sc.zeros(dims=['x'], shape=[3])))
  <scipp.DataArray>
  Dimensions: Sizes[x:3, ]
  Data:
                              float64  [dimensionless]  (x)  [0, 0, 0]
"""  # noqa: E501
        method.__signature__ = _make_dict_accessor_signature(  # type: ignore[attr-defined]
            value_type
        )
        cls.get = method


def _expect_dimensionless_or_unitless(x: Variable) -> None:
    if x.unit is not None and x.unit != dimensionless:
        raise UnitError(f'Expected unit dimensionless or no unit, got {x.unit}.')


def _expect_no_variance(x: Variable) -> None:
    if x.variance is not None:
        raise VariancesError('Expected input without variances.')


def _expect_integer(x: Variable) -> None:
    if x.dtype not in (DType.int64, DType.int32):
        raise DTypeError(
            f'Only integer dtypes can be converted to index, got {x.dtype}'
        )


def _int_dunder(self: Variable) -> int:
    _expect_dimensionless_or_unitless(self)
    _expect_no_variance(self)
    return int(self.value)


def _float_dunder(self: Variable) -> float:
    _expect_dimensionless_or_unitless(self)
    _expect_no_variance(self)
    return float(self.value)


def _index_dunder(self: Variable) -> int:
    """Convert self to an integer.

    Requires that ``self``

    - has integer dtype,
    - is dimensionless,
    - is a scalar,
    - has no variance.

    This method exists primarily for the benefit of type-checking label-based indexing.
    Note that label-based indices may have a float dtype even though ``__index__``
    does not allow it.
    """
    _expect_dimensionless_or_unitless(self)
    _expect_no_variance(self)
    _expect_integer(self)
    return int(self.value)


def bind_conversion_to_builtin(cls: Any) -> None:
    cls.__int__ = _convert_to_method(
        name='__int__', func=_int_dunder, abbreviate_doc=False
    )
    cls.__float__ = _convert_to_method(
        name='__float__', func=_float_dunder, abbreviate_doc=False
    )
    cls.__index__ = _convert_to_method(
        name='__index__', func=_index_dunder, abbreviate_doc=False
    )


class _NoDefaultType:
    def __repr__(self) -> str:
        return 'NotSpecified'


_NoDefault = _NoDefaultType()


def _pop(self, key, default=_NoDefault):  # type: ignore[no-untyped-def]  # see _get
    """
    Remove and return an element.

    If key is not found, default is returned if given, otherwise KeyError is raised.

    Examples
    --------
    Remove a coordinate from a DataArray:

      >>> import scipp as sc
      >>> da = sc.DataArray(
      ...     sc.array(dims=['x'], values=[1.0, 2.0, 3.0]),
      ...     coords={'x': sc.arange('x', 3), 'y': sc.arange('x', 3) * 10}
      ... )
      >>> da.coords.pop('y')
      <scipp.Variable> (x: 3)      int64  [dimensionless]  [0, 10, 20]
      >>> 'y' in da.coords
      False

    Pop with default value for missing key:

      >>> da.coords.pop('z', sc.scalar(0))
      <scipp.Variable> ()      int64  [dimensionless]  0

    Remove an item from a Dataset:

      >>> ds = sc.Dataset({'a': sc.array(dims=['x'], values=[1, 2, 3]),
      ...                  'b': sc.array(dims=['x'], values=[4, 5, 6])})
      >>> ds.pop('b')
      <scipp.DataArray>
      ...
      >>> list(ds.keys())
      ['a']
    """
    if key not in self and default is not _NoDefault:
        return default
    return self._pop(key)


def bind_pop() -> None:
    for cls, value_type in _dict_likes:
        method = _convert_to_method(name='pop', func=_pop, abbreviate_doc=False)
        method.__signature__ = _make_dict_accessor_signature(  # type: ignore[attr-defined]
            value_type
        )
        cls.pop = method


def bind_functions_as_methods(
    cls: type, namespace: Mapping[str, types.FunctionType], func_names: Iterable[str]
) -> None:
    for func_name, func in ((n, namespace[n]) for n in func_names):
        bind_function_as_method(cls=cls, name=func_name, func=func)


# Ideally, `func` would be annotated as `types.FunctionType`.
# But that makes mypy flag calls to `bind_function_as_method` as errors
# because functions are instances of `Callable` but not of `FunctionType`.
# Note, using `Callable` does not work because it only defines `__call__`,
# but not define the required attributes.
def bind_function_as_method(
    *, cls: type, name: str, func: Any, abbreviate_doc: bool = True
) -> None:
    setattr(
        cls,
        name,
        _convert_to_method(name=name, func=func, abbreviate_doc=abbreviate_doc),
    )


def _convert_to_method(
    *, name: str, func: Any, abbreviate_doc: bool = True
) -> types.FunctionType:
    method = types.FunctionType(
        func.__code__, func.__globals__, name, func.__defaults__, func.__closure__
    )
    method.__kwdefaults__ = func.__kwdefaults__
    method.__annotations__ = func.__annotations__
    if func.__doc__ is not None:
        # Extract the summary from the docstring.
        # This relies on check W293 in flake8 to avoid implementing a more
        # sophisticate / expensive parser that running during import of scipp.
        # Line feeds are replaced because they mess with the
        # reST parser of autosummary.
        if abbreviate_doc:
            method.__doc__ = (
                func.__doc__.split('\n\n', 1)[0].replace('\n', ' ')
                + f'\n\n:seealso: Details in :py:meth:`scipp.{name}`'
            )
        else:
            method.__doc__ = func.__doc__
    if hasattr(func, '__wrapped__'):
        method.__wrapped__ = func.__wrapped__  # type: ignore[attr-defined]
    return method
