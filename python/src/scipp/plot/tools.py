# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import config
from .._scipp import core as sc

# Other imports
import numpy as np


def get_line_param(name=None, index=None):
    """
    Get the default line parameter from the config.
    If an index is supplied, return the i-th item in the list.
    """
    param = getattr(config.plot, name)
    return param[index % len(param)]


def edges_to_centers(x):
    """
    Convert array edges to centers
    """
    return 0.5 * (x[1:] + x[:-1])


def centers_to_edges(x):
    """
    Convert array centers to edges
    """
    if len(x) < 2:
        dx = 0.5 * abs(x[0])
        if dx == 0.0:
            dx = 0.5
        return np.array([x[0] - dx, x[0] + dx])
    else:
        e = edges_to_centers(x)
        return np.concatenate([[2.0 * x[0] - e[0]], e, [2.0 * x[-1] - e[-1]]])


def parse_params(params=None,
                 defaults=None,
                 globs=None,
                 variable=None,
                 array=None,
                 min_val=None,
                 max_val=None):
    """
    Construct the colorbar settings using default and input values
    """
    from matplotlib.colors import Normalize, LogNorm, LinearSegmentedColormap

    parsed = dict(config.plot.params)
    if defaults is not None:
        for key, val in defaults.items():
            parsed[key] = val
    if globs is not None:
        for key, val in globs.items():
            # Global parameters need special treatment because by default they
            # are set to None, and we don't want to overwrite the defaults.
            if val is not None:
                parsed[key] = val
    if params is not None:
        if isinstance(params, bool):
            params = {"show": params}
        for key, val in params.items():
            parsed[key] = val

    need_norm = False
    # TODO: sc.min/max currently return nan if the first value in the
    # variable array is a nan. Until sc.nanmin/nanmax are implemented, we fall
    # back to using numpy, both when a Variable and a numpy array are supplied.
    if variable is not None:
        _find_min_max(variable.values, parsed)
        need_norm = True
    if array is not None:
        _find_min_max(array, parsed)
        need_norm = True

    if need_norm:
        if min_val is not None:
            parsed["vmin"] = min(parsed["vmin"], min_val)
        if max_val is not None:
            parsed["vmax"] = max(parsed["vmax"], max_val)
        if parsed["log"]:
            norm = LogNorm(vmin=10.0**parsed["vmin"],
                           vmax=10.0**parsed["vmax"])
        else:
            norm = Normalize(vmin=parsed["vmin"], vmax=parsed["vmax"])
        parsed["norm"] = norm

    # Convert color into custom colormap
    if parsed["color"] is not None:
        parsed["cmap"] = LinearSegmentedColormap.from_list(
            "tmp", [parsed["color"], parsed["color"]])

    return parsed


def make_fake_coord(dim, size, unit=None):
    args = {"values": np.arange(size)}
    if unit is not None:
        args["unit"] = unit
    return sc.Variable(dims=[dim], **args)


def _find_min_max(array, params):
    if params["vmin"] is None or params["vmax"] is None:
        if params["log"]:
            with np.errstate(divide="ignore", invalid="ignore"):
                valid = np.ma.log10(array)
        else:
            valid = np.ma.masked_invalid(array, copy=False)
    if params["vmin"] is None:
        params["vmin"] = valid.min()
    if params["vmax"] is None:
        params["vmax"] = valid.max()
