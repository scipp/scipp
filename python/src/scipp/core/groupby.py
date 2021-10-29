# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Matthew Andrew

from typing import Optional, Union

from .._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func


def groupby(
    data: Union[_cpp.DataArray, _cpp.Dataset],
    group: Union[_cpp.Variable, str],
    *,
    bins: Optional[_cpp.Variable] = None
) -> Union[_cpp.GroupByDataArray, _cpp.GroupByDataset]:
    """Group dataset or data array based on values of specified labels.

    :param data: Input data to reduce.
    :param group: Name of labels to use for grouping
      or Variable to use for grouping
    :param bins: Optional bins for grouping label values.
    :return: GroupBy helper object.
    """
    if bins is None:
        return _call_cpp_func(_cpp.groupby, data, group)
    else:
        return _call_cpp_func(_cpp.groupby, data, group, bins)
