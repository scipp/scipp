# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from __future__ import annotations

from collections import defaultdict
from collections.abc import Callable, Sequence
from typing import Any, cast

import numpy as np
import numpy.typing as npt

from ..core import (
    DataArray,
    Dataset,
    DType,
    Variable,
    vector,
    vectors,
)
from ..spatial import (
    affine_transform,
    affine_transforms,
    linear_transform,
    linear_transforms,
)
from ..typing import VariableLike


def to_dict(scipp_obj: VariableLike) -> dict[str, Any]:
    """Convert a Scipp object (Variable, DataArray or Dataset)
    to a Python :class:`dict`.

    Parameters
    ----------
    scipp_obj:
        Scipp object to be converted to a python dict.

    Returns
    -------
    :
        A dict containing all the information necessary to fully define
        the supplied Scipp object.

    See Also
    --------
    scipp.from_dict
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
    else:
        raise NotImplementedError("Unsupported type for to_dict")


def _vec_parser(x: Any, shp: object) -> npt.NDArray[Any]:
    """Parse vector_3_float to 2D NumPy array."""
    return np.array(x)


def _obj_array_parser(x: Any, shp: object) -> npt.NDArray[Any]:
    """Coerce ElementArrayView_PyObject into a flattened NumPy array."""
    if not shp:
        return np.array(x)
    # Probably an ElementArrayView_PyObject, which cannot be iterated
    # over normally (despite having an `.__iter__()`)
    return np.array([x[i] for i in range(len(x))])


def _variable_to_dict(v: Variable) -> dict[str, Any]:
    """Convert a Scipp Variable to a python dict."""
    out: dict[str, Any] = {
        "dims": v.dims,
        "shape": v.shape,
        "unit": v.unit,
        "dtype": v.dtype,
    }
    if not v.aligned:
        out["aligned"] = False

    # Use defaultdict to return the raw values/variances by default
    dtype_parser: dict[str, Callable[..., Any]] = defaultdict(lambda: lambda x, y: x)
    # Using raw dtypes as dict keys doesn't appear to work, so we need to
    # convert to strings.
    dtype_parser.update(
        {
            str(DType.vector3): _vec_parser,
            str(DType.linear_transform3): _vec_parser,
            str(DType.string): _vec_parser,
            str(DType.PyObject): _obj_array_parser,
        }
    )

    str_dtype = str(v.dtype)

    # Check if variable is 0D:
    suffix = "s" if len(out["dims"]) > 0 else ""
    out["values"] = dtype_parser[str_dtype](getattr(v, "value" + suffix), v.shape)
    var = getattr(v, "variance" + suffix)
    out["variances"] = (
        dtype_parser[str_dtype](var, v.shape) if var is not None else None
    )
    return out


def _data_array_to_dict(da: DataArray) -> dict[str, Any]:
    """Convert a Scipp DataArray to a python dict."""
    out: dict[str, Any] = {"coords": {}, "masks": {}}
    for key in out.keys():
        for name, item in getattr(da, key).items():
            out[key][str(name)] = _variable_to_dict(item)
    out['coords'] = out.pop('coords')
    out["data"] = _variable_to_dict(da.data)
    out["name"] = da.name
    return out


def from_dict(dict_obj: dict[str, Any]) -> VariableLike:
    """Convert a Python dict to a Scipp Variable, DataArray or Dataset.

    If the input keys contain both `'coords'` and `'data'`, then a DataArray is
    returned.
    If the input keys contain both `'dims'` and `'values'`, as Variable is
    returned.
    Otherwise, a Dataset is returned.

    Parameters
    ----------
    dict_obj:
        A python dict to be converted to a scipp object.

    Returns
    -------
    :
        A Scipp Variable, DataArray or Dataset.

    See Also
    --------
    scipp.to_dict
    """
    keys_as_set = set(dict_obj.keys())
    if {"coords", "data"}.issubset(keys_as_set):
        # Case of a DataArray-like dict (most-likely)
        return _dict_to_data_array(dict_obj)
    elif keys_as_set.issubset(
        {"dims", "values", "variances", "unit", "dtype", "shape", "aligned"}
    ):
        # Case of a Variable-like dict (most-likely)
        return _dict_to_variable(dict_obj)
    else:
        # Case of a Dataset-like dict
        out = Dataset(
            {key: _dict_to_data_array(item) for key, item in dict_obj.items()}
        )
        return out


def _dict_to_variable(d: dict[str, Any]) -> Variable:
    """Convert a Python dict to a Scipp Variable."""
    d = dict(d)
    # The Variable constructor does not accept both `shape` and `values`. If
    # `values` is present, remove `shape` from the list.
    keylist = set(d.keys())
    if "values" in keylist and "shape" in keylist:
        keylist.remove("shape")
    out = {}

    for key in keylist:
        if key == "dtype" and isinstance(d[key], str):
            out[key] = getattr(DType, d[key])
        else:
            out[key] = d[key]

    # Hack for types that cannot be directly constructed using Variable()
    if out['dims']:
        init: dict[str, Callable[..., Variable]] = {
            'vector3': vectors,
            'linear_transform3': linear_transforms,
            # Note: while one can technically directly construct a
            # Variable of this .dtype, it is error prone due to the
            # different data layouts in numpy/scipy and in the Eigen
            # matrices, so just use the helper function
            'affine_transform3': affine_transforms,
        }
    else:
        init = {
            'vector3': vector,
            'linear_transform3': linear_transform,
            'affine_transform3': affine_transform,
        }
    make_var = init.get(dtype := str(out.get('dtype', None)), Variable)
    if make_var != Variable:
        if not out['dims']:
            out['value'] = out['values']
            del out['values']
            del out['dims']
        for key in ['dtype', 'variance', 'variances']:
            out.pop(key, None)

    # Note: N-D string/object variables are stored as 1-D arrays
    # internally; recover the dimensionality info where appropriate
    if (
        dtype in ('string', 'PyObject')
        and d.get('dims')
        and len(cast(Sequence[str], d['dims'])) > 1
        and d.get('shape') is not None
    ):
        var = make_var(**{**out, 'dims': ['_']}).fold(
            '_', dims=d['dims'], shape=d['shape']
        )
    else:
        var = make_var(**out)
    return var


def _dict_to_data_array(d: dict[str, Any]) -> DataArray:
    """Convert a Python dict to a Scipp DataArray."""
    d = dict(d)
    if "data" not in d:
        raise KeyError(
            "To create a DataArray, the supplied dict must contain "
            f"'data'. Got {d.keys()}."
        )
    meta: dict[str, dict[str, Variable]] = {"coords": {}, "masks": {}}
    for key in meta.keys():
        if key in d:
            for name, item in d[key].items():
                meta[key][name] = _dict_to_variable(item)
    data = _dict_to_variable(d["data"])
    return DataArray(data, **meta)  # type: ignore[arg-type]
