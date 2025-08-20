# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock

import uuid
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

    The returned label is intended for temporarily reshaping an array and should
    not become visible to users.
    The label is guaranteed to not be present in the input, but it may be used in
    other variables.
    """
    used = {*(x.dims for x in data)}
    for dim in _USED_AUX_DIMS:
        if dim not in used:
            return dim
    dim = uuid.uuid4().hex
    _USED_AUX_DIMS.append(dim)
    return dim
