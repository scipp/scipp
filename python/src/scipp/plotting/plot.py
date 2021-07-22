# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .._scipp import core as sc
from .. import typing
from ..compat.dict import from_dict
from .dispatch import dispatch
from .objects import PlotDict
from .tools import get_line_param
import numpy as np
import itertools


def _ndarray_to_variable(ndarray):
    """
    Convert a numpy ndarray to a Variable.
    Fake dimension labels begin at 'x' and cycle through the alphabet.
    """
    dims = [f"axis-{i}" for i in range(len(ndarray.shape))]
    return sc.Variable(dims=dims, values=ndarray)


def _make_plot_key(plt_key, all_keys):
    if plt_key in all_keys:
        key_gen = (f'{plt_key}_{i}' for i in itertools.count(1))
        plt_key = next(x for x in key_gen if x not in all_keys)
    return plt_key


def _brief_str(obj):
    sizes = ', '.join([f'{dim}: {size}' for dim, size in obj.sizes.items()])
    return f'scipp.{type(obj).__name__}({sizes})'


def _input_to_data_array(item, all_keys, key=None):
    """
    Convert an input for the plot function to a DataArray or a dict of
    DataArrays.
    """
    to_plot = {}
    if isinstance(item, sc.Dataset):
        for name in sorted(item.keys()):
            if typing.has_numeric_type(item[name]):
                proto_plt_key = f'{key}_{name}' if key else name
                to_plot[_make_plot_key(proto_plt_key, all_keys)] = item[name]
    elif isinstance(item, sc.Variable):
        if typing.has_numeric_type(item):
            if key is None:
                key = _brief_str(item)
            to_plot[_make_plot_key(key, all_keys)] = sc.DataArray(data=item)
    elif isinstance(item, sc.DataArray):
        if typing.has_numeric_type(item):
            if key is None:
                key = item.name
            to_plot[_make_plot_key(key, all_keys)] = item
    elif isinstance(item, np.ndarray):
        if key is None:
            key = str(type(item))
        to_plot[_make_plot_key(
            key, all_keys)] = sc.DataArray(data=_ndarray_to_variable(item))
    else:
        raise RuntimeError("plot: Unknown input type: {}. Allowed inputs are "
                           "a Dataset, a DataArray, a Variable (and their "
                           "respective views), a numpy ndarray, and a dict of "
                           "Variables, DataArrays or ndarrays".format(type(item)))
    return to_plot


def plot(scipp_obj,
         projection=None,
         color=None,
         marker=None,
         linestyle=None,
         linewidth=None,
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
            inventory.update(
                _input_to_data_array(from_dict(scipp_obj), all_keys=inventory.keys()))
        except:  # noqa: E722
            for key, item in scipp_obj.items():
                inventory.update(
                    _input_to_data_array(item, all_keys=inventory.keys(), key=key))
    else:
        inventory.update(_input_to_data_array(scipp_obj, all_keys=inventory.keys()))

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
        if (ndims > 0) and (np.sum(var.shape) > 0):
            if ndims == 1 or projection == "1d" or projection == "1D":
                key = f"{var.dims}.{var.unit}"
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
                    mpl_line_params[n] = get_line_param(name=n,
                                                        index=mpl_line_params[n])

            if key not in tobeplotted.keys():
                tobeplotted[key] = dict(ndims=ndims,
                                        scipp_obj_dict=dict(),
                                        mpl_line_params=dict())
                for n in mpl_line_params.keys():
                    tobeplotted[key]["mpl_line_params"][n] = {}
            tobeplotted[key]["scipp_obj_dict"][name] = inventory[name]
            for n, p in mpl_line_params.items():
                tobeplotted[key]["mpl_line_params"][n][name] = p

    # Plot all the subsets
    output = PlotDict()
    for key, val in tobeplotted.items():
        output._items[key] = dispatch(scipp_obj_dict=val["scipp_obj_dict"],
                                      ndim=val["ndims"],
                                      projection=projection,
                                      mpl_line_params=val["mpl_line_params"],
                                      **kwargs)

    if len(output) > 1:
        return output
    elif len(output) > 0:
        return output._items[list(output.keys())[0]]
    else:
        raise ValueError("Input contains nothing that can be plotted. "
                         "Input may be of dtype vector or string, "
                         f"or may have zero dimensions:\n{scipp_obj}")
