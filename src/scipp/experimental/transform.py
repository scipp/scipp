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
from .. import core

import numba
from inspect import signature


def _as_numba_cfunc(function, unit_kernel=None):
    dtype = 'double'
    narg = len(signature(function).parameters)
    cfunc = numba.cfunc(dtype + '(' + ','.join([dtype] * narg) + ')')(function)
    cfunc.unit_func = function if unit_kernel is None else unit_kernel
    return cfunc


def transform(func, unit_func=None, dtype='float64', auto_convert_dtypes=False):
    """Transform one or more variables using a custom kernel.

    Only variables with dtype=float64 are supported.
    Variances are not supported.

    :param kernel: Kernel to use for the transform. Can be a plain function or
                   a function decorated using `@scipp.experimental.transform_kernel()`
    """
    from .._scipp.core import experimental_transform
    if dtype != 'float64':
        raise RuntimeError('Only float64 arguments supported at this point')
    else:
        dtype = 'double'
    func = _as_numba_cfunc()(func, unit_func=unit_func)

    def func(*args):
        if auto_convert_dtypes:
            args = [arg.to(dtype='float64', copy=False) for arg in args]
        return experimental_transform(func, *args)

    return func
