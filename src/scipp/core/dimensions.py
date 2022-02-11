# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from .._scipp.core import Variable, DataArray, Dataset, CoordError
from .dataset import merge


def _make_sizes(obj):
    """
    Makes a dictionary of dimensions labels to dimension sizes
    """
    return dict(zip(obj.dims, obj.shape))


def _rename_variable(var: Variable, dims_dict: dict = None, **names) -> Variable:
    """
    Rename the dimensions labels of a Variable.
    The renaming can be defined:
       - using a dict mapping the old to new names, e.g. rename({'x': 'a', 'y': 'b'})
       - using keyword arguments, e.g. rename(x='a', y='b')
    """
    return var.rename_dims({**({} if dims_dict is None else dims_dict), **names})


def _rename_data_array(da: DataArray, dims_dict: dict = None, **names) -> DataArray:
    """
    Rename the dimensions, coordinates and attributes of a Dataset.
    The renaming can be defined:
       - using a dict mapping the old to new names, e.g. rename({'x': 'a', 'y': 'b'})
       - using keyword arguments, e.g. rename(x='a', y='b')
    """
    renaming_dict = {**({} if dims_dict is None else dims_dict), **names}
    out = da.rename_dims(renaming_dict)
    for old, new in renaming_dict.items():
        if new in out.meta:
            raise CoordError(
                f"Cannot rename '{old}' to '{new}', since a coord or attr named {new} "
                "already exists.")
        for meta in (out.coords, out.attrs):
            if old in meta:
                meta[new] = meta.pop(old)
    return out


def _rename_dataset(ds: Dataset, dims_dict: dict = None, **names) -> Dataset:
    """
    Rename the dimensions, coordinates and attributes of all the items in a dataset.
    The renaming can be defined:
       - using a dict mapping the old to new names, e.g. rename({'x': 'a', 'y': 'b'})
       - using keyword arguments, e.g. rename(x='a', y='b')
    """
    renaming_dict = {**({} if dims_dict is None else dims_dict), **names}
    ds_from_items = Dataset()
    for key, item in ds.items():
        dims_dict = {old: new for old, new in renaming_dict.items() if old in item.dims}
        ds_from_items[key] = _rename_data_array(item, dims_dict=dims_dict)
    dict_of_coords = {}
    for dim, coord in ds.coords.items():
        if dim not in ds_from_items.coords:
            dims_dict = {
                old: new
                for old, new in renaming_dict.items() if old in coord.dims
            }
            dict_of_coords[dims_dict.get(dim,
                                         dim)] = _rename_variable(coord,
                                                                  dims_dict=dims_dict)
    return merge(ds_from_items, Dataset(coords=dict_of_coords))
