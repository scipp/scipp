# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Matthew Andrew
from .._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func


def combine_masks(masks, labels, shape):
    """
    Combine all masks into a single one following the OR operation.
    This requires a masks view as an input, followed by the
    dimension labels and shape of the Variable/DataArray. The
    labels and the shape are used to create a Dimensions object.
    The function then iterates through the masks view and combines
    only the masks that have all their dimensions contained in the
    Variable/DataArray Dimensions.

    :param masks: Masks view of the dataset's masks.
    :param labels: A list of dimension labels.
    :param shape: A list of dimension extents.
    :type masks: MaskView
    :type labels: list
    :type shape: list
    :return: A new variable that contains the union of all masks.
    :rtype: Variable
    """
    return _call_cpp_func(_cpp.combine_masks, labels, shape)


def merge(lhs: _cpp.Dataset, rhs: _cpp.Dataset) -> _cpp.Dataset:
    """Merge two datasets into one.

    :param lhs: First dataset.
    :param rhs: Second dataset.
    :raises: If there are conflicting items with different content.
    :return: A new dataset that contains the union of all data items,
             coords, masks and attributes.
    """
    return _call_cpp_func(_cpp.merge, lhs, rhs)
