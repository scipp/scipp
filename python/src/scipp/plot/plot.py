# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .._scipp import core as sc
from .sciplot import SciPlot


def plot(input_data, collapse=None, projection=None, axes=None, color=None,
         marker=None, linestyle=None, linewidth=None, **kwargs):
    """
    Wrapper function to plot any kind of dataset
    """

    # Delayed imports
    from .tools import get_line_param
    from .plot_collapse import plot_collapse
    from .dispatch import dispatch

    # Create a list of variables which will then be dispatched to correct
    # plotting function.
    # Search through the variables and group the 1D datasets that have
    # the same coordinates and units.
    # tobeplotted is a dict that holds four items:
    # [number_of_dimensions, Dataset, color_list, axes].
    tp = type(input_data)
    if tp is sc.DataProxy or tp is sc.DataArray:
        ds = sc.Dataset()
        ds[input_data.name] = input_data
        input_data = ds

    # Prepare color and marker containers
    line_params = {"color": color,
                   "marker": marker,
                   "linestyle": linestyle,
                   "linewidth": linewidth}

    # auto_color = False
    # if color is None:
    #     auto_color = True

    # Counter for 1d data
    line_count = -1

    tobeplotted = dict()
    sparse_dim = dict()
    for name, var in sorted(input_data):
        ndims = len(var.dims)
        if ndims > 0:
            sp_dim = var.sparse_dim
            ax = axes
            if ndims == 1 or projection == "1d" or projection == "1D":
                # Construct a key from the dimensions
                if axes is not None:
                    # Check if we are dealing with a dict mapping dimensions to
                    # labels
                    if isinstance(axes, dict):
                        key = axes[var.dims[0]]
                        ax = [key]
                    else:
                        key = "{}.".format(str(axes))
                else:
                    key = "{}.".format(str(var.dims))
                # Add unit to key
                if sp_dim is not None:
                    key = "{}{}".format(key, str(var.coords[sp_dim].unit))
                else:
                    key = "{}{}".format(key, str(var.unit))
                line_count += 1
            else:
                key = name

            mpl_line_params = {}
            for n, p in line_params.items():
                if p is None:
                    mpl_line_params[n] = get_line_param(name=n, index=line_count)
                elif isinstance(p, list):
                    mpl_line_params[n] = p[line_count]
                    if isinstance(mpl_line_params[n], int):
                        mpl_line_params[n] = get_line_param(
                            name=n, index=mpl_line_params[n])
                elif isinstance(p, int):
                    mpl_line_params[n] = get_line_param(name=n, index=p)
                else:
                    mpl_line_params[n] = p


            print(mpl_line_params)

            # if auto_color:
            #     col = get_color(index=line_count)
            # elif isinstance(color, list):
            #     col = color[line_count]
            #     if isinstance(col, int):
            #         col = get_color(index=col)
            # elif isinstance(color, int):
            #     col = get_color(index=color)
            # else:
            #     col = color



            # if auto_color:
            #     col = get_color(index=line_count)
            # elif isinstance(color, list):
            #     col = color[line_count]
            #     if isinstance(col, int):
            #         col = get_color(index=col)
            # elif isinstance(color, int):
            #     col = get_color(index=color)
            # else:
            #     col = color
            # # color_count += 1

            if key not in tobeplotted.keys():
                tobeplotted[key] = dict(ndims=ndims, dataset=sc.Dataset(),
                                        axes=ax, mpl_line_params=dict())
                for n in mpl_line_params.keys():
                    tobeplotted[key]["mpl_line_params"][n] = []
            tobeplotted[key]["dataset"][name] = input_data[name]
            for n, p in mpl_line_params.items():
                tobeplotted[key]["mpl_line_params"][n].append(p)
            sparse_dim[key] = sp_dim

    # Plot all the subsets
    output = SciPlot()
    for key, val in tobeplotted.items():
        if collapse is not None:
            output[key] = plot_collapse(input_data=val["dataset"],
                                        dim=collapse,
                                        axes=val["axes"],
                                        **kwargs)
        else:
            output[key] = dispatch(input_data=val["dataset"],
                                   name=key,
                                   ndim=val["ndims"],
                                   # color=val["color"],
                                   sparse_dim=sparse_dim[key],
                                   projection=projection,
                                   axes=val["axes"],
                                   mpl_line_params=val["mpl_line_params"],
                                   **kwargs)
    return output
