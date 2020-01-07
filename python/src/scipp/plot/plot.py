# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .._scipp import core as sc
from .sciplot import SciPlot


def plot(scipp_obj, collapse=None, projection=None, axes=None, color=None,
         marker=None, linestyle=None, linewidth=None, bins=None, **kwargs):
    """
    Wrapper function to plot any kind of dataset
    """

    # Delayed imports
    from .tools import get_line_param
    from .plot_collapse import plot_collapse
    from .dispatch import dispatch
    import matplotlib.pyplot as plt

    # Temporarily disable automatic plotting in notebook
    if plt.isinteractive():
        plt.ioff()
        re_enable_interactive = True
    else:
        re_enable_interactive = False

    inventory = dict()
    tp = type(scipp_obj)
    if tp is sc.Dataset or tp is sc.DatasetProxy:
        for name, var in sorted(scipp_obj):
            inventory[name] = var
    elif tp is sc.Variable or tp is sc.VariableProxy:
        inventory[str(tp)] = sc.DataArray(data=scipp_obj)
    elif tp is sc.Dataset or tp is sc.DatasetProxy:
        inventory[scipp_obj.name] = scipp_obj
    elif tp is dict:
        inventory = scipp_obj
    else:
        raise RuntimeError("plot: Unknown input type: {}. Allowed inputs are "
                           "a Dataset, a DataArray, a Variable (and their "
                           "respective proxies), and a dict of "
                           "DataArrays.".format(tp))

    # Prepare container for matplotlib line parameters
    line_params = {"color": color,
                   "marker": marker,
                   "linestyle": linestyle,
                   "linewidth": linewidth}

    # Counter for 1d/sparse data
    line_count = -1

    # Create a list of variables which will then be dispatched to correct
    # plotting function.
    # Search through the variables and group the 1D datasets that have
    # the same coordinates and units.
    # tobeplotted is a dict that holds four items:
    # {number_of_dimensions, Dataset, axes, line_parameters}.
    tobeplotted = dict()
    sparse_dim = dict()
    for name, var in sorted(inventory.items()):
        ndims = len(var.dims)
        if ndims > 0:
            sp_dim = var.sparse_dim
            ax = axes
            if ndims == 1 or projection == "1d" or projection == "1D" or \
               (sp_dim is not None and bins is None):
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
                if sp_dim is not None:
                    line_count += 1

            mpl_line_params = {}
            for n, p in line_params.items():
                if p is None:
                    mpl_line_params[n] = get_line_param(name=n,
                                                        index=line_count)
                elif isinstance(p, list):
                    mpl_line_params[n] = p[line_count]
                    if isinstance(mpl_line_params[n], int):
                        mpl_line_params[n] = get_line_param(
                            name=n, index=mpl_line_params[n])
                elif isinstance(p, int):
                    mpl_line_params[n] = get_line_param(name=n, index=p)
                else:
                    mpl_line_params[n] = p

            if key not in tobeplotted.keys():
                tobeplotted[key] = dict(ndims=ndims, scipp_obj_dict=dict(),
                                        axes=ax, mpl_line_params=dict())
                for n in mpl_line_params.keys():
                    tobeplotted[key]["mpl_line_params"][n] = []
            tobeplotted[key]["scipp_obj_dict"][name] = inventory[name]
            for n, p in mpl_line_params.items():
                tobeplotted[key]["mpl_line_params"][n].append(p)
            sparse_dim[key] = sp_dim

    # Plot all the subsets
    output = SciPlot()
    for key, val in tobeplotted.items():
        if collapse is not None:
            output[key] = plot_collapse(
                data_array=val["scipp_obj_dict"][key], dim=collapse,
                axes=val["axes"], **kwargs)
        else:
            output[key] = dispatch(scipp_obj_dict=val["scipp_obj_dict"],
                                   name=key, ndim=val["ndims"],
                                   sparse_dim=sparse_dim[key],
                                   projection=projection,
                                   axes=val["axes"],
                                   mpl_line_params=val["mpl_line_params"],
                                   bins=bins,
                                   **kwargs)

    # Re-enable automatic plotting in notebook
    if re_enable_interactive:
        plt.ion()

    return output
