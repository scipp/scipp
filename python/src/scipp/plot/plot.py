# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from . import config
from .._scipp import core as sc


def plot(input_data, collapse=None, backend=None, color=None, **kwargs):
    """
    Wrapper function to plot any kind of dataset
    """

    # Delayed imports
    from .tools import get_color
    from .plot_collapse import plot_collapse
    from .dispatch import dispatch

    if backend is None:
        backend = config.backend

    # Create a list of variables which will then be dispatched to correct
    # plotting function.
    # Search through the variables and group the 1D datasets that have
    # the same coordinates and units.
    # tobeplotted is a dict that holds three items:
    # [number_of_dimensions, Dataset, color_list].
    tp = type(input_data)
    if tp is sc.DataProxy or tp is sc.DataArray:
        ds = sc.Dataset()
        ds[input_data.name] = input_data
        input_data = ds

    # Prepare color containers
    auto_color = False
    if color is None:
        auto_color = True
    color_count = 0

    tobeplotted = dict()
    sparse_dim = dict()
    for name, var in sorted(input_data):
        ndims = len(var.dims)
        sp_dim = var.sparse_dim
        if ndims == 1:
            # Construct a key from the dimension and the unit, to group
            # compatible data together.
            key = "{}.".format(str(var.dims[0]))
            if sp_dim is not None:
                key = "{}{}".format(key, str(var.coords[sp_dim].unit))
            else:
                key = "{}{}".format(key, str(var.unit))

            if key not in tobeplotted.keys():
                tobeplotted[key] = [ndims, sc.Dataset(), []]
            tobeplotted[key][1][name] = input_data[name]
            if auto_color:
                col = get_color(index=color_count)
            elif not isinstance(color, list):
                col = color
            else:
                col = color[color_count]
            tobeplotted[key][2].append(col)
            color_count += 1
        elif ndims > 1:
            key = name
            tobeplotted[key] = [ndims, sc.Dataset(input_data[name]), None]
        sparse_dim[key] = sp_dim

    # Plot all the subsets
    # color_count = 0
    output = dict()
    for key, val in tobeplotted.items():
        if collapse is not None:
            output[key] = plot_collapse(input_data=val[1],
                                        dim=collapse,
                                        backend=backend,
                                        color=val[2],
                                        **kwargs)
        else:
            output[key] = dispatch(input_data=val[1],
                                   ndim=val[0],
                                   backend=backend,
                                   color=val[2],
                                   sparse_dim=sparse_dim[key],
                                   **kwargs)

    if backend == "matplotlib":
        return output
    else:
        return
