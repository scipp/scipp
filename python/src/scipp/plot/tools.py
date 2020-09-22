# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import config
from .._utils import name_with_unit
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


def to_bin_centers(x, dim):
    """
    Convert array edges to centers
    """
    return 0.5 * (x[dim, 1:] + x[dim, :-1])


def to_bin_edges(x, dim):
    """
    Convert array centers to edges
    """
    idim = x.dims.index(dim)
    if x.shape[idim] < 2:
        one = 1.0 * x.unit
        return sc.concatenate(x[dim, 0:1] - one, x[dim, 0:1] + one, dim)
    else:
        center = to_bin_centers(x, dim)
        # Note: use range of 0:1 to keep dimension dim in the slice to avoid
        # switching round dimension order in concatenate step.
        left = center[dim, 0:1] - (x[dim, 1] - x[dim, 0])
        right = center[dim, -1] + (x[dim, -1] - x[dim, -2])
        return sc.concatenate(sc.concatenate(left, center, dim), right, dim)


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

    if parsed["log"]:
        norm = LogNorm
    else:
        norm = Normalize
    parsed["norm"] = norm(vmin=parsed["vmin"], vmax=parsed["vmax"])

    # Convert color into custom colormap
    if parsed["color"] is not None:
        parsed["cmap"] = LinearSegmentedColormap.from_list(
            "tmp", [parsed["color"], parsed["color"]])

    if variable is not None:
        parsed["unit"] = name_with_unit(var=variable, name="")

    return parsed


def make_fake_coord(dim, size, unit=None):
    args = {"values": np.arange(size, dtype=np.float64)}
    if unit is not None:
        args["unit"] = unit
    return sc.Variable(dims=[dim], **args)


def vars_to_err(v):
    with np.errstate(invalid="ignore"):
        v = np.sqrt(v)
    non_finites = np.where(np.logical_not(np.isfinite(v)))
    v[non_finites] = 0.0
    return v


def mask_to_float(mask, var):
    return np.where(mask, var, None).astype(np.float)
