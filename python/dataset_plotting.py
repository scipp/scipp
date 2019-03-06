# Dataset imports
from dataset import Data, dataset, dimensionCoord, sqrt
import numpy as np
# Plotly imports
from plotly.offline import init_notebook_mode, iplot
import plotly.graph_objs as go
# Re-direct the output of init_notebook_mode to hide it from the unit tests
import io
from contextlib import redirect_stdout
try:
    with redirect_stdout(io.StringIO()):
        init_notebook_mode(connected=True)
except ImportError:
    print("Warning: the current version of this plotting module was designed to"
          " work inside a Jupyter notebook. Other usage has not been tested.")

# Wrapper function to dispatch the input dataset to the appropriate plotting
# function depending on its dimensions
def plot(input_data):
    ndim = len(input_data.dimensions())
    if ndim == 1:
        return plot_1d(input_data)
    elif ndim == 2:
        return plot_image(input_data)
    elif ndim < 5:
        return plot_sliceviewer(input_data)
    else:
        raise RuntimeError("Plot: unsupported number of dimensions: {}".format(ndim))

#===============================================================================

# Plot a 1D spectrum.
#
# Inputs can be either a Dataset(Slice) or a list of Dataset(Slice)s.
# If bars=True, then a bar plot is produced instead of a standard line.
#
# TODO: find a more general way of handling arguments to be sent to plotly,
# probably via a dictionay of arguments
def plot_1d(input_data, logx=False, logy=False, logxy=False, bars=False):

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

    if bars:
        func = go.Bar
    else:
        func = go.Scatter

    # entries now contains a list of Dataset or DatasetSlice
    # We now construct a list of [x,y] pairs
    # TODO: check that all x coordinates are the same
    data = []
    for item in entries:
        # Scan the datasets
        values = dict()
        variances = dict()
        for var in item:
            key = var.name
            if var.tag == Data.Variance:
                variances[key] = var
            elif var.is_data:
                values[key] = var
        # Now go through the values and see if they have an associated variance.
        # If they do, then use that as error bars.
        # Then go through the variances and check if there are some variances
        # that do not have an associate value; they are to be plotted as normal
        # data.
        tobeplotted = []
        for key, val in values.items():
            if key in variances.keys():
                vari = variances[key]
            else:
                vari = None
            tobeplotted.append([val, vari])
        for key, val in variances.items():
            if key not in values.keys():
                tobeplotted.append([val, None])

        # tobeplotted now contains pairs of [value, variance]
        for v in tobeplotted:

            # Check that data is 1D
            if len(v[0].dimensions.labels) > 1:
                raise RuntimeError("Can only plot 1D data with plot_1d.")

            # Define y
            y = v[0].numpy
            ylab = v[0].unit.name
            name = v[0].name
            # TODO: getting the shape of the dimension array is done in two steps
            # here because v.dimensions.shape[0] returns garbage. One of the
            # objects is going out of scope, we need to figure out which one to fix
            # this.
            ydims = v[0].dimensions
            ny = ydims.shape[0]

            # Define x
            coord = item[dimensionCoord(v[0].dimensions.labels[0])]
            xdims = coord.dimensions
            nx = xdims.shape[0]
            x = coord.numpy
            # Check for bin edges
            if nx == ny + 1:
                x = centers_to_edges(x)
            xlab = "{} [{}]".format(coord.name,coord.unit)

              # Define variance if present
            if v[1] is not None:
                trace = func(
                    x=x,
                    y=y,
                    name=name,
                    error_y=dict(
                        type='data',
                        array=sqrt(v[1]).numpy,
                        visible=True)
                    )
            else:
                trace = func(
                    x=x,
                    y=y,
                    name=name)

            data.append(trace)

    layout = dict(
        xaxis = dict(title = xlab),
        yaxis = dict(title = ylab))
    if logx or logxy:
        layout["xaxis"]["type"] = "log"
    if logy or logxy:
        layout["yaxis"]["type"] = "log"

    return iplot(dict(data=data, layout=layout))

#===============================================================================

