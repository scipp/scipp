# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from ._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func


def to_buckets(data, dim, begin=None, end=None):
    """Create variable with elements defined as slices of `data` along `dim`.

    The elements of the returned variable are "buckets", defined as views into
    `data`. The returned variable keeps and manages a copy of `data` internally.

    The variables `begin` and `end` must have the same dims and shape and
    `dtype=sc.dtype.int64`. The output dims and shape are given by `begin`.
    If only `begin` is given, each bucket is a slice containing a non-range
    slice of `data` at the given indices. If neither `begin` nor `end` are
    given, the output has `dims=[dim]` and contains all non-range slices along
    that dimension.

    :param data: Variable, DataArray, or Dataset to map into buckets.
    :param dim: Dimension for selecting slices of `data`.
    :param begin: Variable with begin-indices for slicing `data` (optional).
    :param end: Variable with end-indices for slicing `data` (optional).
    :return: Variable with elements that are view into `data`.
    """
    return _call_cpp_func(_cpp.to_buckets,
                          dim=dim,
                          data=data,
                          begin=begin,
                          end=end)
