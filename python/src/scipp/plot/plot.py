# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .._scipp import core as sc
from .. import _utils as su
from ..compat.dict import from_dict
from .dispatch import dispatch
from .helpers import Plot
from .tools import make_fake_coord, get_line_param
import numpy as np


def _variable_to_data_array(variable):
    """
    Convert a Variable to a DataArray by giving it some fake integer
    coordinates, which makes it possible to write generic code in the rest of
    the plotting library.
    """
    coords = {}
    for dim, size in zip(variable.dims, variable.shape):
        coords[dim] = make_fake_coord(dim, size)
    return sc.DataArray(data=variable, coords=coords)


def _ndarray_to_variable(ndarray):
    """
    Convert a numpy ndarray to a Variable.
    Fake dimension labels begin at 'x' and cycle through the alphabet.
    """
    dims = [f"axis-{i}" for i in range(len(ndarray.shape))]
    return sc.Variable(dims=dims, values=ndarray)


def _input_to_data_array(item, key=None):
    """
    Convert an input for the plot function to a DataArray or a dict of
    DataArrays.
    """
    to_plot = {}
    if su.is_dataset(item):
        for name in sorted(item.keys()):
            to_plot[name] = item[name]
    elif su.is_variable(item):
        if key is None:
            key = str(type(item))
        to_plot[key] = _variable_to_data_array(item)
    elif su.is_data_array(item):
        if key is None:
            key = item.name
        to_plot[key] = item
    elif isinstance(item, np.ndarray):
        if key is None:
            key = str(type(item))
        to_plot[key] = _variable_to_data_array(_ndarray_to_variable(item))
    else:
        raise RuntimeError("plot: Unknown input type: {}. Allowed inputs are "
                           "a Dataset, a DataArray, a Variable (and their "
                           "respective views), a numpy ndarray, and a dict of "
                           "Variables, DataArrays or ndarrays".format(
                               type(item)))
    return to_plot


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
    Wrapper function to plot a scipp object.

    Possible inputs are:
      - Variable
      - Dataset
      - DataArray
      - numpy ndarray
      - dict of Variables
      - dict of DataArrays
      - dict of numpy ndarrays
      - dict that can be converted to a Scipp object via `from_dict`

    1D Variables are grouped onto the same axes if they have the same dimension
    and the same unit.

    Any other Variables are displayed in their own figure.

    Returns a Plot object which can be displayed in a Jupyter notebook.
    """

    # Decompose the input and return a dict of DataArrays.
    inventory = {}
    if isinstance(scipp_obj, dict):
        try:
            inventory.update(_input_to_data_array(from_dict(scipp_obj)))
        except:  # noqa: E722
            for key, item in scipp_obj.items():
                inventory.update(_input_to_data_array(item, key=key))
    else:
        inventory.update(_input_to_data_array(scipp_obj))

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
        ndims = len(var.dims)
        if ndims > 0:
            if ndims == 1:
                # Construct a key from the dimensions
                if axes is not None:
                    key = list(axes.values())[0]
                else:
                    key = var.dims[0]
                # Add unit to key
                key = "{}.{}".format(key, str(var.unit))
            else:
                key = name
            if ndims == 1 or projection == "1d" or projection == "1D":
                line_count += 1

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

    if len(output) > 1:
        return output
    else:
        return output[list(output.keys())[0]]
