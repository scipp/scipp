# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Matthew Andrew
from ._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func


def counts_to_density(x, dim):
    """Converts counts to count density on a given dimension.

    :param x: Data as counts.
    :param dim: Dimension on which to convert.
    :type x: Dataset or DataArray
    :type dim: str
    :rtype: Dataset or DataArray
    :return: Data as count density.
    """
    return _call_cpp_func(_cpp.counts_to_density, x, dim)


def density_to_counts(x, dim):
    """Converts counts to count density on a given dimension.

    :param x: Data as count density.
    :param dim: Dimension on which to convert.
    :type x: Dataset or DataArray
    :type dim: str
    :rtype: Dataset or DataArray
    :return: Data as counts.
    """
    return _call_cpp_func(_cpp.density_to_counts, x, dim)
