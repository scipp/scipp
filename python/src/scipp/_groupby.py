# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Matthew Andrew
from ._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func


def groupby(data, group, bins=None):
    """Group dataset or data array based on values of specified labels.

    :param data: Input data to reduce.
    :param group: Name of labels to use for grouping
      or Variable to use for grouping
    :param bins: Optional Bins for grouping label values.
    :type data: Dataset or DataArray
    :type group: str or Variable
    :type bins: Variable
    :return: GroupBy helper object.
    """
    if bins is None:
        return _call_cpp_func(_cpp.groupby, data, group)
    else:
        return _call_cpp_func(_cpp.groupby, data, group, bins)
