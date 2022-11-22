# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock

# flake8: noqa
"""
Staging area for experimental and untested scipp features.

Contents of this submodule are subject to changes and removal without
notice.
"""

from copy import copy
import math
from types import MethodType

from numba import types as nbtypes
from numba.extending import overload

from .._scipp.core import Unit
from .. import core


def kernel_unit_rmul(self, other):
    if isinstance(other, Unit):
        return self.__regular_rmul__(other)
    return self


def kernel_unit_rtruediv(self, other):
    if isinstance(other, Unit):
        return self.__regular_rtruediv__(other)
    return self


def process_unit(unit):
    unit = Unit(str(unit))
    unit.__regular_rmul__ = unit.__rmul__
    unit.__rmul__ = MethodType(kernel_unit_rmul, unit)
    unit.__regular_rtruediv__ = unit.__rtruediv__
    unit.__rtruediv__ = MethodType(kernel_unit_rtruediv, unit)
    return unit


def transform_kernel(dtype='float64'):
    if dtype != 'float64':
        raise RuntimeError('Only float64 arguments supported at this point')
    else:
        dtype = 'double'
    import numba
    from inspect import signature

    def decorator(function, unit_kernel=None):
        narg = len(signature(function).parameters)
        cfunc = numba.cfunc(dtype + '(' + ','.join([dtype] * narg) + ')')(function)

        def unit_func(*args):
            return function(*(process_unit(unit) for unit in args))

        cfunc.unit_func = unit_func if unit_kernel is None else unit_kernel
        return cfunc

    return decorator


def transform(kernel, *args, unit_kernel=None):
    """Transform one or more variables using a custom kernel.

    Only variables with dtype=float64 are supported.
    Variances are not supported.

    :param kernel: Kernel to use for the transform. Can be a plain function or
                   a function decorated using `@scipp.experimental.transform_kernel()`
    """
    from .._scipp.core import experimental_transform
    if not hasattr(kernel, 'address'):
        kernel = transform_kernel()(kernel, unit_kernel=unit_kernel)
    return experimental_transform(kernel, *args)


# need extra function so that resolver is a new function for each wrapped func
# if inlined in make_numba_overloads, the repeated defs overwrite each other
def make_numba_overload(sc_func, impl):

    @overload(sc_func, inline='always')
    def resolver(x):
        if isinstance(x, nbtypes.Float):

            def impl_wrapper(x):
                return impl(x)

            return impl_wrapper


def make_numba_overloads(func_tuples):
    for sc_func, impl in func_tuples:
        make_numba_overload(sc_func, impl)


# TODO This is just an example,
#      apart from sqrt, the scipp functions do not support units.
make_numba_overloads([(core.sqrt, math.sqrt), (core.abs, abs), (core.sin, math.sin)])
