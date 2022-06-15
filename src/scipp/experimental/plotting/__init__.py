# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

# flake8: noqa E402, F401

import matplotlib.pyplot as plt

plt.ioff()

from typing import Union, Dict
from .plot import Plot
from .model import Node, show_graph, node, input_node
from .figure import Figure
from . import widgets
from ... import Variable, DataArray, Dataset, arange
from ...typing import VariableLike


def plot(obj: Union[VariableLike, Dict[str, VariableLike]]) -> Figure:
    """Plot a Scipp object.

    Parameters
    ----------
    obj:
        The object to be plotted.

    Returns
    -------
    :
        A figure.
    """
    if isinstance(obj, Variable):
        obj = DataArray(
            data=obj,
            coords={dim: arange(dim, size)
                    for dim, size in obj.sizes.items()})
    if isinstance(obj, DataArray):
        return Figure(input_node(obj))
    nodes = [input_node(v) for v in obj.values()]
    return Figure(*nodes)
