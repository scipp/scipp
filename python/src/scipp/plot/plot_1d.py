# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from ..tools import edges_to_centers, axis_label
from . import config
from .plot_tools import render_plot

# Other imports
import numpy as np
import plotly.graph_objs as go


def plot_1d(input_data, backend=None, logx=False, logy=False, logxy=False,
            axes=None, color=None, filename=None):
    """
    Plot a 1D spectrum.

    Input is a dictionary containing a list of DataProxy.
    If the coordinate of the x-axis contains bin edges, then a bar plot is
    made.

    TODO: find a more general way of handling arguments to be sent to plotly,
    probably via a dictionay of arguments
    """

    data = []
    color_count = 0
    for name, var in input_data.items():
        xcoord = var.coords[var.dims[0]]
        x = xcoord.values
        xlab = axis_label(xcoord)
        y = var.values
        ylab = axis_label(var=var, name=name)

        nx = x.shape[0]
        ny = y.shape[0]
        histogram = False
        if nx == ny + 1:
            histogram = True

        # Define trace
        trace = dict(x=x, y=y, name=ylab, type='scattergl')
        if histogram:
            trace["line"] = {"shape": 'hv'}
            trace["y"] = np.concatenate((trace["y"], [0.0]))
            trace["fill"] = 'tozeroy'
        if color is not None:
            trace["marker"] = {"color": color[color_count]}
        # Include variance if present
        if var.variances is not None:
            err_dict = dict(
                    type='data',
                    array=np.sqrt(var.variances),
                    visible=True,
                    color=color[color_count])
            if histogram:
                trace2 = dict(x=edges_to_centers(x), y=y, showlegend=False,
                              type='scattergl', mode='markers',
                              error_y=err_dict,
                              marker={"color": color[color_count]})
                data.append(trace2)
            else:
                trace["error_y"] = err_dict

        data.append(trace)
        color_count += 1

    layout = dict(
        xaxis=dict(title=xlab),
        yaxis=dict(),
        showlegend=True,
        legend=dict(x=0.0, y=1.15, orientation="h"),
        height=config.height
    )
    if histogram:
        layout["barmode"] = "overlay"
    if logx or logxy:
        layout["xaxis"]["type"] = "log"
    if logy or logxy:
        layout["yaxis"]["type"] = "log"

    fig = go.Figure(data=data, layout=layout)
    render_plot(static_fig=fig, interactive_fig=fig, backend=backend,
                filename=filename)
    # if filename is not None:
    #     if filename.endswith(".html"):
    #         write_html(fig=fig, file=filename, auto_open=False)
    #     else:
    #         write_image(fig=fig, file=filename)
    # else:
    #     # display(fig)
    #     # Image(pio.to_image(fig, format='png'))
    #     # display(Image(to_image(fig, format='png')))
    #     display_figure(static_fig=fig, interactive_fig=fig,
    #                    backend=backend)
    return
