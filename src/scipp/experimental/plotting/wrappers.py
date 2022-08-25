# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from typing import Union, Dict
from .model import input_node
from .figure import Figure
from ... import Variable, DataArray, Dataset, arange
from ...typing import VariableLike
import numpy as np


def _to_data_array(obj):
    out = obj
    if isinstance(out, np.ndarray):
        dims = [f"axis-{i}" for i in range(len(out.shape))]
        out = Variable(dims=dims, values=out)
    if isinstance(out, Variable):
        out = DataArray(data=out)
    for dim, size in out.sizes.items():
        if dim not in out.meta:
            out.coords[dim] = arange(dim, size)
    return out


def plot(obj: Union[VariableLike, Dict[str, VariableLike]], **kwargs) -> Figure:
    """Plot a Scipp object.

    Parameters
    ----------
    obj:
        The object to be plotted. Possible inputs are:
        - Variable
        - Dataset
        - DataArray
        - numpy ndarray
        - dict of Variables
        - dict of DataArrays
        - dict of numpy ndarrays
    aspect:
        Aspect ratio for the axes. Default is `'auto'`.
    cbar:
        Show colorbar in 2d plots if `True`. Default is `True`.
    errorbars:
        Show errorbars in 1d plots if `True`. Default is `True`.
    grid:
        Show grid if `True`. Default is `False`.
    mask_color:
        Color of masks in 1d plots.
    norm:
        Set to 'log' for a logarithmic y-axis (1d plots) or logarithmic colorscale
        (2d plots). Default is 'linear'.
    scale:
        Change axis scaling between `log` and `linear`. For example, specify
        `scale={'tof': 'log'}` if you want log-scale for the `tof` dimension.
    title:
        The figure title.
    vmin:
        Lower bound for data to be displayed (y-axis for 1d plots, colorscale for
        2d plots). Default is `None`.
    vmax:
        Upper bound for data to be displayed (y-axis for 1d plots, colorscale for
        2d plots). Default is `None`.
    **kwargs:
        All other kwargs are directly forwarded to the underlying plotting library
        (Matplotlib).

    Returns
    -------
    :
        A figure.
    """
    if isinstance(obj, (dict, Dataset)):
        to_plot = {key: _to_data_array(item) for key, item in obj.items()}
        nodes = [input_node(v) for v in to_plot.values()]
        return Figure(*nodes, **kwargs)
    else:
        return Figure(input_node(_to_data_array(obj)), **kwargs)
