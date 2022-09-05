# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from typing import Union, Dict
from .model import input_node
from .figure import Figure
from ... import Variable, DataArray, Dataset, arange
from ...typing import VariableLike
import numpy as np
import inspect


def _to_data_array(obj):
    out = obj
    if isinstance(out, np.ndarray):
        dims = [f"axis-{i}" for i in range(len(out.shape))]
        out = Variable(dims=dims, values=out)
    if isinstance(out, Variable):
        out = DataArray(data=out)
    out = out.copy(deep=False)
    for dim, size in out.sizes.items():
        if dim not in out.meta:
            out.coords[dim] = arange(dim, size)
    return out


def plot(obj: Union[VariableLike, Dict[str, VariableLike]],
         aspect: str = 'auto',
         cbar: bool = True,
         crop: Dict[str, Dict[str, Variable]] = None,
         errorbars: bool = True,
         grid: bool = False,
         mask_color: str = 'black',
         norm: str = 'linear',
         scale: Dict[str, str] = None,
         title: str = "",
         vmin: Variable = None,
         vmax: Variable = None,
         **kwargs) -> Figure:
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
        Aspect ratio for the axes.
    cbar:
        Show colorbar in 2d plots if `True`.
    crop:
        Set the axis limits. Limits should be given as a dict with one entry per
        dimension to be cropped. Each entry should be a nested dict containing scalar
        values for `'min'` and/or `'max'`. Example:
        `da.plot(crop={'time': {'min': 2 * sc.Unit('s'), 'max': 40 * sc.Unit('s')}})`
    errorbars:
        Show errorbars in 1d plots if `True`.
    grid:
        Show grid if `True`.
    mask_color:
        Color of masks in 1d plots.
    norm:
        Set to 'log' for a logarithmic y-axis (1d plots) or logarithmic colorscale
        (2d plots).
    scale:
        Change axis scaling between `log` and `linear`. For example, specify
        `scale={'tof': 'log'}` if you want log-scale for the `tof` dimension.
    title:
        The figure title.
    vmin:
        Lower bound for data to be displayed (y-axis for 1d plots, colorscale for
        2d plots).
    vmax:
        Upper bound for data to be displayed (y-axis for 1d plots, colorscale for
        2d plots).
    **kwargs:
        All other kwargs are directly forwarded to Matplotlib, the underlying plotting
        library. The underlying functions called are the following:
        - 1d data with a non bin-edge coordinate: `plot`
        - 1d data with a bin-edge coordinate: `step`
        - 2d data: `pcolormesh`

    Returns
    -------
    :
        A figure.
    """
    _, _, _, listed_args = inspect.getargvalues(inspect.currentframe())
    all_args = {
        **{k: v
           for k, v in listed_args.items() if k not in ('obj', 'kwargs')},
        **kwargs
    }
    if isinstance(obj, (dict, Dataset)):
        to_plot = {key: _to_data_array(item) for key, item in obj.items()}
        nodes = [input_node(v) for v in to_plot.values()]
        return Figure(*nodes, **all_args)
    else:
        return Figure(input_node(_to_data_array(obj)), **all_args)
