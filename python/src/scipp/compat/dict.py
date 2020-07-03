# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .. import _utils as su
from .._scipp import core as sc


def to_dict(scipp_obj):

    if su.is_variable(scipp_obj):
        return _variable_to_dict(scipp_obj)
    elif su.is_data_array(scipp_obj):
        return _data_array_to_dict(scipp_obj)
    elif su.is_dataset(scipp_obj):
        # TODO: This currently duplicates all coordinates that would otherwise
        # be at the Dataset level onto the individual DataArrays. Since we are
        # only copying references to the same object, this should not use more
        # memory.
        return {name: _data_array_to_dict(item) for name, item in scipp_obj.items()}


def _variable_to_dict(v):
    return {"dims": _dims_to_strings(v.dims),
            "shape": v.shape,
            "values": v.values,
            "variances": v.variances,
            "unit": v.unit,
            "dtype": v.dtype}


def _data_array_to_dict(da):
    out = {"coords": {}, "masks": {}, "attrs": {}}
    for key in out.keys():
        for name, item in getattr(da, key).items():
            out[key][str(name)] = _variable_to_dict(item)
    out["data"] = _variable_to_dict(da.data)
    out["values"] = da.values
    out["variances"] = da.variances
    out["dims"] = _dims_to_strings(da.dims)
    out["shape"] = da.shape
    out["name"] = da.name
    out["unit"] = da.unit
    out["dtype"] = da.dtype
    return out


def _dims_to_strings(dims):
    return [str(dim) for dim in dims]


def from_dict(dict_obj):
    if "unit" not in dict_obj:
        # Case of a Dataset-like dict
        out = sc.Dataset()
        for key, item in dict_obj.items():
            out[key] = _dict_to_data_array(item)
        return out
    elif "coords" in dict_obj:
        # Case of a DataArray-like dict
        return _dict_to_data_array(dict_obj)
    else:
        return sc.Variable(**dict_obj)


def _dict_to_data_array(d):
    out = {"coords": {}, "masks": {}, "attrs": {}}
    for key in out.keys():
        for name, item in getattr(d, key).items():
            out[key][name] = sc.Variable(**item)
    # out["data"] = _variable_to_dict(da.data)
    # out["values"] = da.values
    # out["variances"] = da.variances
    # out["dims"] = _dims_to_strings(da.dims)
    # out["shape"] = da.shape
    # out["name"] = da.name
    # out["unit"] = str(da.unit)
    # out["dtype"] = str(da.dtype)

    return sc.DataArray(data=sc.Variable(**d["data"]),
        coords=out["coords"],
        masks=out["masks"],
        attrs=out["attrs"])


    # if "dtype" in dict_obj:
    #     # Case of a Variable
