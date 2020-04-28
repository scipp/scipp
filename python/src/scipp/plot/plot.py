# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from typing import List, Dict

# Scipp imports
from scipp.plot.plot_impl.prepare_collapse import prepare_collapse
from scipp.plot.plot_impl.plot_request import PlotRequest
from .sciplot import SciPlot
from .tools import get_line_param
from .._scipp import core as sc


def tiled_plot(scipp_objs: List,
               collapse=None,
               projection=None,
               axes=None,
               color=None,
               marker=None,
               linestyle=None,
               linewidth=None,
               bins=None,
               **kwargs):
    if isinstance(scipp_objs, list) or isinstance(scipp_objs, set):
        packed_objs = {}
        for obj in scipp_objs:
            obj_dict = _pack_into_dict(obj)
            if not obj_dict:
                # Nothing to plot
                continue

            # We need to be careful of duplicates - keep trying keys until we find a unique name
            assert len(obj_dict) == 1, "Duplicate name handling requires _pack_into_dict to return max one val"
            original_key = str(next(iter(obj_dict)))
            key = original_key
            i = 1
            while key in packed_objs:
                key = f"{original_key}_{i}"
                i += 1

            packed_objs[key] = obj_dict[original_key]
    elif isinstance(scipp_objs, dict):
        packed_objs = scipp_objs
    else:
        raise ValueError("A list, set or dict of plottable items must be passed,"
                         f" got {repr(scipp_objs)} instead")

    if not packed_objs:
        return  # Nothing to do

    to_be_plotted = _prepare_plot(axes=axes, bins=bins, color=color,
                                  inventory=packed_objs, linestyle=linestyle,
                                  linewidth=linewidth, marker=marker, projection=projection)

    # Delayed imports
    from scipp.plot.plot_impl.dispatch import dispatch

    # Plot all the subsets
    output = SciPlot()
    # We pack this as a list of requests (rather than into the struct as a list)
    # so that each subplot could have its own options in the future
    request_list = []
    for key, val in to_be_plotted.items():
        if collapse is not None:
            request_list.append(prepare_collapse(data_array=val["scipp_obj_dict"][key],
                                                 dim=collapse))
        else:
            request_list.append(PlotRequest(bins=bins,
                                            mpl_line_params=val["mpl_line_params"],
                                            ndims=val["ndims"],
                                            projection=projection,
                                            scipp_objs=val["scipp_obj_dict"]))
    # TODO how do we decide which key this belongs to?
    output["tiled"] = dispatch(request=request_list, **kwargs)
    return output


def plot(scipp_obj,
         collapse=None,
         projection=None,
         axes=None,
         color=None,
         marker=None,
         linestyle=None,
         linewidth=None,
         bins=None,
         **kwargs):
    """
    Wrapper function to plot any kind of dataset
    """

    # Delayed imports
    from scipp.plot.plot_impl.dispatch import dispatch

    inventory = _pack_into_dict(scipp_obj)

    tobeplotted = _prepare_plot(axes, bins, color, inventory, linestyle,
                                linewidth, marker, projection)

    # Plot all the subsets
    output = SciPlot()
    for key, val in tobeplotted.items():
        if collapse is not None:
            request = prepare_collapse(data_array=val["scipp_obj_dict"][key],
                                       dim=collapse)
        else:
            request = PlotRequest(bins=bins,
                                  mpl_line_params=val["mpl_line_params"],
                                  ndims=val["ndims"],
                                  projection=projection,
                                  scipp_objs=val["scipp_obj_dict"])
        output[key] = dispatch(request=[request], **kwargs)

    return output


def _prepare_plot(axes, bins, color, inventory, linestyle, linewidth, marker, projection):
    # Prepare container for matplotlib line parameters
    line_params = {
        "color": color,
        "marker": marker,
        "linestyle": linestyle,
        "linewidth": linewidth
    }

    # Counter for 1d/event data
    line_count = -1
    # Create a list of variables which will then be dispatched to correct
    # plotting function.
    # Search through the variables and group the 1D datasets that have
    # the same coordinates and units.
    # tobeplotted is a dict that holds four items:
    # {number_of_dimensions, Dataset, axes, line_parameters}.
    tobeplotted = dict()
    for name, var in sorted(inventory.items()):
        if sc.contains_events(var) and bins is None:
            raise RuntimeError("The `bins` argument must be specified when "
                               "plotting event data.")

        ndims = len(var.dims)
        if bins is not None and sc.contains_events(var):
            ndims += 1
        if ndims > 0:
            ax = axes
            if ndims == 1 or projection == "1d" or projection == "1D":
                # Construct a key from the dimensions
                if axes is not None:
                    # Check if we are dealing with a dict mapping dimensions to
                    # labels
                    if isinstance(axes, dict):
                        key = axes[str(var.dims[0])]
                        ax = [key]
                    else:
                        key = ".".join(axes)
                else:
                    key = ".".join([str(dim) for dim in var.dims])
                # Add unit to key
                key = "{}.{}".format(key, str(var.unit))
                line_count += 1
            else:
                key = name

            mpl_line_params = {}
            for n, p in line_params.items():
                if p is None:
                    mpl_line_params[n] = line_count
                elif isinstance(p, list):
                    mpl_line_params[n] = p[line_count]
                elif isinstance(p, dict):
                    if name in p:
                        mpl_line_params[n] = p[name]
                    else:
                        mpl_line_params[n] = line_count
                else:
                    mpl_line_params[n] = p
                if isinstance(mpl_line_params[n], int):
                    mpl_line_params[n] = get_line_param(
                        name=n, index=mpl_line_params[n])

            original_key = key
            i = 1
            while key in tobeplotted:
                key = original_key + '_' + str(i)
                i += 1

            tobeplotted[key] = dict(ndims=ndims,
                                    scipp_obj_dict=dict(),
                                    axes=ax,
                                    mpl_line_params=dict())
            for n in mpl_line_params.keys():
                tobeplotted[key]["mpl_line_params"][n] = {}
            tobeplotted[key]["scipp_obj_dict"][name] = inventory[name]
            for n, p in mpl_line_params.items():
                tobeplotted[key]["mpl_line_params"][n][name] = p
    return tobeplotted


def _pack_into_dict(scipp_obj) -> Dict:
    inventory = dict()
    tp = type(scipp_obj)
    if tp is sc.Dataset or tp is sc.DatasetView:
        for name in sorted(scipp_obj.keys()):
            inventory[name] = scipp_obj[name]
    elif tp is sc.Variable or tp is sc.VariableView:
        inventory[str(tp)] = sc.DataArray(data=scipp_obj)
    elif tp is sc.DataArray or tp is sc.DataArrayView:
        inventory[scipp_obj.name] = scipp_obj
    elif tp is dict:
        inventory = scipp_obj
    else:
        raise RuntimeError("plot: Unknown input type: {}. Allowed inputs are "
                           "a Dataset, a DataArray, a Variable (and their "
                           "respective proxies), and a dict of "
                           "DataArrays.".format(tp))
    return inventory
