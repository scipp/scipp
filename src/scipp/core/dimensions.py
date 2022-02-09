# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from .._scipp.core import Variable, DataArray, Dataset, DimensionError


def _make_sizes(obj):
    """
    Makes a dictionary of dimensions labels to dimension sizes
    """
    return dict(zip(obj.dims, obj.shape))


def _make_renaming_dict(new_name_or_name_dict: dict = None, **names) -> dict:
    """
    Make a dict of renaming dimensions, using either the dict supplied or generating
    one from the keyword arguments.

    Note that this implementation fails in the unlikely case that one of the dims is
    named `new_name_or_name_dict`. An implementation that would capture this would be
    more complicated/ugly.
    """
    if new_name_or_name_dict is not None:
        if not isinstance(new_name_or_name_dict, dict):
            raise TypeError('A dict mapping old dimension names to new ones must be '
                            'supplied. Got a {}.'.format(type(new_name_or_name_dict)))
        return new_name_or_name_dict
    else:
        return {**names}


def _rename_variable(var: Variable,
                     new_name_or_name_dict: dict = None,
                     **names) -> Variable:
    """
    Rename the dimensions labels of a Variable.
    The renaming can be defined:
       - using a dict mapping the old to new names, e.g. rename({'x': 'a', 'y': 'b'})
       - using keyword arguments, e.g. rename(x='a', y='b')
    """
    return var.rename_dims(_make_renaming_dict(new_name_or_name_dict, **names))


def _rename_data_array(da: DataArray,
                       new_name_or_name_dict: dict = None,
                       **names) -> DataArray:
    """
    Rename the dimensions, coordinates and attributes of a Dataset.
    The renaming can be defined:
       - using a dict mapping the old to new names, e.g. rename({'x': 'a', 'y': 'b'})
       - using keyword arguments, e.g. rename(x='a', y='b')
    """
    renaming_dict = _make_renaming_dict(new_name_or_name_dict, **names)
    out = da.rename_dims(renaming_dict)
    for meta in (out.coords, out.attrs):
        for coord in meta:
            if coord in renaming_dict:
                if renaming_dict[coord] in meta:
                    raise DimensionError(
                        'Cannot rename coordinate {} to {} as this would erase an '
                        'existing coordinate.'.format(coord, renaming_dict[coord]))
                meta[renaming_dict[coord]] = meta.pop(coord)
    return out


def _rename_dataset(ds: Dataset,
                    new_name_or_name_dict: dict = None,
                    **names) -> Dataset:
    """
    Rename the dimensions, coordinates and attributes of all the items in a dataset.
    The renaming can be defined:
       - using a dict mapping the old to new names, e.g. rename({'x': 'a', 'y': 'b'})
       - using keyword arguments, e.g. rename(x='a', y='b')
    """
    return Dataset(data={
        key: _rename_data_array(ds[key], new_name_or_name_dict=None, **names)
        for key in ds
    })
