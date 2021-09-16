# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from __future__ import annotations

from ..core import vector, vectors, matrix, matrices
from ..core import dtype
from ..core import Variable, DataArray, Dataset
from ..typing import VariableLike

import numpy as np
from collections import defaultdict


def to_dict(scipp_obj: VariableLike) -> dict:
    """
    Convert a scipp object (Variable, DataArray or Dataset) to a python dict.

    :param scipp_obj: A Variable, DataArray or Dataset to be converted to a
                      python dict.
    :return: A dict containing all the information necessary to fully define
             the supplied scipp object.
    """
    if isinstance(scipp_obj, Variable):
        return _variable_to_dict(scipp_obj)
    elif isinstance(scipp_obj, DataArray):
        return _data_array_to_dict(scipp_obj)
    elif isinstance(scipp_obj, Dataset):
        # TODO: This currently duplicates all coordinates that would otherwise
        # be at the Dataset level onto the individual DataArrays. We are also
        # manually duplicating all attributes, since these are not carried when
        # accessing items of a Dataset.
        out = {}
        for name, item in scipp_obj.items():
            out[name] = _data_array_to_dict(item)
        return out


def _vec_parser(x, shp):
    """
    Parse vector_3_float to 2D numpy array
    """
    return np.array(x)


def _variable_to_dict(v):
    """
    Convert a scipp Variable to a python dict.
    """
    out = {
        "dims": _dims_to_strings(v.dims),
        "shape": v.shape,
        "unit": v.unit,
        "dtype": v.dtype
    }

    # Use defaultdict to return the raw values/variances by default
    dtype_parser = defaultdict(lambda: lambda x, y: x)
    # Using raw dtypes as dict keys doesn't appear to work, so we need to
    # convert to strings.
    dtype_parser.update({
        str(dtype.vector_3_float64): _vec_parser,
        str(dtype.matrix_3_float64): _vec_parser,
        str(dtype.string): _vec_parser,
    })

    str_dtype = str(v.dtype)

    # Check if variable is 0D:
    suffix = "s" if len(out["dims"]) > 0 else ""
    out["values"] = dtype_parser[str_dtype](getattr(v, "value" + suffix), v.shape)
    var = getattr(v, "variance" + suffix)
    out["variances"] = dtype_parser[str_dtype](var,
                                               v.shape) if var is not None else None
    return out


def _data_array_to_dict(da):
    """
    Convert a scipp DataArray to a python dict.
    """
    out = {"coords": {}, "masks": {}, "attrs": {}}
    for key in out.keys():
        for name, item in getattr(da, key).items():
            out[key][str(name)] = _variable_to_dict(item)
    out['coords'] = out.pop('coords')
    out["data"] = _variable_to_dict(da.data)
    out["name"] = da.name
    return out


def _dims_to_strings(dims):
    """
    Convert dims that may or may not be strings to strings.
    """
    return [str(dim) for dim in dims]


def from_dict(dict_obj: dict) -> VariableLike:
    """
    Convert a python dict to a scipp Variable, DataArray or Dataset.
    If the input keys contain both `'coords'` and `'data'`, then a DataArray is
    returned.
    If the input keys contain both `'dims'` and `'values'`, as Variable is
    returned.
    Otherwise, a Dataset is returned.

    :param dict_obj: A python dict to be converted to a scipp object.
    :return: A scipp Variable, DataArray or Dataset.
    """
    keys_as_set = set(dict_obj.keys())
    if {"coords", "data"}.issubset(keys_as_set):
        # Case of a DataArray-like dict (most-likely)
        return _dict_to_data_array(dict_obj)
    elif keys_as_set.issubset({"dims", "values", "variances",
                               "unit", "dtype", "shape"}):  # yapf: disable
        # Case of a Variable-like dict (most-likely)
        return _dict_to_variable(dict_obj)
    else:
        # Case of a Dataset-like dict
        out = Dataset()
        for key, item in dict_obj.items():
            out[key] = _dict_to_data_array(item)
        return out


def _dict_to_variable(d):
    """
    Convert a python dict to a scipp Variable.
    """
    d = dict(d)
    # The Variable constructor does not accept both `shape` and `values`. If
    # `values` is present, remove `shape` from the list.
    keylist = list(d.keys())
    if "values" in keylist and "shape" in keylist:
        keylist.remove("shape")
    out = {}

    for key in keylist:
        if key == "dtype" and isinstance(d[key], str):
            out[key] = getattr(dtype, d[key])
        else:
            out[key] = d[key]
    # Hack for types that cannot be directly constructed using Variable()
    if out['dims']:
        init = {'vector_3_float64': vectors, 'matrix_3_float64': matrices}
    else:
        init = {'vector_3_float64': vector, 'matrix_3_float64': matrix}
    make_var = init.get(str(out.get('dtype', None)), Variable)
    if make_var != Variable:
        if not out['dims']:
            out['value'] = out['values']
            del out['values']
            del out['dims']
        for key in ['dtype', 'variance', 'variances']:
            if key in out:
                del out[key]
    return make_var(**out)


def _dict_to_data_array(d):
    """
    Convert a python dict to a scipp DataArray.
    """
    d = dict(d)
    if "data" not in d:
        raise KeyError("To create a DataArray, the supplied dict must contain "
                       "'data'. Got {}.".format(d.keys()))
    out = {"coords": {}, "masks": {}, "attrs": {}}
    for key in out.keys():
        if key in d:
            for name, item in d[key].items():
                out[key][name] = _dict_to_variable(item)
    out["data"] = _dict_to_variable(d["data"])
    return DataArray(**out)
