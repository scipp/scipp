# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .. import detail
from .. import _utils as su
from .._scipp import core as sc

import numpy as np
from collections import defaultdict


def to_dict(scipp_obj):
    """
    Convert a scipp object (Variable, DataArray or Dataset) to a python dict.

    :param scipp_obj: A Variable, DataArray or Dataset to be converted to a
                      python dict.
    :type scipp_obj: Variable, DataArray, or Dataset
    :return: A dict containing all the information necessary to fully define
             the supplied scipp object.
    :rtype: dict
    """
    if su.is_variable(scipp_obj):
        return _variable_to_dict(scipp_obj)
    elif su.is_data_array(scipp_obj):
        return _data_array_to_dict(scipp_obj)
    elif su.is_dataset(scipp_obj):
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
        str(sc.dtype.vector_3_float64): _vec_parser,
        str(sc.dtype.matrix_3_float64): _vec_parser,
        str(sc.dtype.string): _vec_parser,
    })

    str_dtype = str(v.dtype)

    # Check if variable is 0D:
    suffix = "s" if len(out["dims"]) > 0 else ""
    out["value" + suffix] = dtype_parser[str_dtype](getattr(
        v, "value" + suffix), v.shape)
    var = getattr(v, "variance" + suffix)
    out["variance" + suffix] = dtype_parser[str_dtype](
        var, v.shape) if var is not None else None
    return out


def _data_array_to_dict(da):
    """
    Convert a scipp DataArray to a python dict.
    """
    out = {"aligned_coords": {}, "masks": {}, "unaligned_coords": {}}
    for key in out.keys():
        for name, item in getattr(da, key).items():
            out[key][str(name)] = _variable_to_dict(item)
    out['coords'] = out.pop('aligned_coords')
    out["data"] = _variable_to_dict(da.data)
    out["name"] = da.name
    return out


def _dims_to_strings(dims):
    """
    Convert dims that may or may not be strings to strings.
    """
    return [str(dim) for dim in dims]


def from_dict(dict_obj):
    """
    Convert a python dict to a scipp Variable, DataArray or Dataset.
    If the input keys contain both `'coords'` and `'data'`, then a DataArray is
    returned.
    If the input keys contain both `'dims'` and `'values'`, as Variable is
    returned.
    Otherwise, a Dataset is returned.

    :param dict_obj: A python dict to be converted to a scipp object.
    :type dict_obj: dict
    :return: A scipp Variable, DataArray or Dataset.
    :rtype: Variable, DataArray, or Dataset
    """
    keys_as_set = set(dict_obj.keys())
    if ({"coords", "data"}.issubset(keys_as_set)):
        # Case of a DataArray-like dict (most-likely)
        return _dict_to_data_array(dict_obj)
    elif (keys_as_set.issubset(
        {"dims", "values", "variances", "unit", "dtype", "shape"})
          or keys_as_set.issubset(
              {"value", "variance", "unit", "dtype", "shape", "dims"})):
        # Case of a Variable-like dict (most-likely)
        return _dict_to_variable(dict_obj)
    else:
        # Case of a Dataset-like dict
        out = sc.Dataset()
        for key, item in dict_obj.items():
            out[key] = _dict_to_data_array(item)
        return out


def _dict_to_variable(d):
    """
    Convert a python dict to a scipp Variable.
    """
    _check_dict_input(d)
    # The Variable constructor does not accept both `shape` and `values`. If
    # `values` is present, remove `shape` from the list. Also remove `dims` in
    # the case of a 0D variable.
    keylist = list(d.keys())
    if "value" in keylist:
        if "shape" in keylist:
            keylist.remove("shape")
        if "dims" in keylist:
            keylist.remove("dims")
    if "values" in keylist and "shape" in keylist:
        keylist.remove("shape")
    out = {}

    for key in keylist:
        if key == "dtype" and isinstance(d[key], str):
            out[key] = getattr(sc.dtype, d[key])
        else:
            out[key] = d[key]
    return sc.Variable(**out)


def _dict_to_data_array(d):
    """
    Convert a python dict to a scipp DataArray.
    """
    _check_dict_input(d)
    if ("data" not in d):
        raise KeyError("To create a DataArray, the supplied dict must contain "
                       "'data'. Got {}.".format(d.keys()))
    out = {"coords": {}, "masks": {}, "unaligned_coords": {}}
    for key in out.keys():
        if key in d:
            for name, item in d[key].items():
                out[key][name] = _dict_to_variable(item)
    out["data"] = _dict_to_variable(d["data"])
    da = detail.move_to_data_array(**out)
    return da


def _check_dict_input(d):
    """
    Throw if the input is not a dict
    """
    if not isinstance(d, dict):
        raise TypeError("The supplied input must be a dictionary.")
