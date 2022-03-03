# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Matthew Andrew
# flake8: noqa: E501

from __future__ import annotations
import numpy as np
from typing import Dict, List, Optional, Sequence, Tuple, Union
import warnings

from .._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func
from ..typing import VariableLike


def broadcast(x: _cpp.Variable, dims: Union[List[str], Tuple[str, ...]],
              shape: Sequence[int]) -> _cpp.Variable:
    """Broadcast a variable.

    Note that scipp operations broadcast automatically, so using this function
    directly is rarely required.

    :param x: Variable to broadcast.
    :param dims: List of new dimensions.
    :param shape: New extents in each dimension.
    :return: New variable with requested dimension labels and shape.
    """
    return _call_cpp_func(_cpp.broadcast, x, dims, shape)


def concat(x: Sequence[VariableLike], dim: str) -> VariableLike:
    """Concatenate input arrays along the given dimension.

    Concatenation can happen in two ways:

    - Along an existing dimension, yielding a new dimension extent
      given by the sum of the input's extents.
    - Along a new dimension that is not contained in either of the inputs,
      yielding an output with one extra dimensions.

    In the case of a data array or dataset, the coords and masks are also
    concatenated.
    Coords and masks for any but the given dimension are required to match
    and are copied to the output without changes.

    :param x: Sequence of input variables, data arraus, or datasets.
    :param dim: Dimension along which to concatenate.
    :raises: If the dtype or unit does not match, or if the
             dimensions and shapes are incompatible.
    :return: Concatenation of the inputs.

    Examples:

      >>> a = sc.arange('x', 3)
      >>> b = 100 * sc.arange('x', 3)
      >>> c = sc.concat([a, b], dim='x')
      >>> c
      <scipp.Variable> (x: 6)      int64  [dimensionless]  [0, 1, ..., 100, 200]
      >>> c.values
      array([  0,   1,   2,   0, 100, 200])

      >>> d = sc.concat([a, b], dim='y')
      >>> d
      <scipp.Variable> (y: 2, x: 3)      int64  [dimensionless]  [0, 1, ..., 100, 200]
      >>> d.values
      array([[  0,   1,   2],
             [  0, 100, 200]])

      >>> x = sc.DataArray(sc.arange('x', 3), coords={'x': sc.arange('x', 3)})
      >>> y = sc.DataArray(100 * sc.arange('x', 3), coords={'x': 100 * sc.arange('x', 3)})
      >>> z = sc.concat([x, y], dim='x')
      >>> z
      <scipp.DataArray>
      Dimensions: Sizes[x:6, ]
      Coordinates:
        x                           int64  [dimensionless]  (x)  [0, 1, ..., 100, 200]
      Data:
                                    int64  [dimensionless]  (x)  [0, 1, ..., 100, 200]
      >>> z.values
      array([  0,   1,   2,   0, 100, 200])
    """
    return _call_cpp_func(_cpp.concat, x, dim)


