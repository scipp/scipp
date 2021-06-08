# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen

from ._scipp import core
from .utils import get


def bind_get():
    for cls in [core.Dataset, core.Coords, core.Masks]:
        setattr(cls, 'get', get)


def bind_functions_as_methods(cls, namespace, func_names):
    for func_name, func in map(lambda n: (n, namespace[n]), func_names):
        method = func.__get__(None, cls)
        # Extract the summary from the docstring.
        # This relies on check W293 in flake8 to avoid implementing a more
        # sophisticate / expensive parser that running during import of scipp.
        method.__doc__ = (
            func.__doc__.split('\n\n', 1)[0] +
            f'\n\n:seealso: Details in :py:meth:`scipp.{func_name}`')
        setattr(cls, func_name, method)
