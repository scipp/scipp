# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from .._scipp.core import Variable, DataArray, Dataset, DimensionError


def _make_sizes(obj):
    """
    Makes a dictionary of dimensions labels to dimension sizes
    """
    return dict(zip(obj.dims, obj.shape))


def _make_renaming_dict(dims_dict: dict = None, **names) -> dict:
    """
    Make a dict of renaming dimensions, using either the dict supplied or generating
    one from the keyword arguments.

    Note that this implementation fails in the unlikely case that one of the dims is
    named `dims_dict`. An implementation that would capture this would be
    more complicated/ugly.
    """
    out = {**names}
    if dims_dict is not None:
        if isinstance(dims_dict, dict):
            return dims_dict
        elif isinstance(dims_dict, str):
            # This is the unlikely case that `dims_dict` is one of the old dimensions
            out['dims_dict'] = dims_dict
        else:
            raise TypeError('A dict mapping old dimension names to new ones must be '
                            'supplied. Got a {}.'.format(type(dims_dict)))
    return out


def _rename_variable(var: Variable, dims_dict: dict = None, **names) -> Variable:
    """
    Rename the dimensions labels of a Variable.
    The renaming can be defined:
       - using a dict mapping the old to new names, e.g. rename({'x': 'a', 'y': 'b'})
       - using keyword arguments, e.g. rename(x='a', y='b')
    """
    return var.rename_dims(_make_renaming_dict(dims_dict, **names))


def _rename_data_array(da: DataArray, dims_dict: dict = None, **names) -> DataArray:
    """
    Rename the dimensions, coordinates and attributes of a Dataset.
    The renaming can be defined:
       - using a dict mapping the old to new names, e.g. rename({'x': 'a', 'y': 'b'})
       - using keyword arguments, e.g. rename(x='a', y='b')
    """
    renaming_dict = _make_renaming_dict(dims_dict, **names)
    out = da.rename_dims(renaming_dict)
    for coord in renaming_dict:
        for meta in (out.coords, out.attrs):
            if coord in meta:
                if (renaming_dict[coord] in meta) and (renaming_dict[coord]
                                                       not in renaming_dict):
                    raise DimensionError(
                        'Cannot rename coordinate {} to {} as this would erase an '
                        'existing coordinate.'.format(coord, renaming_dict[coord]))
                meta[renaming_dict[coord]] = meta.pop(coord)
    return out


def _rename_dataset(ds: Dataset, dims_dict: dict = None, **names) -> Dataset:
    """
    Rename the dimensions, coordinates and attributes of all the items in a dataset.
    The renaming can be defined:
       - using a dict mapping the old to new names, e.g. rename({'x': 'a', 'y': 'b'})
       - using keyword arguments, e.g. rename(x='a', y='b')
    """
    renaming_dict = _make_renaming_dict(dims_dict, **names)
    out = Dataset()
    for key, item in ds.items():
        if set(renaming_dict.keys()).issubset(set(item.dims)):
            out[key] = _rename_data_array(ds[key], dims_dict=renaming_dict)
        else:
            out[key] = item
    return out
