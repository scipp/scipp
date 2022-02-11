# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen

import types

from ._scipp import core
from .utils import get

_dict_likes = [
    core.Dataset, core.Coords, core.Masks, core._BinsMeta, core._BinsCoords,
    core._BinsMasks, core._BinsAttrs
]


def bind_get():
    for cls in _dict_likes:
        method = _convert_to_method(name='get', func=get, abbreviate_doc=False)
        method.__doc__ = "Get the value associated with the " \
                         "provided key or the default value."
        setattr(cls, 'get', method)


class _NoDefaultType:
    def __repr__(self):
        return 'NotSpecified'


_NoDefault = _NoDefaultType()


def _pop(self, name, default=_NoDefault):
    """
    Remove and return an element.

    :param name: Key to remove.
    :param default: If the mapping does not contain an element `name`,
                    return this value or raise `KeyError` is no default is given.
    """
    if name not in self and default is not _NoDefault:
        return default
    return self._pop(name)


def bind_pop():
    for cls in _dict_likes:
        _bind_function_as_method(cls=cls, name='pop', func=_pop, abbreviate_doc=False)


def bind_functions_as_methods(cls, namespace, func_names):
    for func_name, func in map(lambda n: (n, namespace[n]), func_names):
        _bind_function_as_method(cls=cls, name=func_name, func=func)


def _bind_function_as_method(*, cls, name, func, abbreviate_doc=True):
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