def fold(x: VariableLike,
         dim: str,
         sizes: Optional[Dict[str, int]] = None,
         dims: Optional[Union[List[str], Tuple[str, ...]]] = None,
         shape: Optional[Sequence[int]] = None) -> VariableLike:
    """Fold a single dimension of a variable or data array into multiple dims.

    :param x: Variable or DataArray to fold.
    :param dim: A single dim label that will be folded into more dims.
    :param sizes: A dict mapping new dims to new shapes.
    :param dims: A list of new dims labels.
    :param shape: A list of new dim shapes.
    :raises: If the volume of the old shape is not equal to the
             volume of the new shape.
    :return: Variable or DataArray with requested dimension labels and shape.

    Examples:

      >>> v = sc.arange('x', 6)
      >>> v
      <scipp.Variable> (x: 6)      int64  [dimensionless]  [0, 1, ..., 4, 5]
      >>> sc.fold(v, dim='x', sizes={'y': 2, 'z': 3})
      <scipp.Variable> (y: 2, z: 3)      int64  [dimensionless]  [0, 1, ..., 4, 5]
      >>> sc.fold(v, dim='x', sizes={'y': 2, 'z': 3}).values
      array([[0, 1, 2],
             [3, 4, 5]])

      >>> sc.fold(v, dim='x', dims=['y', 'z'], shape=[2, 3])
      <scipp.Variable> (y: 2, z: 3)      int64  [dimensionless]  [0, 1, ..., 4, 5]

      >>> sc.fold(v, dim='x', sizes={'y': 2, 'z': -1})
      <scipp.Variable> (y: 2, z: 3)      int64  [dimensionless]  [0, 1, ..., 4, 5]

      >>> a = sc.DataArray(0.1 * sc.arange('x', 6), coords={'x': sc.arange('x', 6)})
      >>> sc.fold(a, dim='x', sizes={'y': 2, 'z': 3})
      <scipp.DataArray>
      Dimensions: Sizes[y:2, z:3, ]
      Coordinates:
        x                           int64  [dimensionless]  (y, z)  [0, 1, ..., 4, 5]
      Data:
                                  float64  [dimensionless]  (y, z)  [0, 0.1, ..., 0.4, 0.5]
      >>> sc.fold(a, dim='x', sizes={'y': 2, 'z': 3}).data.values
      array([[0. , 0.1, 0.2],
             [0.3, 0.4, 0.5]])
      >>> sc.fold(a, dim='x', sizes={'y': 2, 'z': 3}).coords['x'].values
      array([[0, 1, 2],
             [3, 4, 5]])
    """
    if sizes is not None:
        if (dims is not None) or (shape is not None):
            raise RuntimeError(
                "If sizes is defined, dims and shape must be None in fold.")
        sizes = sizes.copy()
    else:
        if (dims is None) or (shape is None):
            raise RuntimeError("Both dims and shape must be defined in fold.")
        sizes = dict(zip(dims, shape))

    # Handle potential size of -1.
    # Note that we implement this here on the Python layer, because one cannot create
    # a C++ Dimensions object with negative sizes.
    new_shape = list(sizes.values())
    minus_one_count = new_shape.count(-1)
    if minus_one_count > 1:
        raise _cpp.DimensionError(
            "Can only have a single -1 in the new requested shape.")
    if minus_one_count == 1:
        ind = new_shape.index(-1)
        del new_shape[ind]
        new_volume = np.prod(new_shape)
        dim_size = x.sizes[dim] // new_volume
        if x.sizes[dim] % new_volume != 0:
            raise ValueError("-1 in new shape was computed to be {}, but the original "
                             "shape {} cannot be divided by {}.".format(
                                 dim_size, x.sizes[dim], dim_size))
        sizes[list(sizes.keys())[ind]] = dim_size

    return _call_cpp_func(_cpp.fold, x, dim, sizes)


