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


def plot(obj: Union[VariableLike, Dict[str, VariableLike]]) -> Figure:
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

    Returns
    -------
    :
        A figure.
    """
    if isinstance(obj, (dict, Dataset)):
        to_plot = {key: _to_data_array(item) for key, item in obj.items()}
        nodes = [input_node(v) for v in to_plot.values()]
        return Figure(*nodes)
    else:
        return Figure(input_node(_to_data_array(obj)))
