# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock

from typing import TypeVar

from .._scipp.core import CoordError, DataArray, Dataset, Variable
from .argument_handlers import IntoStrDict, combine_dict_args
from .bins import bins
from .variable import scalar

_T = TypeVar('_T', Variable, DataArray, Dataset)


def _rename_dims(
    self: _T, dims_dict: IntoStrDict[str] | None = None, /, **names: str
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
        Dictionary or items iterator mapping old to new names.
    names:
        Mapping of old to new names as keyword arguments.

    Returns
    -------
    :
        A new object with renamed dimensions.

    Examples
    --------

      >>> import scipp as sc
      >>> var = sc.array(dims=['x', 'y'], values=[[1, 2], [3, 4]])
      >>> var.rename_dims({'x': 'row', 'y': 'col'}).sizes
      {'row': 2, 'col': 2}

    Using keyword arguments:

      >>> var.rename_dims(x='a', y='b').dims
      ('a', 'b')

    Only specified dimensions are renamed:

      >>> var.rename_dims(x='i').dims
      ('i', 'y')
    """
    return self._rename_dims(combine_dict_args(dims_dict, names))


def _rename_variable(
    var: Variable, dims_dict: IntoStrDict[str] | None = None, /, **names: str
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
        Dictionary or items iterator mapping old to new names.
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

    Examples
    --------
    For Variables, ``rename`` is equivalent to ``rename_dims``:

      >>> import scipp as sc
      >>> var = sc.array(dims=['x'], values=[1, 2, 3], unit='m')
      >>> var.rename(x='distance')
      <scipp.Variable> (distance: 3)      int64              [m]  [1, 2, 3]

      >>> var.rename({'x': 'position'})
      <scipp.Variable> (position: 3)      int64              [m]  [1, 2, 3]
    """
    return var.rename_dims(combine_dict_args(dims_dict, names))


def _rename_data_array(
    da: DataArray, dims_dict: IntoStrDict[str] | None = None, /, **names: str
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
        Dictionary or items iterator mapping old to new names.
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
    if out.is_binned:
        out.data = bins(**out.bins.constituents)
    for old, new in renaming_dict.items():
        if new in out.coords:
            raise CoordError(
                f"Cannot rename '{old}' to '{new}', since a coord named {new} "
                "already exists."
            )
        if old in out.coords:
            out.coords[new] = out.coords.pop(old)
        if out.is_binned:
            if old in out.bins.coords:
                out.bins.coords[new] = out.bins.coords.pop(old)
    return out


def _rename_dataset(
    ds: Dataset, dims_dict: IntoStrDict[str] | None = None, /, **names: str
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
        Dictionary or items iterator mapping old to new names.
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
