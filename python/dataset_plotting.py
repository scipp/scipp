# Dataset imports
from dataset import Data, dataset, dimensionCoord
import numpy as np
# Plotly imports
from plotly.offline import init_notebook_mode, iplot
init_notebook_mode(connected=True)
import plotly.graph_objs as go

#===============================================================================

# Plot a 2D slice through a 3D dataset with a slider to adjust the position of
# the slice in the third dimension
def plot_sliceviewer(input_data, field=None):

    # Delay import to here, as ipywidgets is not part of the base plotly package
    try:
        from ipywidgets import interactive, HBox, VBox
    except ImportError:
        print("Sorry, the sliceviewer requires ipywidgets which was not found on this system")
        return

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
#
# TODO: find a more general way of handling arguments to be sent to plotly,
# probably via a dictionay of arguments
def plot_1d(input_data, logx=False, logy=False, logxy=False):

    entries = []
    # Case of a single dataset
    if (type(input_data) is dataset.Dataset) or (type(input_data) is dataset.DatasetSlice):
        entries.append(input_data)
    # Case of a list of datasets
    elif type(input_data) is list:
        # Go through the list items:
        for item in input_data:
            if (type(item) is dataset.Dataset) or (type(item) is dataset.DatasetSlice):
                entries.append(item)
            else:
                raise RuntimeError("Bad data type in list input of plot_1d. "
                                   "Expected either Dataset or DatasetSlice, "
                                   "got " + type(item))
    else:
        raise RuntimeError("Bad data type in input of plot_1d. Expected either "
                           "Dataset or DatasetSlice, got " + type(item))

    # entries now contains a list of Dataset or DatasetSlice
    # We now construct a list of [x,y] pairs
    # TODO: check that all x coordinates are the same
    data = []
    for item in entries:
        # Scan the datasets
        values = []
        for var in item:
            if var.is_data:
                values.append(var)

        if len(values) > 1:
            raise RuntimeError("More than one Data.Value found! Please use e.g."
                               " plot(dataset.subset(Data.Value, 'sample')) to "
                               "select only a single Value.")

        # Check that data is 1D
        if len(values[0].dimensions.labels) > 1:
            raise RuntimeError("Can only plot 1D data with plot_1d.")

        # Define y
        y = values[0].numpy
        ylab = values[0].unit.name
        name = values[0].name
        ydims = values[0].dimensions
        ny = ydims.shape[0]

        # Define x
        coord = item[dimensionCoord(values[0].dimensions.labels[0])]
        xdims = coord.dimensions
        nx = xdims.shape[0]

        x = coord.numpy
        # Check for bin edges
        if nx == ny + 1:
            x = 0.5 * (x[1:] + x[:-1])
        xlab = "{} [{}]".format(coord.name,coord.unit)

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
    if logx or logxy:
        layout["xaxis"]["type"] = "log"
    if logy or logxy:
        layout["yaxis"]["type"] = "log"

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
