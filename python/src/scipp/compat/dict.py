# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .. import detail
from .. import _utils as su
from .._scipp import core as sc

import numpy as np
from collections import defaultdict
from itertools import product


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
        copy_attrs = len(scipp_obj.attrs.keys()) > 0
        for name, item in scipp_obj.items():
            out[name] = _data_array_to_dict(item)
            if copy_attrs:
                for key, attr in scipp_obj.attrs.items():
                    out[name]["attrs"][key] = _variable_to_dict(attr)
        return out


def _vec_parser(x, shp):
    """
    Parse vector_3_float to 2D numpy array
    """
    return np.array(x)


def _event_parser(x, shp):
    """
    Parse event list data to numpy array of numpy arrays
    """
    return np.reshape([np.array(x[i]) for i in range(len(x))], shp)


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
        str(sc.dtype.event_list_float32): _event_parser,
        str(sc.dtype.event_list_float64): _event_parser
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
    out = {"coords": {}, "masks": {}, "attrs": {}}
    for key in out.keys():
        for name, item in getattr(da, key).items():
            out[key][str(name)] = _variable_to_dict(item)
    if da.unaligned is not None:
        out["unaligned"] = _data_array_to_dict(da.unaligned)
    else:
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
    if ({"coords", "data"}.issubset(keys_as_set)
            or {"coords", "unaligned"}.issubset(keys_as_set)):
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

    is_event_data = False
    if "dtype" in keylist:
        is_event_data = str(d["dtype"]).startswith("event_list_float")

    # TODO: maybe this constructor would be worth adding to the scipp module
    # itself?
    if is_event_data:
        if "shape" not in d:
            shp = d["values"].shape
        else:
            shp = d["shape"]

        var = sc.Variable(dims=d["dims"],
                          shape=shp,
                          dtype=getattr(sc.dtype, str(d["dtype"])))

        ndim = len(d["dims"])
        indices = tuple()
        for i in range(ndim):
            indices += range(shp[i]),
        # Now construct all indices combinations using itertools
        for ind in product(*indices):
            # And for each indices combination, slice the original data
            vslice = var
            aslice = d["values"]
            for i in range(ndim):
                vslice = vslice[d["dims"][i], ind[i]]
                aslice = aslice[ind[i]]
            vslice.values = aslice
        return var

    else:
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
    if ("data" not in d) and ("unaligned" not in d):
        raise KeyError("To create a DataArray, the supplied dict must contain "
                       "either 'data' or 'unaligned'. "
                       "Got {}.".format(d.keys()))
    out = {"coords": {}, "masks": {}, "attrs": {}}
    for key in out.keys():
        if key in d:
            for name, item in d[key].items():
                out[key][name] = _dict_to_variable(item)
    unaligned = None
    if "unaligned" in d:
        unaligned = _dict_to_data_array(d["unaligned"])
    else:
        out["data"] = _dict_to_variable(d["data"])
    da = detail.move_to_data_array(**out)
    if unaligned is not None:
        da.unaligned = detail.move(unaligned)
    return da