# Plot a 2D image.
#
# If countours=True, a filled contour plot is produced, if False, then a
# standard image made of pixels is created.
# If plot=False, then not plot is produced, instead the layout and Data.Value
# variable are returned.
def plot_image(input_data, axes=None, contours=False, plot=True):

    ndim = len(input_data.dimensions())
    if axes is not None:
        naxes = len(axes)
    else:
        naxes = 0
    # TODO: this currently allows for plot_image to be called with a 3D dataset
    # and plot=False, which would lead to an error. We should think of a better
    # way to protect against this.
    if (ndim > 1) and (((ndim < 3) or ((ndim < 5) and not plot)) or (naxes == 2)):

        values = []
        coords = []
        for var in input_data:
            if var.is_coord:
                coords.append(var)
            elif var.is_data:
                values.append(var)

        if len(values) > 1:
            raise RuntimeError("More than one Data.Value found! Please use e.g."
                               " plot_image(dataset.subset('sample'))"
                               " to select only a single Value.")

        if axes is None:
            xcoord = input_data[dimensionCoord(values[0].dimensions.labels[0])]
            ycoord = input_data[dimensionCoord(values[0].dimensions.labels[1])]
        else:
            x_not_found = True
            y_not_found = True
            for c in coords:
                dimcoord = dimensionCoord(c.dimensions.labels[0])
                if dimcoord == axes[0]:
                    xcoord = input_data[dimcoord]
                    x_not_found = False
                if dimcoord == axes[1]:
                    ycoord = input_data[dimcoord]
                    y_not_found = False
            if x_not_found:
                raise RuntimeError("Requested x coordinate {} was not found".format(axes[0]))
            if y_not_found:
                raise RuntimeError("Requested y coordinate {} was not found".format(axes[1]))

        x = xcoord.numpy
        y = ycoord.numpy

        ratio = (np.amax(y) - np.amin(y)) / (np.amax(x) - np.amin(x))

        layout = dict(
            autosize=False,
            width=800,
            height=800*ratio,
            xaxis = dict(title = "{} [{}]".format(xcoord.name, xcoord.unit)),
            yaxis = dict(title = "{} [{}]".format(ycoord.name, ycoord.unit))
            )

        if plot:
            if contours:
                plot_type = 'contour'
            else:
                plot_type = 'heatmap'
            data = [dict(
                x = centers_to_edges(x),
                y = centers_to_edges(y),
                z = values[0].numpy,
                type = plot_type,
                colorscale = 'Viridis',
                colorbar=dict(
                    title="{} [{}]".format(values[0].name,values[0].unit),
                    titleside = 'right',
                    )
                )]
            return iplot(dict(data=data, layout=layout))
        else:
            return [values[0], layout]

    else:
        raise RuntimeError("Unsupported number of dimensions in plot_image.")

#===============================================================================

# Plot a 2D slice through a 3D dataset with a slider to adjust the position of
# the slice in the third dimension.
def plot_sliceviewer(input_data):

    # Delay import to here, as ipywidgets is not part of the base plotly package
    try:
        from ipywidgets import interactive, VBox
    except ImportError:
        print("Sorry, the sliceviewer requires ipywidgets which was not found "
              "on this system")
        return

    ndim = len(input_data.dimensions())
    if (ndim > 2) and (ndim < 5):

        # Use the machinery in plot_image to make the slices
        values, layout = plot_image(input_data, plot=False)

        a = values.numpy
        nx = np.shape(a)

        if ndim == 3:

            fig = go.FigureWidget(
                data = [go.Heatmap(
                    z = a[:,:,0],
                    colorscale = 'Viridis',
                    colorbar=dict(
                        title="{} [{}]".format(values.name,values.unit),
                        titleside = 'right',
                        )
                    )],
                layout = layout
            )

            def update_z(zpos):
                fig.data[0].z = a[:,:,zpos]

            # Add a slider that updates the slice plane
            # TODO: find a way to better name the 'zpos' text next to the slider
            slider = interactive(update_z, zpos=(0, nx[2]-1, 1))
            vb = VBox((fig, slider))
            vb.layout.align_items = 'center'
            return vb

        elif ndim == 4:

            fig = go.FigureWidget(
                data = [go.Heatmap(
                    z = a[:,:,0,0],
                    colorscale = 'Viridis',
                    colorbar=dict(
                        title="{} [{}]".format(values.name,values.unit),
                        titleside = 'right',
                        )
                    )],
                layout = layout
            )

            positions = {"i" : 0, "j" : 0}
            def update_slice():
                fig.data[0].z = a[:,:,positions["i"],positions["j"]]
            def update_i(ipos):
                positions["i"] = ipos
                update_slice()
            def update_j(jpos):
                positions["j"] = jpos
                update_slice()

            # Add a slider that updates the slice plane
            # TODO: find a way to better name the 'zpos' text next to the slider
            slider_i = interactive(update_i, ipos=(0, nx[2]-1, 1))
            slider_j = interactive(update_j, jpos=(0, nx[3]-1, 1))
            
            vb = VBox((fig, slider_i, slider_j))
            vb.layout.align_items = 'center'
            return vb

    else:
        raise RuntimeError("Unsupported number of dimensions in sliceviewer.")

#===============================================================================

def centers_to_edges(x):

    return 0.5 * (x[1:] + x[:-1])
