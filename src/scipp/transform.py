# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from typing import Callable, Optional
import numba
from inspect import signature
from .core import Variable
from ._scipp.core import experimental_transform


def _as_numba_cfunc(function, unit_func=None):
    dtype = 'double'
    narg = len(signature(function).parameters)
    cfunc = numba.cfunc(dtype + '(' + ','.join([dtype] * narg) + ')')(function)
    cfunc.unit_func = function if unit_func is None else unit_func
    return cfunc


def transform(func: Callable,
              unit_func: Optional[Callable] = None,
              dtype: str = 'float64',
              auto_convert_dtypes: bool = False) -> Callable:
    """
    Create a function for transforming input variables based on element-wise operation.

    This uses ``numba.cfunc`` to compile a kernel that Scipp can use for transforming
    the variable contents. Only variables with dtype=float64 are supported. Variances
    are not supported.

    Parameters
    ----------
    func:
        Function to compute an output element from input element values.
    unit_func:
        Function to compute the output unit. If ``None``, ``func`` wil be used.
    dtype:
        Must be 'float64'.
    auto_convert_dtypes:
        Set to ``True`` to automatically convert all inputs to ``dtype``.

    Returns
    -------
    :
        A callable that applies ``func`` to the elements of the variables passed to it.

    Examples
    --------

      >>> from scipp.transformations import transform
    """
    if dtype != 'float64':
        raise RuntimeError('Only float64 arguments supported at this point')
    else:
        dtype = 'double'
    func = _as_numba_cfunc(func, unit_func=unit_func)

    def transform_custom(*args: Variable) -> Variable:
        if auto_convert_dtypes:
            args = [arg.to(dtype='float64', copy=False) for arg in args]
        return experimental_transform(func, *args)

    return transform_custom
