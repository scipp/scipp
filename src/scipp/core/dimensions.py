# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from .._scipp.core import Variable, DataArray, Dataset, DimensionError


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
        for meta in (out.coords, out.attrs):
            if old in meta:
                if new in meta:
                    raise DimensionError(
                        f'Cannot rename coordinate {old} to {new} as this would erase '
                        'an existing coordinate.')
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
    out = Dataset()
    for key, item in ds.items():
        dims_intersect = set(renaming_dict.keys()).intersection(set(item.dims))
        if dims_intersect:
            out[key] = _rename_data_array(
                ds[key], dims_dict={dim: renaming_dict[dim]
                                    for dim in dims_intersect})
        else:
            out[key] = item
    return out
