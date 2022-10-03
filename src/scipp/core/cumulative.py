# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock

from typing import Literal, Optional

from .._scipp import core as _cpp
from ..typing import VariableLikeType
from ._cpp_wrapper_util import call_func as _call_cpp_func
from .concepts import transform_data


def cumsum(a: VariableLikeType,
           dim: Optional[str] = None,
           mode: Literal['exclusive', 'inclusive'] = 'inclusive') -> VariableLikeType:
    """Return the cumulative sum along the specified dimension.

    See :py:func:`scipp.sum` on how rounding errors for float32 inputs are handled.

    Parameters
    ----------
    a:
        Input data.
    dim:
        Optional dimension along which to calculate the sum. If not
        given, the cumulative sum along all dimensions is calculated.
    mode:
        Include or exclude the ith element (along dim) in the sum.
        Options are 'inclusive' and 'exclusive'. Defaults to 'inclusive'.

    Returns
    -------
    :
        The cumulative sum of the input values.
    """

    def _cumsum(data):
        if dim is None:
            return _call_cpp_func(_cpp.cumsum, data, mode=mode)
        else:
            return _call_cpp_func(_cpp.cumsum, data, dim, mode=mode)

    return transform_data(a, _cumsum)
