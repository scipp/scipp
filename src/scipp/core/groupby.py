# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Matthew Andrew


from .._scipp import core as _cpp


def groupby(
    data: _cpp.DataArray | _cpp.Dataset,
    /,
    group: _cpp.Variable | str,
    *,
    bins: _cpp.Variable | None = None,
) -> _cpp.GroupByDataArray | _cpp.GroupByDataset:
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
