# Dataset imports
from .dataset import *
import numpy as np
# Plotly imports
from plotly import __version__
from plotly.offline import download_plotlyjs, init_notebook_mode, plot, iplot
init_notebook_mode(connected=True)
from ipywidgets import interactive, HBox, VBox
import plotly.graph_objs as go


def plot_sliceviewer(input_data, field=None):

    dims = input_data.dimensions()
    if (len(dims) > 2) and (len(dims) < 5):

        # Get spatial extents by iterating over the coordinates, and also search
        # for data fields
        axmin = []
        axmax = []
        axlab = []
        fields = []
        nx = []
        ifield = 0
        for var in input_data:
            if var.is_coord:
                values = var.numpy
                axmin.append(values[0] - 0.5*(values[1]-values[0]))
                axmax.append(values[-1] + 0.5*(values[-1]-values[-2]))
                axlab.append([var.name, var.unit])
            elif var.is_data:
                fields.append([var.name, var.unit])
                if var.name == field:
                    ifield = len(fields) - 1

        ratio = (axmax[1] - axmin[1]) / (axmax[0] - axmin[0])

        if (field is None) and (len(fields) > 1):
            raise RuntimeError("More than one data field found! Please specify which one to display")

        a = input_data[Data.Value, fields[ifield][0]].numpy
        nx = np.shape(a)

        fig = go.FigureWidget(

            data = [go.Heatmap(
                z = a[:,:,0],
                colorscale = 'Viridis',
                colorbar=dict(
                    title="{} [{}]".format(fields[ifield][0],fields[ifield][1]),
                    titleside = 'right',
                    )
                )],

            layout = dict(
                autosize=False,
                width=800,
                height=800*ratio,
                xaxis = dict(
                    range = [axmin[0],axmax[0]],
                    title = "{} [{}]".format(axlab[0][0],axlab[0][1])),
                yaxis = dict(
                    scaleanchor = "x",
                    scaleratio = ratio,
                    range = [axmin[1],axmax[1]],
                    title = "{} [{}]".format(axlab[1][0],axlab[1][1]))
            )

        )

        def update_z(zpos):
            fig.data[0].z = a[:,:,zpos]

        # Add a slider that updates the slice plane
        slider = interactive(update_z, zpos=(0, nx[2]-1, 1))
        vb = VBox((fig, slider))
        vb.layout.align_items = 'center'
        return vb
    else:
        raise RuntimeError("Unsupported number of dimensions in sliceviewer.")


def plot_1d(input_data, field=None):

    dims = input_data.dimensions()
    if len(dims) == 1:

        # Get spatial extents by iterating over the coordinates, and also search
        # for data fields
        axlab = []
        coords = []
        fields = []
        ifield = 0
        for var in input_data:
            if var.is_coord:
                coords.append(var.numpy)
                axlab.append([var.name, var.unit])
            elif var.is_data:
                fields.append([var.name, var.unit])
                if var.name == field:
                    ifield = len(fields) - 1

        if (field is None) and (len(fields) > 1):
            raise RuntimeError("More than one data field found! Please specify which one to display")

        y = input_data[Data.Value, fields[ifield][0]].numpy

        trace0 = go.Scatter(
            x=coords[0],
            y=y
        )
        data = [trace0]

        layout = dict(
            xaxis = dict(
                title = "{} [{}]".format(axlab[0][0],axlab[0][1])
                ),
            yaxis = dict(
                title="{} [{}]".format(fields[ifield][0],fields[ifield][1])
                )
            )

        iplot(dict(data=[trace0], layout=layout))

    else:
        raise RuntimeError("Unsupported number of dimensions in plot_1d.")

