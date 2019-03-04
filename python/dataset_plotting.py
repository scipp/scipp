# Dataset imports
from dataset import Data, dataset
import numpy as np
# Plotly imports
from plotly import __version__
from plotly.offline import download_plotlyjs, init_notebook_mode, plot, iplot
init_notebook_mode(connected=True)
from ipywidgets import interactive, HBox, VBox
import plotly.graph_objs as go

#===============================================================================

# Plot a 2D slice through a 3D dataset with a slider to adjust the position of
# the slice in the third dimension
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

#===============================================================================

# Plot a 1D spectrum
# Inputs can be:
# 1. d (=dataset)
# 2. d, var (=string)
# 3. [d1, d2]
# 4. [[d1, var1], [d2, var2]]
# 5. [[d1, var1], [d2]]
def plot_1d(input_data, var=None):

    arr = []
    # Case of a single dataset
    if type(input_data) is dataset.Dataset:
        arr.append([input_data, var])
    # The more general case: there can be many different types of lists
    elif type(input_data) is list:
        # Go through the list items recursively:
        for item in input_data:
            if type(item) is dataset.Dataset:
                arr.append([item, None])
            elif type(item) is list:
                ktem = [None, None]
                for jtem in item:
                    if type(jtem) is dataset.Dataset:
                        ktem[0] = jtem
                    elif type(jtem) is str:
                        ktem[1] = jtem
                    else:
                        raise RuntimeError("Bad data type in input of plot_1d")
                arr.append(ktem)

    # arr now contains a list of [dataset,var] pairs
    # We now construct a list of [x,y] pairs
    # TODO: check that all x coordinates are the same
    data = []
    for item in arr:
        # Scan the datasets
        coords = []
        fields = []
        for var in item[0]:
            if var.is_coord:
                coords.append(var)
            if var.is_data:
                fields.append(var)
                if var.name == item[1]:
                    ifield = len(fields) - 1

        if (item[1] is None) and (len(fields) > 1):
            raise RuntimeError("More than one data field found! Please specify which one to display")

        y = fields[ifield].numpy
        ylab = fields[ifield].unit.name
        name = fields[ifield].name

        # Now find the appropriate coordinate
        coord_not_found = True
        for c in coords:
            if c.dimensions == fields[ifield].dimensions:
                x = c.numpy
                xlab = "{} [{}]".format(c.name,c.unit)
                coord_not_found = False
        if coord_not_found:
            raise RuntimeError("The required coordinate was not found in the dataset")

        trace = go.Scatter(
            x=x,
            y=y,
            name=name
        )
        data.append(trace)

    layout = dict(
        xaxis = dict(
            title = xlab
            ),
        yaxis = dict(
            title = ylab
            )
        )

    iplot(dict(data=data, layout=layout))

    return

#===============================================================================

# Plot a 2D image of pixels
def plot_image(input_data, field=None):
    dims = input_data.dimensions()
    if (len(dims) > 1) and (len(dims) < 3):

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

        data = [go.Heatmap(
            z = input_data[Data.Value, fields[ifield][0]].numpy,
            colorscale = 'Viridis',
            colorbar=dict(
                title="{} [{}]".format(fields[ifield][0],fields[ifield][1]),
                titleside = 'right',
                )
            )]

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

        iplot(dict(data=data, layout=layout))

    else:
        raise RuntimeError("Unsupported number of dimensions in plot_image.")
