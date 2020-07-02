# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .. import _utils as su


def to_dict(scipp_obj):

    if su.is_variable(scipp_obj):
        return _variable_to_dict(scipp_obj)
    elif su.is_data_array(scipp_obj):
        return _data_array_to_dict(scipp_obj)
    elif su.is_dataset(scipp_obj):
        return {name: _data_array_to_dict(item) for name, item in scipp_obj.items()}


def _variable_to_dict(v):
    return {"dims": _dims_to_strings(v.dims),
            "shape": v.shape,
            "values": v.values,
            "variances": v.variances,
            "unit": str(v.unit),
            "dtype": str(v.dtype)}


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
    return out


def _dims_to_strings(dims):
    return [str(dim) for dim in dims]
