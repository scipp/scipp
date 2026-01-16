# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Matthew Andrew
# ruff: noqa: E501

from collections.abc import Sequence
from typing import overload

import numpy as np

from .._scipp import core as _cpp
from ..typing import VariableLikeType
from . import data_group
from ._cpp_wrapper_util import call_func as _call_cpp_func
from ._sizes import _parse_dims_shape_sizes
from .concepts import transform_data
from .cpp_classes import Variable


@overload
def broadcast(
    x: VariableLikeType,
    *,
    dims: Sequence[str],
    shape: Sequence[int],
) -> VariableLikeType: ...


@overload
def broadcast(
    x: VariableLikeType,
    *,
    sizes: dict[str, int],
) -> VariableLikeType: ...


def broadcast(
    x: VariableLikeType,
    *,
    dims: Sequence[str] | None = None,
    shape: Sequence[int] | None = None,
    sizes: dict[str, int] | None = None,
) -> VariableLikeType:
    """Broadcast a Variable or a DataArray.

    If the input is a DataArray, coordinates and attributes are shallow-copied
    and masks are deep-copied.

    Note that Scipp operations broadcast automatically, so using this function
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

    Examples
    --------
    Broadcast a scalar to an array:

      >>> import scipp as sc
      >>> x = sc.scalar(5, unit='m')
      >>> sc.broadcast(x, sizes={'x': 3})
      <scipp.Variable> (x: 3)      int64              [m]  [5, 5, 5]

    Broadcast a 1-D array to 2-D:

      >>> x = sc.array(dims=['x'], values=[1, 2, 3], unit='m')
      >>> result = sc.broadcast(x, sizes={'x': 3, 'y': 2})
      >>> result
      <scipp.Variable> (x: 3, y: 2)      int64              [m]  [1, 1, ..., 3, 3]
      >>> result.values
      array([[1, 1],
             [2, 2],
             [3, 3]])

    Using ``dims`` and ``shape`` instead of ``sizes``:

      >>> sc.broadcast(x, dims=['x', 'y'], shape=[3, 2])
      <scipp.Variable> (x: 3, y: 2)      int64              [m]  [1, 1, ..., 3, 3]

    Broadcasting fails if the target size conflicts with an existing dimension:

      >>> x = sc.array(dims=['x'], values=[1, 2, 3])
      >>> sc.broadcast(x, sizes={'x': 5})  # doctest: +IGNORE_EXCEPTION_DETAIL
      Traceback (most recent call last):
          ...
      scipp.core.DimensionError: Cannot broadcast existing dimension 'x' ...
    """
    dims_and_shape = _parse_dims_shape_sizes(dims=dims, shape=shape, sizes=sizes)
    dims = dims_and_shape["dims"]
    shape = dims_and_shape["shape"]

    def _broadcast(data: Variable) -> Variable:
        return _call_cpp_func(_cpp.broadcast, data, dims, shape)  # type: ignore[return-value]

    return transform_data(x, _broadcast)


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
      * x                           int64  [dimensionless]  (x)  [0, 1, ..., 100, 200]
      Data:
                                    int64  [dimensionless]  (x)  [0, 1, ..., 100, 200]
      >>> z.values
      array([  0,   1,   2,   0, 100, 200])
    """
    if x and isinstance(x[0], data_group.DataGroup):
        return data_group.apply_to_items(
            concat,
            x,
            dim,
        )
    return _call_cpp_func(_cpp.concat, x, dim)  # type: ignore[return-value]


@overload
def fold(
    x: VariableLikeType,
    dim: str,
    *,
    dims: Sequence[str],
    shape: Sequence[int],
) -> VariableLikeType: ...


@overload
def fold(
    x: VariableLikeType,
    dim: str,
    *,
    sizes: dict[str, int],
) -> VariableLikeType: ...


def fold(
    x: VariableLikeType,
    dim: str,
    *,
    dims: Sequence[str] | None = None,
    shape: Sequence[int] | None = None,
    sizes: dict[str, int] | None = None,
) -> VariableLikeType:
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
      * x                           int64  [dimensionless]  (y, z)  [0, 1, ..., 4, 5]
      Data:
                                  float64  [dimensionless]  (y, z)  [0, 0.1, ..., 0.4, 0.5]
      >>> sc.fold(a, dim='x', sizes={'y': 2, 'z': 3}).data.values
      array([[0. , 0.1, 0.2],
             [0.3, 0.4, 0.5]])
      >>> sc.fold(a, dim='x', sizes={'y': 2, 'z': 3}).coords['x'].values
      array([[0, 1, 2],
             [3, 4, 5]])
    """
    dims_and_shape = _parse_dims_shape_sizes(dims=dims, shape=shape, sizes=sizes)
    dims = dims_and_shape["dims"]
    new_shape = list(dims_and_shape["shape"])

    # Handle potential size of -1.
    # Note that we implement this here on the Python layer, because one cannot create
    # a C++ Dimensions object with negative sizes.
    minus_one_count = new_shape.count(-1)
    if minus_one_count > 1:
        raise _cpp.DimensionError(
            "Can only have a single -1 in the new requested shape."
        )
    if minus_one_count == 1:
        if (size := x.sizes[dim]) is None:
            raise ValueError(
                f"Dim {dim} has inconsistent size, cannot compute final shape."
            )
        ind = new_shape.index(-1)
        new_shape[ind] = 1
        new_volume = np.prod(new_shape)
        dim_size = int(size // new_volume)
        if size % new_volume != 0:
            raise ValueError(
                f"-1 in new shape was computed to be {dim_size}, but the original "
                f"shape {size} cannot be divided by {dim_size}."
            )
        new_shape[ind] = dim_size

    return _call_cpp_func(_cpp.fold, x, dim, dims, new_shape)  # type: ignore[return-value]


def flatten(
    x: VariableLikeType,
    dims: Sequence[str] | None = None,
    to: str | None = None,
) -> VariableLikeType:
    """Flatten multiple dimensions into a single dimension.

    If the input has a bin-edge coordinate that cannot be joined together it will not
    be included in the output.

    If the input is a DataArray then coords and masks that contain at least one
    of the flattened dimensions will also be flattened. This implies that when
    flattening all dims, i.e., when ``dims=None``, all coords and masks that
    share *some or all* dimensions with the data will be flattened.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Multi-dimensional input to flatten.
    dims:
        A list of dim labels that will be flattened.
        If ``None``, all dimensions will be flattened.
        If the list is empty, this will effectively add a new inner dimension of length
        1 to the data (meta data such as coords and masks are not touched in this case).
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
      * x                           int64  [dimensionless]  (x)  [0, 1]
      * xy                          int64  [dimensionless]  (x, y)  [0, 1, ..., 4, 5]
      * y                           int64  [dimensionless]  (y)  [0, 1, 2]
      Data:
                                  float64  [dimensionless]  (x, y)  [0, 0.1, ..., 0.4, 0.5]
      >>> sc.flatten(a, to='u')
      <scipp.DataArray>
      Dimensions: Sizes[u:6, ]
      Coordinates:
      * x                           int64  [dimensionless]  (u)  [0, 0, ..., 1, 1]
      * xy                          int64  [dimensionless]  (u)  [0, 1, ..., 4, 5]
      * y                           int64  [dimensionless]  (u)  [0, 1, ..., 1, 2]
      Data:
                                  float64  [dimensionless]  (u)  [0, 0.1, ..., 0.4, 0.5]

    """
    if to is None:
        # Note that this is a result of the fact that we want to support
        # calling flatten without kwargs, and that in this case it semantically
        # makes more sense for the dims that we want to flatten to come first
        # in the argument list.
        raise ValueError("The final flattened dimension is required.")
    return _call_cpp_func(_cpp.flatten, x, dims, to)  # type: ignore[return-value]


def transpose(
    x: VariableLikeType, dims: Sequence[str] | None = None
) -> VariableLikeType:
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

    Examples
    --------
    Reverse the dimensions of a 2-D array:

      >>> import scipp as sc
      >>> data = sc.array(dims=['x', 'y'], values=[[1, 2, 3], [4, 5, 6]])
      >>> data
      <scipp.Variable> (x: 2, y: 3)      int64  [dimensionless]  [1, 2, ..., 5, 6]
      >>> data.values
      array([[1, 2, 3],
             [4, 5, 6]])
      >>> sc.transpose(data)
      <scipp.Variable> (y: 3, x: 2)      int64  [dimensionless]  [1, 4, ..., 3, 6]
      >>> sc.transpose(data).values
      array([[1, 4],
             [2, 5],
             [3, 6]])

    Specify the order of dimensions explicitly:

      >>> data3d = sc.array(dims=['x', 'y', 'z'], values=sc.arange('t', 24).values.reshape(2, 3, 4))
      >>> data3d.sizes
      {'x': 2, 'y': 3, 'z': 4}
      >>> sc.transpose(data3d, dims=['z', 'x', 'y']).sizes
      {'z': 4, 'x': 2, 'y': 3}

    With a DataArray, coordinates are preserved:

      >>> da = sc.DataArray(
      ...     data=sc.array(dims=['x', 'y'], values=[[1, 2], [3, 4]], unit='K'),
      ...     coords={
      ...         'x': sc.array(dims=['x'], values=[0.0, 1.0], unit='m'),
      ...         'y': sc.array(dims=['y'], values=[10, 20], unit='s')
      ...     }
      ... )
      >>> sc.transpose(da)
      <scipp.DataArray>
      Dimensions: Sizes[y:2, x:2, ]
      Coordinates:
      * x                         float64              [m]  (x)  [0, 1]
      * y                           int64              [s]  (y)  [10, 20]
      Data:
                                    int64              [K]  (y, x)  [1, 3, 2, 4]
    """
    return _call_cpp_func(_cpp.transpose, x, dims if dims is not None else [])  # type: ignore[return-value]


def squeeze(
    x: VariableLikeType, dim: str | Sequence[str] | None = None
) -> VariableLikeType:
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

      >>> v = sc.arange('a', 3).fold('a', sizes={'x': 1, 'y': 3, 'z': 1})
      >>> v
      <scipp.Variable> (x: 1, y: 3, z: 1)      int64  [dimensionless]  [0, 1, 2]
      >>> sc.squeeze(v)
      <scipp.Variable> (y: 3)      int64  [dimensionless]  [0, 1, 2]
      >>> sc.squeeze(v, 'z')
      <scipp.Variable> (x: 1, y: 3)      int64  [dimensionless]  [0, 1, 2]
      >>> sc.squeeze(v, ['x', 'z'])
      <scipp.Variable> (y: 3)      int64  [dimensionless]  [0, 1, 2]

    Coordinates for squeezed dimensions become unaligned:

      >>> da = sc.DataArray(v, coords={'x': sc.arange('x', 1),
      ...                              'y': sc.arange('y', 3)})
      >>> da
      <scipp.DataArray>
      Dimensions: Sizes[x:1, y:3, z:1, ]
      Coordinates:
      * x                           int64  [dimensionless]  (x)  [0]
      * y                           int64  [dimensionless]  (y)  [0, 1, 2]
      Data:
                                    int64  [dimensionless]  (x, y, z)  [0, 1, 2]
      >>> sc.squeeze(da)
      <scipp.DataArray>
      Dimensions: Sizes[y:3, ]
      Coordinates:
        x                           int64  [dimensionless]  ()  0
      * y                           int64  [dimensionless]  (y)  [0, 1, 2]
      Data:
                                    int64  [dimensionless]  (y)  [0, 1, 2]
    """
    return _call_cpp_func(_cpp.squeeze, x, (dim,) if isinstance(dim, str) else dim)  # type: ignore[return-value]
