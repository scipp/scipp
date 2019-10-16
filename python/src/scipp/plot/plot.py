# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .._scipp import core as sc


def plot(input_data, collapse=None, backend=None, color=None, **kwargs):
    """
    Wrapper function to plot any kind of dataset
    """

    # Delayed imports
    from .tools import get_color
    from .plot_collapse import plot_collapse
    from .dispatch import dispatch

    # Create a list of variables which will then be dispatched to the plot_auto
    # function.
    # Search through the variables and group the 1D datasets that have
    # the same coordinate axis.
    # tobeplotted is a dict that holds pairs of
    # [number_of_dimensions, DatasetSlice], or
    # [number_of_dimensions, [List of DatasetSlices]] in the case of
    # 1d sc.Data.
    # TODO: 0D data is currently ignored -> find a nice way of
    # displaying it?
    tp = type(input_data)
    if tp is sc.DataProxy or tp is sc.DataArray:
        ds = sc.Dataset()
        ds[input_data.name] = input_data
        input_data = ds
    if tp is not list:
        input_data = [input_data]

    tobeplotted = dict()
    for ds in input_data:
        for name, var in ds:
            coords = var.coords
            ndims = len(coords)
            if ndims == 1:
                # Make a unique key from the dataset id in case there are more
                # than one dataset with 1D variables with the same coordinates
                key = "{}_{}".format(str(id(ds)), str(var.dims[0]))
                if key in tobeplotted.keys():
                    tobeplotted[key][1][name] = ds[name]
                else:
                    tobeplotted[key] = [ndims, {name: ds[name]}]
            elif ndims > 1:
                tobeplotted[name] = [ndims, ds[name]]

    # Plot all the subsets
    color_count = 0
    output = dict()
    for key, val in tobeplotted.items():
        if val[0] == 1:
            if color is None:
                color = []
                for l in val[1].keys():
                    color.append(get_color(index=color_count))
                    color_count += 1
            elif not isinstance(color, list):
                color = [color]
            name = None
        else:
            color = None
            name = key
        if collapse is not None:
            output[key] = plot_collapse(input_data=val[1], dim=collapse,
                                        name=name, backend=backend, **kwargs)
        else:
            output[key] = dispatch(input_data=val[1], ndim=val[0], name=name,
                                   backend=backend, color=color, **kwargs)

    if backend == "matplotlib":
        return output
    else:
        return
