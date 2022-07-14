# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Matthew Andrew
# flake8: noqa: E501

import numpy as np
from typing import Dict, List, Optional, Sequence, Tuple, Union

from .._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func
from ._sizes import _parse_dims_shape_sizes
from ..typing import VariableLikeType


def broadcast(
    x: VariableLikeType,
    dims: Optional[Union[List[str], Tuple[str, ...]]] = None,
    shape: Optional[Sequence[int]] = None,
    sizes: Optional[Dict[str, int]] = None,
) -> VariableLikeType:
    """Broadcast a Variable or a DataArray.

    If the input is a DataArray, coordinates and attributes are shallow-copied
    and masks are deep-copied.

    Note that scipp operations broadcast automatically, so using this function
    directly is rarely required.

    One and only one of these sets of arguments must be given:

    - ``dims`` and ``shape``
    - ``sizes``

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data to broadcast.
    dims:
        List of new dimensions.
    shape:
        New extents in each dimension.
    sizes:
        New dimension labels to sizes map.

    Returns
    -------
    : Same type as input
        New Variable or DataArray with requested dimension labels and shape.
    """
    sizes = _parse_dims_shape_sizes(dims=dims, shape=shape, sizes=sizes)
    if isinstance(x, _cpp.Variable):
        return _call_cpp_func(_cpp.broadcast, x, sizes["dims"], sizes["shape"])
    elif isinstance(x, _cpp.DataArray):
        return _cpp.DataArray(data=_call_cpp_func(_cpp.broadcast, x.data, sizes["dims"],
                                                  sizes["shape"]),
                              coords={c: coord
                                      for c, coord in x.coords.items()},
                              attrs={a: attr
                                     for a, attr in x.attrs.items()},
                              masks={m: mask.copy()
                                     for m, mask in x.masks.items()})
    else:
        raise TypeError("Broadcast only supports Variable and DataArray as inputs.")


def concat(x: Sequence[VariableLikeType], dim: str) -> VariableLikeType:
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

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Sequence of input variables, data arrays, or datasets.
    dim:
        Dimension along which to concatenate.

    Returns
    -------
    : Same type as input
        Concatenation of the inputs.

    Examples
    --------

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


def fold(x: VariableLikeType,
         dim: str,
         sizes: Optional[Dict[str, int]] = None,
         dims: Optional[Union[List[str], Tuple[str, ...]]] = None,
         shape: Optional[Sequence[int]] = None) -> VariableLikeType:
    """Fold a single dimension of a variable or data array into multiple dims.

    One and only one of these sets of arguments must be given:

    - ``dims`` and ``shape``
    - ``sizes``

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Variable or DataArray to fold.
    dim:
        A single dim label that will be folded into more dims.
    sizes:
        A dict mapping new dims to new shapes.
    dims:
        A list of new dims labels.
    shape:
        A list of new dim shapes.

    Returns
    -------
    : Same type as input
        Variable or DataArray with requested dimension labels and shape.

    Raises
    ------
    scipp.DimensionError
        If the volume of the old shape is not equal to the
        volume of the new shape.

    Examples
    --------

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
    sizes = _parse_dims_shape_sizes(dims=dims, shape=shape, sizes=sizes)

    # Handle potential size of -1.
    # Note that we implement this here on the Python layer, because one cannot create
    # a C++ Dimensions object with negative sizes.
    new_shape = sizes["shape"]
    minus_one_count = new_shape.count(-1)
    if minus_one_count > 1:
        raise _cpp.DimensionError(
            "Can only have a single -1 in the new requested shape.")
    if minus_one_count == 1:
        ind = new_shape.index(-1)
        new_shape[ind] = 1
        new_volume = np.prod(new_shape)
        dim_size = x.sizes[dim] // new_volume
        if x.sizes[dim] % new_volume != 0:
            raise ValueError("-1 in new shape was computed to be {}, but the original "
                             "shape {} cannot be divided by {}.".format(
                                 dim_size, x.sizes[dim], dim_size))
        new_shape[ind] = dim_size

    return _call_cpp_func(_cpp.fold, x, dim, sizes["dims"], new_shape)


def flatten(x: VariableLikeType,
            dims: Optional[Union[List[str], Tuple[str, ...]]] = None,
            to: Optional[str] = None) -> VariableLikeType:
    """Flatten multiple dimensions into a single dimension.

    If the input has a bin-edge coordinate that cannot be joined together it will not
    be included in the output.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Multi-dimensional input to flatten.
    dims:
        A list of dim labels that will be flattened.
        If ``None``, all dimensions will be flattened.
    to:
        A single dim label for the resulting flattened dim.

    Returns
    -------
    : Same type as input
        Variable or DataArray with requested dimension labels and shape.

    Raises
    ------
    scipp.DimensionError
        If the input does not have a contiguous memory layout,
        i.e. flattening would require moving data around.
        This can be resolved by (deep-)copying the input.

    Examples
    --------

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
        raise ValueError("The final flattened dimension is required.")
    if dims is None:
        dims = x.dims
    return _call_cpp_func(_cpp.flatten, x, dims, to)


def transpose(
        x: VariableLikeType,
        dims: Optional[Union[List[str], Tuple[str, ...]]] = None) -> VariableLikeType:
    """Transpose dimensions of the input.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Object to transpose.
    dims:
        List of dimensions in desired order.
        If ``None``, reverses existing order.

    Returns
    -------
    : Same type as input
        The transpose of the input.

    Raises
    ------
    scipp.DimensionError
        If ``dims`` are incompatible with the input data.
    """
    return _call_cpp_func(_cpp.transpose, x, dims if dims is not None else [])


def squeeze(
        x: VariableLikeType,
        dim: Optional[Union[str, List[str], Tuple[str,
                                                  ...]]] = None) -> VariableLikeType:
    """Remove dimensions of length 1.

    This is equivalent to indexing the squeezed dimensions with index 0, that is
    ``squeeze(x, ['x', 'y'])`` is equivalent to ``x['x', 0]['y', 0]``.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Object to remove dimensions from.
    dim:
        If given, the dimension(s) to squeeze.
        If ``None``, all length-1 dimensions are squeezed.

    Returns
    -------
    : Same type as input
        Input with length-1 dimensions removed.

    Raises
    ------
    scipp.DimensionError
        If a dimension in ``dim`` does not have length 1.

    See Also
    --------
    scipp.Variable.squeeze, scipp.DataArray.squeeze,
    scipp.Dataset.squeeze, numpy.squeeze

    Examples
    --------

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
