# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Matthew Andrew
from ._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func
from typing import Sequence


def broadcast(x, dims, shape):
    """Broadcast a variable.

    Note that scipp operations broadcast automatically, so using this function
    directly is rarely required.

    :param x: Variable to broadcast.
    :param dims: List of new dimensions.
    :param shape: New extents in each dimension.
    :type x: Variable
    :type dims: list[str]
    :type shape: list[int]
    :return: New variable with requested dimension labels and shape.
    """
    return _call_cpp_func(_cpp.broadcast, x, dims, shape)


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
    :param dim: Dimension along which to concatenate.
    :type x: Dataset, DataArray or Variable. Must be same type as y
    :type y: Dataset, DataArray or Variable. Must be same type as x
    :type dim: str
    :raises: If the dtype or unit does not match, or if the
             dimensions and shapes are incompatible.
    :return: The absolute values of the input.
    """
    return _call_cpp_func(_cpp.concatenate, x, y, dim)


def reshape(x, sizes=None, dims=None, shape=None):
    """Reshape a variable.

    Examples:
      a = sc.array(dims=['x'], values=[1, 2, 3, 4, 5, 6])
      sc.reshape(var, sizes={'y': 2, 'z': 3})
      sc.reshape(var, dims=['y', 'z'], shape=(2, 3))

    :param x: Variable to reshape.
    :param sizes: A dict mapping new dims to new shapes.
    :param dims: A list of new dims labels.
    :param shape: A list of new dim shapes.
    :type x: Variable
    :type sizes: dict
    :type dims: list[str]
    :type shape: list[int
    :raises: If the volume of the old shape is not equal to the
             volume of the new shape.
    :return: Variable with requested dimension labels and shape.
    """
    if dims is None:
        return _call_cpp_func(_cpp.reshape, x, sizes)
    else:
        return _call_cpp_func(_cpp.reshape, x, dict(zip(dims, shape)))


def split(x, dim, sizes=None, dims=None, shape=None):
    """Split a single dimension of a data array into multiple dims.

    Examples:
      sc.split(da, 'x', {'y': 2, 'z': 3})
      sc.split(da, 'x', dims=['y', 'x'], shape=[2, 3])

    :param x: DataArray to split.
    :param dim: A single dim label that will be split into more dims.
    :param to_dims: A dict mapping new dims to new shapes.
    :type x: DataArray
    :type dim: str
    :type to_dims: dict
    :raises: If the volume of the old shape is not equal to the
             volume of the new shape.
    :return: DataArray with requested dimension labels and shape.
    """
    if dims is None:
        return _call_cpp_func(_cpp.split, x, dim, sizes)
    else:
        return _call_cpp_func(_cpp.split, x, dim, dict(zip(dims, shape)))


def flatten(x, dims=None, dim=None):
    """Flatten or flatten multiple dimensions of a data array into a single
    dimension. If dims is omitted, then we flatten all of the inputs dimensions
    into a single dim.

    Examples:
      sc.flatten(da, dims=['x', 'y'], dim='z')
      sc.flatten(da, dim='z')

    :param x: DataArray to flatten.
    :param dims: A list of dim labels that will be flattened.
    :param dim: A single dim label for the resulting flattened dim.
    :type x: DataArray
    :type dims: list[str]
    :type dim: str
    :raises: If the bin edge coordinates cannot be stitched back together.
    :return: DataArray with requested dimension labels and shape.
    """
    if dims is None:
        dims = x.dims
    return _call_cpp_func(_cpp.flatten, x, dims, dim)


def transpose(x, dims: Sequence[str]):
    """Transpose dimensions of a variable.

    :param x: Variable to transpose.
    :param dims: List of dimensions in desired order. If default,
                        reverses existing order.
    :type x: Variable
    :type dims: list[str]
    :raises: If the dtype or unit does not match, or if the
             dimensions and shapes are incompatible.
    :return: The absolute values of the input.
    """
    return _call_cpp_func(_cpp.transpose, x, dims)
