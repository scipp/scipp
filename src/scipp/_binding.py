# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen
from __future__ import annotations

import inspect
import types
from collections.abc import Iterable, Mapping
from typing import Any

from ._scipp import core
from .core.cpp_classes import DataArray, Variable

_dict_likes = [
    (core.Dataset, core.DataArray),
    (core.Coords, core.Variable),
    (core.Masks, core.Variable),
    (core._BinsMeta, core.Variable),
    (core._BinsCoords, core.Variable),
    (core._BinsMasks, core.Variable),
    (core._BinsAttrs, core.Variable),
]


def _make_dict_accessor_signature(
    value_type: type, has_default: bool
) -> inspect.Signature:
    params = [
        inspect.Parameter(name='self', kind=inspect.Parameter.POSITIONAL_OR_KEYWORD),
        inspect.Parameter(
            name='key',
            kind=inspect.Parameter.POSITIONAL_OR_KEYWORD,
            annotation=str,
        ),
    ]
    if has_default:
        params.append(
            inspect.Parameter(
                name='default',
                kind=inspect.Parameter.POSITIONAL_OR_KEYWORD,
                annotation=value_type | None,
                default=None,
            )
        )
    sig = inspect.Signature(
        parameters=params,
        return_annotation=value_type | None,
    )
    return sig


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
        method.__doc__ = (
            "Get the value associated with the " "provided key or the default value."
        )
        method.__signature__ = _make_dict_accessor_signature(  # type: ignore[attr-defined]
            value_type, has_default=True
        )
        cls.get = method


def _expect_dimensionless_or_unitless(x: Variable | DataArray) -> None:
    if x.unit is not None and x.unit != core.units.dimensionless:
        raise core.UnitError(f'Expected unit dimensionless or no unit, got {x.unit}.')


def _expect_no_variance(x: Variable | DataArray) -> None:
    if x.variance is not None:
        raise core.VariancesError('Expected input without variances.')


def _int_dunder(self: Variable | DataArray) -> int:
    _expect_dimensionless_or_unitless(self)
    _expect_no_variance(self)
    return int(self.value)


def _float_dunder(self: Variable | DataArray) -> float:
    _expect_dimensionless_or_unitless(self)
    _expect_no_variance(self)
    return float(self.value)


def bind_conversion_to_builtin(cls: Any) -> None:
    cls.__int__ = _convert_to_method(name='__int__', func=_int_dunder)
    cls.__float__ = _convert_to_method(name='__float__', func=_float_dunder)


class _NoDefaultType:
    def __repr__(self) -> str:
        return 'NotSpecified'


_NoDefault = _NoDefaultType()


def _pop(self, key, default=_NoDefault):  # type: ignore[no-untyped-def]  # see _get
    """
    Remove and return an element.

    If key is not found, default is returned if given, otherwise KeyError is raised.
    """
    if key not in self and default is not _NoDefault:
        return default  # type: ignore[return-value]  # the `if` above is a type check
    return self._pop(key)  # type: ignore[attr-defined, no-any-return]


def bind_pop() -> None:
    for cls, value_type in _dict_likes:
        method = _convert_to_method(name='pop', func=_pop, abbreviate_doc=False)
        method.__signature__ = _make_dict_accessor_signature(  # type: ignore[attr-defined]
            value_type, has_default=True
        )
        cls.pop = method


def bind_functions_as_methods(
    cls: type, namespace: Mapping[str, types.FunctionType], func_names: Iterable[str]
) -> None:
    for func_name, func in ((n, namespace[n]) for n in func_names):
        bind_function_as_method(cls=cls, name=func_name, func=func)


# Ideally, `func` would be annotated as `types.FunctionType`.
# But that makes mypy flag calls to `bind_function_as_method` as errors
# because functions are instances of `Callable` but not of `FUnctionType`.
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
