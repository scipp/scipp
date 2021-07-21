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

import math

from numba import types
from numba.extending import overload

from .. import _math as sc_math
from .. import _trigonometry as sc_trigonometry


def transform_kernel(dtype='float64'):
    if dtype != 'float64':
        raise RuntimeError('Only float64 arguments supported at this point')
    else:
        dtype = 'double'
    import numba
    from inspect import signature

    def decorator(function):
        narg = len(signature(function).parameters)
        return numba.cfunc(dtype + '(' + ','.join([dtype] * narg) +
                           ')')(function)

    return decorator


def transform(kernel, *args):
    """Transform one or more variables using a custom kernel.

    Only variables with dtype=float64 are supported.
    Variances are not supported.

    :param kernel: Kernel to use for the transform. Can be a plain function or
                   a function decorated using `@scipp.experimental.transform_kernel()`
    """
    from .._scipp.core import experimental_transform
    if not hasattr(kernel, 'address'):
        kernel = transform_kernel()(kernel)
    return experimental_transform(kernel, *args)


# need extra function so that resolver is a new function for each wrapped func
# if inlined in make_numba_overloads, the repeated defs overwrite each other
def make_numba_overload(sc_func, impl):
    @overload(sc_func, inline='always')
    def resolver(x):
        if isinstance(x, types.Float):

            def impl_wrapper(x):
                return impl(x)

            return impl_wrapper


def make_numba_overloads(func_tuples):
    for sc_func, impl in func_tuples:
        make_numba_overload(sc_func, impl)


# TODO This is just an example,
#      apart from sqrt, the scipp functions do not support units.
make_numba_overloads([(sc_math.sqrt, math.sqrt), (sc_math.abs, abs),
                      (sc_trigonometry.sin, math.sin)])