def flatten(x: VariableLike,
            dims: Optional[Union[List[str], Tuple[str, ...]]] = None,
            to: Optional[str] = None) -> VariableLike:
    """Flatten multiple dimensions of a variable or data array into a single
    dimension. If dims is omitted, then we flatten all of the inputs dimensions
    into a single dim.

    :param x: Variable or DataArray to flatten.
    :param dims: A list of dim labels that will be flattened.
    :param to: A single dim label for the resulting flattened dim.
    :raises: If the bin edge coordinates cannot be stitched back together.
    :return: Variable or DataArray with requested dimension labels and shape.

    Examples:

      >>> v = sc.array(dims=['x', 'y'], values=np.arange(6).reshape(2, 3))
      >>> v
      <scipp.Variable> (x: 2, y: 3)      int64  [dimensionless]  [0, 1, ..., 4, 5]
      >>> sc.flatten(v, to='u')
      <scipp.Variable> (u: 6)      int64  [dimensionless]  [0, 1, ..., 4, 5]
      >>> sc.flatten(v, dims=['x', 'y'], to='u')
      <scipp.Variable> (u: 6)      int64  [dimensionless]  [0, 1, ..., 4, 5]

      >>> v = sc.array(dims=['x', 'y', 'z'], values=np.arange(24).reshape(2, 3, 4))
      >>> v
      <scipp.Variable> (x: 2, y: 3, z: 4)      int64  [dimensionless]  [0, 1, ..., 22, 23]
      >>> sc.flatten(v, to='u')
      <scipp.Variable> (u: 24)      int64  [dimensionless]  [0, 1, ..., 22, 23]
      >>> sc.flatten(v, dims=['x', 'y'], to='u')
      <scipp.Variable> (u: 6, z: 4)      int64  [dimensionless]  [0, 1, ..., 22, 23]
      >>> sc.flatten(v, dims=['y', 'z'], to='u')
      <scipp.Variable> (x: 2, u: 12)      int64  [dimensionless]  [0, 1, ..., 22, 23]

      >>> a = sc.DataArray(0.1 * sc.array(dims=['x', 'y'], values=np.arange(6).reshape(2, 3)),
      ...        coords={'x': sc.arange('x', 2),
      ...                'y': sc.arange('y', 3),
      ...                'xy': sc.array(dims=['x', 'y'],
      ...                               values=np.arange(6).reshape(2, 3))})
      >>> a
      <scipp.DataArray>
      Dimensions: Sizes[x:2, y:3, ]
      Coordinates:
        x                           int64  [dimensionless]  (x)  [0, 1]
        xy                          int64  [dimensionless]  (x, y)  [0, 1, ..., 4, 5]
        y                           int64  [dimensionless]  (y)  [0, 1, 2]
      Data:
                                  float64  [dimensionless]  (x, y)  [0, 0.1, ..., 0.4, 0.5]
      >>> sc.flatten(a, to='u')
      <scipp.DataArray>
      Dimensions: Sizes[u:6, ]
      Coordinates:
        x                           int64  [dimensionless]  (u)  [0, 0, ..., 1, 1]
        xy                          int64  [dimensionless]  (u)  [0, 1, ..., 4, 5]
        y                           int64  [dimensionless]  (u)  [0, 1, ..., 1, 2]
      Data:
                                  float64  [dimensionless]  (u)  [0, 0.1, ..., 0.4, 0.5]

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


def transpose(x: VariableLike,
              dims: Optional[Union[List[str], Tuple[str, ...]]] = None) -> VariableLike:
    """Transpose dimensions of a variable, a data array, or a dataset.

    :param x: Object to transpose.
    :param dims: List of dimensions in desired order. If default,
                 reverses existing order.
    :raises: If the dtype or unit does not match, or if the
             dimensions and shapes are incompatible.
    :return: The absolute values of the input.
    """
    return _call_cpp_func(_cpp.transpose, x, dims if dims is not None else [])


def squeeze(
        x: VariableLike,
        dim: Optional[Union[str, List[str], Tuple[str, ...]]] = None) -> VariableLike:
    """Remove dimensions of length 1.

    This is equivalent to indexing the squeezed dimensions with index 0, that is
    ``squeeze(x, ['x', 'y'])`` is equivalent to ``x['x', 0]['y', 0]``.

    :param x: Object to remove dimensions from.
    :param dim: If given, the dimension(s) to squeeze.
                If ``None``, all length-1 dimensions are squeezed.
    :raises: If a dimension in `dim` does not have length 1.
    :return: `x` with dimensions squeezed out.

    :seealso: :py:func:`scipp.Variable.squeeze`
              :py:func:`scipp.DataArray.squeeze`
              :py:func:`scipp.Dataset.squeeze`
              :py:func:`numpy.squeeze`

    Examples:

      >>> v = sc.arange('a', 3).fold('a', {'x': 1, 'y': 3, 'z': 1})
      >>> v
      <scipp.Variable> (x: 1, y: 3, z: 1)      int64  [dimensionless]  [0, 1, 2]
      >>> sc.squeeze(v)
      <scipp.Variable> (y: 3)      int64  [dimensionless]  [0, 1, 2]
      >>> sc.squeeze(v, 'z')
      <scipp.Variable> (x: 1, y: 3)      int64  [dimensionless]  [0, 1, 2]
      >>> sc.squeeze(v, ['x', 'z'])
      <scipp.Variable> (y: 3)      int64  [dimensionless]  [0, 1, 2]

    Coordinates for squeezed dimensions are turned into attributes:

      >>> da = sc.DataArray(v, coords={'x': sc.arange('x', 1),
      ...                              'y': sc.arange('y', 3)})
      >>> da
      <scipp.DataArray>
      Dimensions: Sizes[x:1, y:3, z:1, ]
      Coordinates:
        x                           int64  [dimensionless]  (x)  [0]
        y                           int64  [dimensionless]  (y)  [0, 1, 2]
      Data:
                                    int64  [dimensionless]  (x, y, z)  [0, 1, 2]
      >>> sc.squeeze(da)
      <scipp.DataArray>
      Dimensions: Sizes[y:3, ]
      Coordinates:
        y                           int64  [dimensionless]  (y)  [0, 1, 2]
      Data:
                                    int64  [dimensionless]  (y)  [0, 1, 2]
      Attributes:
        x                           int64  [dimensionless]  ()  [0]
    """
    return _call_cpp_func(_cpp.squeeze, x, (dim, ) if isinstance(dim, str) else dim)
