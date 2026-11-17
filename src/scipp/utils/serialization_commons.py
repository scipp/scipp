# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Terence Tsang

from typing import Any

from scipp.core import Bins

__all__ = ['should_reduce_binned_data']


def should_reduce_binned_data(bins: Bins[Any], threshold: float = 1.5) -> bool:
    """
    Check if we should deep-copy before serialization.

    Crude mechanism to avoid writing large buffers, e.g., from
    overallocation or when writing a slice of a larger variable. The
    copy causes some overhead, but so would the (much more complicated)
    solution to extract contents bin-by-bin. This approach will likely
    need to be revisited in the future.

    Args
    ----
    bins:
        :class:`Bins` object.
    threshold:
        If the underlying data is at least this many times the size of
        the resultant data (whence the ``bins`` are from), recommend a
        deep copy.

    Returns
    -------
    :
        Whether we are to reduce and reindex by making a deep copy
        before serializing the data ``bins`` is associated with.
    """
    params = bins.constituents
    underlying_size = params['data'].sizes[params['dim']]
    used_size: int = bins.size().sum().value
    return underlying_size > threshold * used_size
