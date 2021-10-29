# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Matthew Andrew

from typing import Union

from .._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func


def counts_to_density(x: Union[_cpp.DataArray, _cpp.Dataset],
                      dim: str) -> Union[_cpp.DataArray, _cpp.Dataset]:
    """Converts counts to count density on a given dimension.

    :param x: Data as counts.
    :param dim: Dimension on which to convert.
    :return: Data as count density.
    """
    return _call_cpp_func(_cpp.counts_to_density, x, dim)


def density_to_counts(x: Union[_cpp.DataArray, _cpp.Dataset],
                      dim: str) -> Union[_cpp.DataArray, _cpp.Dataset]:
    """Converts counts to count density on a given dimension.

    :param x: Data as count density.
    :param dim: Dimension on which to convert.
    :return: Data as counts.
    """
    return _call_cpp_func(_cpp.density_to_counts, x, dim)
