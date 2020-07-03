# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .. import _utils as su
from .._scipp import core as sc


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


def _variable_to_dict(v):
    """
    Convert a scipp Variable to a python dict.
    """
    return {
        "dims": _dims_to_strings(v.dims),
        "shape": v.shape,
        "values": v.values,
        "variances": v.variances,
        "unit": v.unit,
        "dtype": v.dtype
    }


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
    if ({"coords", "data"}.issubset(set(dict_obj.keys())) or
        {"coords", "unaligned"}.issubset(set(dict_obj.keys()))):
        # Case of a DataArray-like dict (most-likely)
        return _dict_to_data_array(dict_obj)
    elif ({"dims", "values"}.issubset(set(dict_obj.keys())) or
          {"dims", "shape"}.issubset(set(dict_obj.keys()))):
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
    out = {}
    # The Variable constructor does not accept both `shape` and `values`. If
    # `values` is present, remove `shape` from the list.
    keylist = list(d.keys())
    if "values" in keylist and "shape" in keylist:
        keylist.remove("shape")

    for key in keylist:
        out[key] = d[key]
    return sc.Variable(**out)


def _dict_to_data_array(d):
    """
    Convert a python dict to a scipp DataArray.
    """
    if ("data" not in d) and ("unaligned" not in d):
        raise KeyError("To create a Dataset, the supplied dict must contain "
                       "either 'data' or 'unaligned'. "
                       "Got {}.".format(d.keys()))
    out = {"coords": {}, "masks": {}, "attrs": {}}
    for key in out.keys():
        if key in d:
            for name, item in d[key].items():
                out[key][name] = _dict_to_variable(item)
    if "unaligned" in d:
        out["unaligned"] = _dict_to_data_array(d["unaligned"])
    else:
        out["data"] = _dict_to_variable(d["data"])

    return sc.DataArray(**out)
