# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .. import config
from copy import copy


def _parse_params(params=None, defaults=None, globs=None, array=None):
    """
    Construct the colorbar settings using default and input values
    """
    from matplotlib.colors import Normalize, LogNorm, LinearSegmentedColormap
    from matplotlib import cm

    parsed = dict(config['plot']['params'])
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

    # if parsed["norm"] == "log":
    #     norm = LogNorm
    # elif parsed["norm"] == "linear":
    #     norm = Normalize
    # else:
    #     raise RuntimeError("Unknown norm. Expected 'linear' or 'log', "
    #                        "got {}.".format(parsed["norm"]))
    # vmin = parsed["vmin"]
    # vmax = parsed["vmax"]
    # parsed["norm"] = norm(vmin=vmin.value if vmin is not None else None,
    #                       vmax=vmax.value if vmax is not None else None)

    # Convert color into custom colormap
    if parsed["color"] is not None:
        parsed["cmap"] = LinearSegmentedColormap.from_list(
            "tmp", [parsed["color"], parsed["color"]])
    else:
        parsed["cmap"] = copy(cm.get_cmap(parsed["cmap"]))

    if parsed["under_color"] is None:
        parsed["cmap"].set_under(parsed["cmap"](0.0))
    else:
        parsed["cmap"].set_under(parsed["under_color"])
    if parsed["over_color"] is None:
        parsed["cmap"].set_over(parsed["cmap"](1.0))
    else:
        parsed["cmap"].set_over(parsed["over_color"])

    return parsed


def make_params(*, cmap=None, norm=None, vmin=None, vmax=None, masks=None, color=None):
    # Scan the input data and collect information
    params = {"values": {}, "masks": {}}
    globs = {"cmap": cmap, "norm": norm, "vmin": vmin, "vmax": vmax, "color": color}
    masks_globs = {"norm": norm, "vmin": vmin, "vmax": vmax}
    # Get the colormap and normalization
    params["values"] = _parse_params(globs=globs)
    params["masks"] = _parse_params(params=masks,
                                    defaults={
                                        "cmap": "gray",
                                        "cbar": False,
                                        "under_color": None,
                                        "over_color": None
                                    },
                                    globs=masks_globs)
    # Set cmap extend state: if we have sliders then we need to extend.
    # We also need to extend if vmin or vmax are set.
    extend_cmap = "neither"
    if (vmin is not None) and (vmax is not None):
        extend_cmap = "both"
    elif vmin is not None:
        extend_cmap = "min"
    elif vmax is not None:
        extend_cmap = "max"

    params['extend_cmap'] = extend_cmap
    return params
