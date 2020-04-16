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
    e = edges_to_centers(x)
    return np.concatenate([[2.0 * x[0] - e[0]], e, [2.0 * x[-1] - e[-1]]])


def parse_params(params=None, defaults=None, globs=None, variable=None, array=None):
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
    if variable is not None:
        if parsed["vmin"] is None:
            parsed["vmin"] = sc.min(variable).value
        if parsed["vmax"] is None:
            parsed["vmax"] = sc.max(variable).value
        need_norm = True
    if array is not None:
        if parsed["vmin"] is None:
            parsed["vmin"] = np.nanmin(array)
        if parsed["vmax"] is None:
            parsed["vmax"] = np.nanmax(array)
        need_norm = True

    if need_norm:
        # TODO: possibly need to add a C++ method for finding min/max of
        # Variables to avoid the creation of a large array of bools in
        # np.ma.masked_invalid
        # if parsed["log"]:
        #     with np.errstate(divide="ignore", invalid="ignore"):
        #         subset = np.ma.masked_invalid(np.log10(array), copy=False)
        # else:
        #     subset = np.ma.masked_invalid(array, copy=False)
        # if parsed["vmin"] is None:
        #     parsed["vmin"] = sc.min(array).value
        # if parsed["vmax"] is None:
        #     parsed["vmax"] = sc.max(array).value
        if parsed["log"]:
            norm = LogNorm(vmin=parsed["vmin"], vmax=parsed["vmax"])
            # norm = LogNorm(vmin=10.0**parsed["vmin"],
            #                vmax=10.0**parsed["vmax"])
        else:
            norm = Normalize(vmin=parsed["vmin"], vmax=parsed["vmax"])
        parsed["norm"] = norm

    # Convert color into custom colormap
    if parsed["color"] is not None:
        parsed["cmap"] = LinearSegmentedColormap.from_list(
            "tmp", [parsed["color"], parsed["color"]])

    return parsed
