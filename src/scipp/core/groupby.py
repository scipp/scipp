# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Matthew Andrew

from typing import Optional, Union

from .._scipp import core as _cpp


def groupby(
    data: Union[_cpp.DataArray, _cpp.Dataset],
    /,
    group: Union[_cpp.Variable, str],
    *,
    bins: Optional[_cpp.Variable] = None
) -> Union[_cpp.GroupByDataArray, _cpp.GroupByDataset]:
    """Group dataset or data array based on values of specified labels.

    Parameters
    ----------
    data:
        Input data to reduce.
    group:
        Name of labels to use for grouping or Variable to use for grouping
    bins:
        Optional bins for grouping label values.

    Returns
    -------
    :
        GroupBy helper object.
    """
    if bins is None:
        return _cpp.groupby(data, group)
    else:
        return _cpp.groupby(data, group, bins)
