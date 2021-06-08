# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen

import types

from ._scipp import core
from .utils import get


def bind_get():
    for cls in [core.Dataset, core.Coords, core.Masks]:
        setattr(cls, 'get', get)


def bind_functions_as_methods(cls, namespace, func_names):
    for func_name, func in map(lambda n: (n, namespace[n]), func_names):
        method = types.FunctionType(func.__code__, func.__globals__, func_name,
                                    func.__defaults__, func.__closure__)
        # Extract the summary from the docstring.
        # This relies on check W293 in flake8 to avoid implementing a more
        # sophisticate / expensive parser that running during import of scipp.
        # Line feeds are replaced because they mess with the
        # reST parser of autosummary.
        method.__doc__ = (
            func.__doc__.split('\n\n', 1)[0].replace('\n', ' ') +
            f'\n\n:seealso: Details in :py:meth:`scipp.{func_name}`')
        setattr(cls, func_name, method)
