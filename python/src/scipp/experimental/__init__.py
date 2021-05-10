# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock

# flake8: noqa


def kernel(dtype='float64'):
    if dtype != 'float64':
        raise RuntimeError('Only float64 arguments supported at this point')
    else:
        dtype = 'double'
    import numba
    from inspect import signature

    def decorator(function):
        narg = len(signature(function).parameters)
        function = numba.cfunc(dtype + '(' + ','.join([dtype] * narg) +
                               ')')(function)

        def wrapper(*args, **kwargs):
            return function(*args, **kwargs)

        return wrapper

    return decorator


from .._scipp.core import experimental_transform_unary as transform_unary
from .._scipp.core import experimental_transform_binary as transform_binary
from .._scipp.core import experimental_transform_ternary as transform_ternary
