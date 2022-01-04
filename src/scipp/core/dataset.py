# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Matthew Andrew
from .._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func


def irreducible_mask(masks: _cpp.Masks, dim: str) -> _cpp.Variable:
    """
    The union of all masks with irreducible dimension.

    Irreducible means that a reduction operation must apply these masks since
    they depend on the reduction dimension.

    Returns None if there is no irreducible mask.
    """
    return _call_cpp_func(_cpp.irreducible_mask, masks, dim)


def merge(lhs: _cpp.Dataset, rhs: _cpp.Dataset) -> _cpp.Dataset:
    """Merge two datasets into one.

    :param lhs: First dataset.
    :param rhs: Second dataset.
    :raises: If there are conflicting items with different content.
    :return: A new dataset that contains the union of all data items,
             coords, masks and attributes.
    """
    return _call_cpp_func(_cpp.merge, lhs, rhs)
