# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Matthew Andrew
from ._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func
from typing import Sequence


def concatenate(x, y, dim):
    """Concatenate input data array along the given dimension.

    Concatenation can happen in two ways:
    - Along an existing dimension, yielding a new dimension extent
      given by the sum of the input's extents.
    - Along a new dimension that is not contained in either of the inputs,
      yielding an output with one extra dimensions.

    In the case of a data array or dataset, the coords, and masks are also
    concatenated.
    Coords, and masks for any but the given dimension are required to match
    and are copied to the output without changes.

    :param x: Left hand side input.
    :param y: Right hand side input.
    :param dim (Dim): Dimension along which to concatenate.
    :raises: If the dtype or unit does not match, or if the
             dimensions and shapes are incompatible.
    :return: The absolute values of the input.
    """
    return _call_cpp_func(_cpp.concatenate, x, y, dim)


def reshape(x, dims, shape):
    """Reshape a variable.

    :param x (Variable): Variable to reshape.
    :param dims (list): List of new dimensions.
    :param shape (list): New extents in each dimension.
    :raises: If the volume of the old shape is not equal to the
             volume of the new shape.
    :return: New variable with requested dimension labels and shape.
    """
    return _call_cpp_func(_cpp.reshape, x, dims, shape)


def transpose(x, dims: Sequence[str]):
    """Transpose dimensions of a variable.

    :param x (Variable): Variable to transpose.
    :param dims (list[str]): List of dimensions in desired order. If default,
                        reverses existing order.
    :raises: If the dtype or unit does not match, or if the
             dimensions and shapes are incompatible.
    :return: The absolute values of the input.
    """
    return _call_cpp_func(_cpp.transpose, x, dims)