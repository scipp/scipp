# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock

import uuid
from itertools import chain
from typing import TypeVar

from .._scipp.core import CoordError, DataArray, Dataset, Variable
from .argument_handlers import combine_dict_args
from .variable import scalar

_T = TypeVar('_T', Variable, DataArray, Dataset)


def _rename_dims(
    self: _T, dims_dict: dict[str, str] | None = None, /, **names: str
) -> _T:
    """Rename dimensions.

    The renaming can be defined:

    - using a dict mapping the old to new names, e.g.
      ``rename_dims({'x': 'a', 'y': 'b'})``
    - using keyword arguments, e.g. ``rename_dims(x='a', y='b')``

    In both cases, x is renamed to a and y to b.

    Dimensions not specified in either input are unchanged.

    This function only renames dimensions.
    See the ``rename`` method to also rename coordinates and attributes.

    Parameters
    ----------
    dims_dict:
        Dictionary mapping old to new names.
    names:
        Mapping of old to new names as keyword arguments.

    Returns
    -------
    :
        A new object with renamed dimensions.
    """
    return self._rename_dims(combine_dict_args(dims_dict, names))


def _rename_variable(
    var: Variable, dims_dict: dict[str, str] | None = None, /, **names: str
) -> Variable:
    """Rename dimension labels.

    The renaming can be defined:

    - using a dict mapping the old to new names, e.g. ``rename({'x': 'a', 'y': 'b'})``
    - using keyword arguments, e.g. ``rename(x='a', y='b')``

    In both cases, x is renamed to a and y to b.

    Dimensions not specified in either input are unchanged.

    Parameters
    ----------
    dims_dict:
        Dictionary mapping old to new names.
    names:
        Mapping of old to new names as keyword arguments.

    Returns
    -------
    :
        A new variable with renamed dimensions which shares a buffer with the input.

    See Also
    --------
    scipp.Variable.rename_dims:
        Equivalent for ``Variable`` but differs for ``DataArray`` and ``Dataset``.
    """
    return var.rename_dims(combine_dict_args(dims_dict, names))


def _rename_data_array(
    da: DataArray, dims_dict: dict[str, str] | None = None, /, **names: str
) -> DataArray:
    """Rename the dimensions, coordinates, and attributes.

    The renaming can be defined:

    - using a dict mapping the old to new names, e.g. ``rename({'x': 'a', 'y': 'b'})``
    - using keyword arguments, e.g. ``rename(x='a', y='b')``

    In both cases, x is renamed to a and y to b.

    Names not specified in either input are unchanged.

    Parameters
    ----------
    dims_dict:
        Dictionary mapping old to new names.
    names:
        Mapping of old to new names as keyword arguments.

    Returns
    -------
    :
        A new data array with renamed dimensions, coordinates, and attributes.
        Buffers are shared with the input.

    See Also
    --------
    scipp.DataArray.rename_dims:
        Only rename dimensions, not coordinates and attributes.
    """
    renaming_dict = combine_dict_args(dims_dict, names)
    out = da.rename_dims(renaming_dict)
    if out.bins is not None:
        from .bins import bins

        out.data = bins(**out.bins.constituents)
    for old, new in renaming_dict.items():
        if new in out.coords:
            raise CoordError(
                f"Cannot rename '{old}' to '{new}', since a coord named {new} "
                "already exists."
            )
        if old in out.coords:
            out.coords[new] = out.coords.pop(old)
        if out.bins is not None:
            if old in out.bins.coords:
                out.bins.coords[new] = out.bins.coords.pop(old)
    return out


def _rename_dataset(
    ds: Dataset, dims_dict: dict[str, str] | None = None, /, **names: str
) -> Dataset:
    """Rename the dimensions, coordinates and attributes of all the items.

    The renaming can be defined:

    - using a dict mapping the old to new names, e.g. ``rename({'x': 'a', 'y': 'b'})``
    - using keyword arguments, e.g. ``rename(x='a', y='b')``

    In both cases, x is renamed to a and y to b.

    Names not specified in either input are unchanged.

    Parameters
    ----------
    dims_dict:
        Dictionary mapping old to new names.
    names:
        Mapping of old to new names as keyword arguments.

    Returns
    -------
    :
        A new dataset with renamed dimensions, coordinates, and attributes.
        Buffers are shared with the input.

    See Also
    --------
    scipp.Dataset.rename_dims:
        Only rename dimensions, not coordinates and attributes.
    """
    dims_dict = combine_dict_args(dims_dict, names)
    if len(ds) != 0:
        return Dataset(
            {key: _rename_data_array(value, dims_dict) for key, value in ds.items()}
        )
    # This relies on broadcast and DataArray.__init__ not making copies
    # to avoid allocating too much extra memory.
    dummy = DataArray(
        scalar(0).broadcast(dims=ds.dims, shape=ds.shape), coords=ds.coords
    )
    return Dataset(coords=_rename_data_array(dummy, dims_dict).coords)


_USED_AUX_DIMS: list[str] = []


def new_dim_for(*data: Variable | DataArray) -> str:
    """Return a dimension label that is not in the input's dimensions.

    Sometimes, it is useful to add temporary dimensions to arrays in intermediate
    operations. For example, to stack arrays in a new dimension and then reduce
    over that new dimension. A simple, but bad solution is to generate a new
    dimension label every time. E.g.,

    >>> import uuid
    >>> # Make some fake data:
    >>> arrays = [sc.arange('x', 4), 2 * sc.arange('x', 4)]
    >>> dim = uuid.uuid4().hex
    >>> stacked = sc.concat(arrays, dim=dim)
    >>> reduced = sc.sum(stacked, dim=dim)

    However, Scipp encodes dimension labels as 16-bit integers internally.
    This means that there can be a maximum of ``2**16`` different dimension labels.
    Further, labels are not freed up after use.
    So it is better to not generate many temporary labels but instead reuse labels
    as much as possible.
    ``new_dim_for`` does exactly that.
    Similarly to the code above, it generates dimension labels using ``uuid4``.
    However, it reuses generated labels whenever possible and so is very unlikely
    to hit the ``2**16`` limit.

    So a better solution to the code above is:

    >>> # Make some fake data:
    >>> arrays = [sc.arange('x', 4), 2 * sc.arange('x', 4)]
    >>> dim = sc.new_dim_for(*arrays)
    >>> stacked = sc.concat(arrays, dim=dim)
    >>> reduced = sc.sum(stacked, dim=dim)

    Parameters
    ----------
    data:
        A number of variables or data arrays.

    Returns
    -------
    :
        A dimension label that is not in any variable or data array in ``data``.
    """
    used: set[str] = set(chain(*(x.dims for x in data)))
    for dim in _USED_AUX_DIMS:
        if dim not in used:
            return dim
    dim = uuid.uuid4().hex
    _USED_AUX_DIMS.append(dim)
    return dim
