# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen

import types

from ._scipp import core

_dict_likes = [
    core.Dataset, core.Coords, core.Masks, core._BinsMeta, core._BinsCoords,
    core._BinsMasks, core._BinsAttrs
]


def _get(self, key, default=None):
    """
    Return the value for key if key is in present, else default.
    """
    try:
        return self[key]
    except KeyError:
        return default


def bind_get():
    for cls in _dict_likes:
        method = _convert_to_method(name='get', func=_get, abbreviate_doc=False)
        method.__doc__ = "Get the value associated with the " \
                         "provided key or the default value."
        setattr(cls, 'get', method)


def _expect_dimensionless_or_unitless(x):
    if x.unit is not None and x.unit != core.units.dimensionless:
        raise core.UnitError(f'Expected unit dimensionless or no unit, got {x.unit}.')


def _expect_no_variance(x):
    if x.variance is not None:
        raise core.VariancesError('Expected input without variances.')


def _int_dunder(self) -> int:
    _expect_dimensionless_or_unitless(self)
    _expect_no_variance(self)
    return int(self.value)


def _float_dunder(self) -> float:
    _expect_dimensionless_or_unitless(self)
    _expect_no_variance(self)
    return float(self.value)


def bind_conversion_to_builtin(cls):
    setattr(cls, '__int__', _convert_to_method(name='__int__', func=_int_dunder))
    setattr(cls, '__float__', _convert_to_method(name='__float__', func=_float_dunder))


class _NoDefaultType:

    def __repr__(self):
        return 'NotSpecified'


_NoDefault = _NoDefaultType()


def _pop(self, key, default=_NoDefault):
    """
    Remove and return an element.

    If key is not found, default is returned if given, otherwise KeyError is raised.
    """
    if key not in self and default is not _NoDefault:
        return default
    return self._pop(key)


def bind_pop():
    for cls in _dict_likes:
        bind_function_as_method(cls=cls, name='pop', func=_pop, abbreviate_doc=False)


def bind_functions_as_methods(cls, namespace, func_names):
    for func_name, func in map(lambda n: (n, namespace[n]), func_names):
        bind_function_as_method(cls=cls, name=func_name, func=func)


def bind_function_as_method(*, cls, name, func, abbreviate_doc=True):
    setattr(cls, name,
            _convert_to_method(name=name, func=func, abbreviate_doc=abbreviate_doc))


def _convert_to_method(*, name, func, abbreviate_doc=True):
    method = types.FunctionType(func.__code__, func.__globals__, name,
                                func.__defaults__, func.__closure__)
    method.__kwdefaults__ = func.__kwdefaults__
    method.__annotations__ = func.__annotations__
    if func.__doc__ is not None:
        # Extract the summary from the docstring.
        # This relies on check W293 in flake8 to avoid implementing a more
        # sophisticate / expensive parser that running during import of scipp.
        # Line feeds are replaced because they mess with the
        # reST parser of autosummary.
        if abbreviate_doc:
            method.__doc__ = (func.__doc__.split('\n\n', 1)[0].replace('\n', ' ') +
                              f'\n\n:seealso: Details in :py:meth:`scipp.{name}`')
        else:
            method.__doc__ = func.__doc__
    return method
