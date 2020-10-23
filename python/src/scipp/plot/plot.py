# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .._scipp import core as sc
from .. import _utils as su
from .dispatch import dispatch
from .tools import make_fake_coord, get_line_param


class Plot(dict):
    """
    The Plot object is used as output for the plot command.
    It is a small wrapper around python dict, with an ipython repr.
    The dict will contain one entry for each entry in the input supplied to
    the plot function.
    More functionalities can be added in the future.
    """
    def __init__(self, *arg, **kw):
        super(Plot, self).__init__(*arg, **kw)

    def _ipython_display_(self):
        import ipywidgets as widgets
        contents = []
        for key, val in self.items():
            contents.append(val._to_widget())
        return widgets.VBox(contents)._ipython_display_()

    def as_static(self, *args, **kwargs):
        for key, item in self.items():
            self[key] = item.as_static(*args, **kwargs)


def _variable_to_data_array(variable):
    coords = {}
    for dim, size in zip(variable.dims, variable.shape):
        coords[dim] = make_fake_coord(dim, size)
    return sc.DataArray(data=variable, coords=coords)


def _raise_plot_input_error(intype):
    raise RuntimeError("plot: Unknown input type: {}. Allowed inputs are "
                       "a Dataset, a DataArray, a Variable (and their "
                       "respective proxies), and a dict of "
                       "Variables or DataArrays.".format(intype))


def _parse_input(scipp_obj):
    inventory = dict()
    if su.is_dataset(scipp_obj):
        for name in sorted(scipp_obj.keys()):
            inventory[name] = scipp_obj[name]
    elif su.is_variable(scipp_obj):
        inventory[str(type(scipp_obj))] = _variable_to_data_array(scipp_obj)
    elif su.is_data_array(scipp_obj):
        inventory[scipp_obj.name] = scipp_obj
    elif isinstance(scipp_obj, dict):
        for key in scipp_obj.keys():
            if su.is_variable(scipp_obj[key]):
                inventory[key] = _variable_to_data_array(scipp_obj[key])
            elif su.is_data_array(scipp_obj[key]):
                inventory[key] = scipp_obj[key]
            else:
                _raise_plot_input_error(type(scipp_obj[key]))
    else:
        _raise_plot_input_error(type(scipp_obj))
    return inventory


def plot(scipp_obj,
         projection=None,
         axes=None,
         color=None,
         marker=None,
         linestyle=None,
         linewidth=None,
         bins=None,
         **kwargs):
    """
    Wrapper function to plot any kind of scipp object.
    """

    inventory = _parse_input(scipp_obj)

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
            if ndims == 1 or projection == "1d" or projection == "1D":
                # Construct a key from the dimensions
                if axes is not None:
                    key = str(list(axes.values())[0])
                else:
                    key = str(var.dims[0])
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

            if key not in tobeplotted.keys():
                tobeplotted[key] = dict(ndims=ndims,
                                        scipp_obj_dict=dict(),
                                        axes=axes,
                                        mpl_line_params=dict())
                for n in mpl_line_params.keys():
                    tobeplotted[key]["mpl_line_params"][n] = {}
            tobeplotted[key]["scipp_obj_dict"][name] = inventory[name]
            for n, p in mpl_line_params.items():
                tobeplotted[key]["mpl_line_params"][n][name] = p

    # Plot all the subsets
    output = Plot()
    for key, val in tobeplotted.items():
        output[key] = dispatch(scipp_obj_dict=val["scipp_obj_dict"],
                               name=key,
                               ndim=val["ndims"],
                               projection=projection,
                               axes=val["axes"],
                               mpl_line_params=val["mpl_line_params"],
                               bins=bins,
                               **kwargs)
    return output
