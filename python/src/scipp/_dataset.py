# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Matthew Andrew
from ._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func


def rebin(x, dim, bins):
    """Rebin a dimension of a data array.

    :param x: Data to rebin.
    :param dim: Dimension to rebin over.
    :param bins: New bin edges.
    :type x: Dataset or DataArray
    :type dim: str
    :type bins: Variable
    :raises: If data cannot be rebinned, e.g., if the unit is not
             counts, or the existing coordinate is not a bin-edge
             coordinate.
    :return: Data rebinned according to the new coordinate.
    """
    return _call_cpp_func(_cpp.rebin, x, dim, bins)
