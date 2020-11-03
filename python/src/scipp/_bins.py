# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from ._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func


def bin_sum(x):
    """Sum of each bin.

    :param x: Input data.
    :raises: If `x` is not binned data.
    :return: The sum of each of the input bins.
    :seealso: :py:func:`scipp.sum` for summing non-binned data
    """
    return _call_cpp_func(_cpp.buckets.sum, x)


def bin_size(x):
    """Number of events or elements in a bin.

    :param x: Input data.
    :raises: If `x` is not binned data.
    :return: The number of elements in each of the input bins.
    """
    return _call_cpp_func(_cpp.bin_size, x)


def merge_bins(x, y=None, dim=None, out=None):
    """Merge bins element-wise by concatenating bin contents along the
    internal dimension.

    The bins to merge are either obtained element-wise from the two inputs `x`
    and `y`, or, if `dim` but not `y` is given, from all bins along the given
    dimension.

    :param x: First input containing bins.
    :param y: Optional second input containing bins.
    :param dim: Optional dimension along which to merge bins. If not given and
                `y` is `None`, the bins are merged along all dimensions.
    :param out: Optional output buffer.
    :raises: If `x` or `y` is not binned data.
    :return: The bins of the two inputs merged.
    """
    if y is not None and dim is not None:
        raise RuntimeError(
            "`merge_bins` requires either two arguments (x and y) or x and a"
            "dim, but not both.")
    if y is not None:
        if out is None:
            return _call_cpp_func(_cpp.buckets.concatenate, x, y)
        else:
            if x is out:
                _call_cpp_func(_cpp.buckets.append, x, y)
            else:
                out = _call_cpp_func(_cpp.buckets.concatenate, x, y)
            return out
    if out is not None:
        raise RuntimeError("`out` arg not support for merge_bins along dim")
    if dim is not None:
        return _call_cpp_func(_cpp.buckets.concatenate, x, dim)
    raise RuntimeError("Reduction along all dims not supported yet.")
