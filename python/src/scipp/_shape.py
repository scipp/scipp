# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Matthew Andrew
from ._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func
from typing import Sequence as _Sequence


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


def fold(x, dim, sizes=None, dims=None, shape=None):
    """Fold a single dimension of a variable or data array into multiple dims.

    Examples:

    .. code-block:: python

      sc.fold(a, 'x', {'y': 2, 'z': 3})
      sc.fold(a, 'x', dims=['y', 'x'], shape=[2, 3])

    :param x: Variable or DataArray to fold.
    :param dim: A single dim label that will be folded into more dims.
    :param sizes: A dict mapping new dims to new shapes.
    :param dims: A list of new dims labels.
    :param shape: A list of new dim shapes.
    :type x: Variable or DataArray
    :type dim: str
    :type sizes: dict
    :type dims: list[str]
    :type shape: list[int]
    :raises: If the volume of the old shape is not equal to the
             volume of the new shape.
    :return: Variable or DataArray with requested dimension labels and shape.
    """
    if sizes is not None:
        if (dims is not None) or (shape is not None):
            raise RuntimeError(
                "If sizes is defined, dims and shape must be None in fold.")
    else:
        if (dims is None) or (shape is None):
            raise RuntimeError("Both dims and shape must be defined in fold.")

    if dims is None:
        return _call_cpp_func(_cpp.fold, x, dim, sizes)
    else:
        return _call_cpp_func(_cpp.fold, x, dim, dict(zip(dims, shape)))


def flatten(x, dims=None, to=None):
    """Flatten multiple dimensions of a variable or data array into a single
    dimension. If dims is omitted, then we flatten all of the inputs dimensions
    into a single dim.

    Examples:
    
    .. code-block:: python
    
      sc.flatten(a, dims=['x', 'y'], to='z')
      sc.flatten(a, to='z')

    :param x: Variable or DataArray to flatten.
    :param dims: A list of dim labels that will be flattened.
    :param to: A single dim label for the resulting flattened dim.
    :type x: Variable or DataArray
    :type dims: list[str]
    :type to: str
    :raises: If the bin edge coordinates cannot be stitched back together.
    :return: Variable or DataArray with requested dimension labels and shape.
    """
    if to is None:
        # Note that this is a result of the fact that we want to support
        # calling flatten without kwargs, and that in this case it semantically
        # makes more sense for the dims that we want to flatten to come first
        # in the argument list.
        raise RuntimeError("The final flattened dimension is required.")
    if dims is None:
        dims = x.dims
    return _call_cpp_func(_cpp.flatten, x, dims, to)


def transpose(x, dims: _Sequence[str]):
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
