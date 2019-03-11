# Dataset imports
from dataset import Data, dataset, dimensionCoord, coordDimension, sqrt, units
import numpy as np
# Plotly imports
from plotly.offline import init_notebook_mode, iplot
# Re-direct the output of init_notebook_mode to hide it from the unit tests
import io
from contextlib import redirect_stdout
try:
    with redirect_stdout(io.StringIO()):
        init_notebook_mode(connected=True)
except ImportError:
    print("Warning: the current version of this plotting module was designed to"
          " work inside a Jupyter notebook. Other usage has not been tested.")

#===============================================================================

def check_input(input_data):

    values = []
    ndim = 0
    for var in input_data:
        if var.is_data:
            values.append(var)
            ndim = max(ndim, len(var.dimensions))

    if (len(values) > 1) and (ndim > 1):
        raise RuntimeError("More than one Data.Value found! Please use e.g."
                           " plot(dataset.subset[Data.Value, 'sample'])"
                           " to select only a single Value.")

    return values, ndim

#===============================================================================

# Wrapper function to dispatch the input dataset to the appropriate plotting
# function depending on its dimensions
def plot(input_data, **kwargs):

    # A list of datasets is only supported for 1d
    if type(input_data) is list:
        return plot_1d(input_data, **kwargs)
    # Case of a single dataset
    else:
        ndim = check_input(input_data)[1]
        if ndim == 1:
            return plot_1d(input_data, **kwargs)
        elif ndim == 2:
            return plot_image(input_data, **kwargs)
        else:
            return plot_sliceviewer(input_data, **kwargs)

#===============================================================================

# Plot a 1D spectrum.
#
# Inputs can be either a Dataset(Slice) or a list of Dataset(Slice)s.
# If bars=True, then a bar plot is produced instead of a standard line.
#
# TODO: find a more general way of handling arguments to be sent to plotly,
# probably via a dictionay of arguments
def plot_1d(input_data, logx=False, logy=False, logxy=False, bars=False,
            axes=None):

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
        plot_type = 'bar'
    else:
        plot_type = 'scatter'

    # entries now contains a list of Dataset or DatasetSlice
    # We now construct a list of [x,y] pairs
    data = []
    coord_check = None
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
            if len(v[0].dimensions) > 1:
                raise RuntimeError("Can only plot 1D data with plot_1d.")

            # Define y
            y = v[0].numpy
            name = axis_label(v[0])
            # TODO: getting the shape of the dimension array is done in two
            # steps here because v.dimensions.shape[0] returns garbage. One of
            # the objects is going out of scope, we need to figure out which one
            # to fix this.
            ydims = v[0].dimensions
            ny = ydims.shape[0]

            # Define x
            if axes is None:
                axes = [dimensionCoord(v[0].dimensions.labels[0])]
            coord = item[axes[0]]
            xdims = coord.dimensions
            nx = xdims.shape[0]
            x = coord.numpy
            # Check for bin edges
            if nx == ny + 1:
                x = edges_to_centers(x)
            xlab = axis_label(coord)
            if (coord_check is not None) and (coord.tag != coord_check):
                raise RuntimeError("All Value fields must have the same "
                                   "x-coordinate axis in plot_1d.")
            else:
                coord_check = coord.tag

            # Define trace
            trace = dict(
                    x = x,
                    y = y,
                    name = name,
                    type = plot_type)
            # Include variance if present
            if v[1] is not None:
                trace["error_y"] = dict(
                    type='data',
                    array=sqrt(v[1]).numpy,
                    visible=True)

            data.append(trace)

    layout = dict(
        xaxis = dict(title = xlab),
        yaxis = dict(),
        showlegend=True,
        legend=dict(x=0.0, y=1.15, orientation="h")
        )
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
# If plot=False, then not plot is produced, instead the layout, Data.Value and
# a transpose flag are returned (this is used when calling plot_image from the
# sliceviewer).
def plot_image(input_data, axes=None, contours=False, logcb=False, cb='Viridis',
               plot=True):

    values, ndim = check_input(input_data)

    if axes is not None:
        naxes = len(axes)
    else:
        naxes = 0

    # TODO: this currently allows for plot_image to be called with a 3D dataset
    # and plot=False, which would lead to an error. We should think of a better
    # way to protect against this.
    if ((ndim > 1) and (ndim < 3)) or ((not plot) and (naxes == 2)):

        # Note the order of the axes here: the outermost [1] dimension is the
        # fast dimension and is plotted along x, while the inner (slow)
        # dimension [0] is plotted along y.
        if axes is None:
            axes = [dimensionCoord(values[0].dimensions.labels[0]),
                    dimensionCoord(values[0].dimensions.labels[1])]

        xcoord = input_data[axes[1]]
        ycoord = input_data[axes[0]]

        x = xcoord.numpy
        y = ycoord.numpy

        # Check for bin edges
        # TODO: find a better way to handle edges. Currently, we convert from
        # edges to centers and then back to edges afterwards inside the plotly
        # object. This is not optimal and could lead to precision loss issues.
        zdims = values[0].dimensions
        nz = zdims.shape
        ydims = ycoord.dimensions
        ny = ydims.shape
        xdims = xcoord.dimensions
        nx = xdims.shape
        if nx[0] == nz[ndim-1] + 1:
            x = edges_to_centers(x)
        if ny[0] == nz[ndim-2] + 1:
            y = edges_to_centers(y)

        if contours:
            plot_type = 'contour'
        else:
            plot_type = 'heatmap'

        title = values[0].name
        if logcb:
            title = "log\u2081\u2080(" + title + ")"
        if values[0].unit != units.dimensionless:
            title += " [{}]".format(values[0].unit)

        data = [dict(
            x = centers_to_edges(x),
            y = centers_to_edges(y),
            z = [0.0],
            type = plot_type,
            colorscale = cb,
            colorbar=dict(
                title=title,
                titleside = 'right',
                )
            )]

        layout = dict(
            xaxis = dict(title = axis_label(xcoord)),
            yaxis = dict(title = axis_label(ycoord))
            )

        # Check if dimensions of arrays agree, if not, plot the transpose
        zlabs = zdims.labels
        if (zlabs[0] == xdims.labels[0]) and (zlabs[1] == ydims.labels[0]):
            transpose = True
        else:
            transpose = False

        if plot:
            z = values[0].numpy
            if transpose:
                z = z.T
            if logcb:
                with np.errstate(invalid="ignore"):
                    z = np.log10(z)
            data[0]["z"] = z
            return iplot(dict(data=data, layout=layout))
        else:
            return data, layout, values[0], transpose

    else:
        raise RuntimeError("Unsupported number of dimensions in plot_image.")

#===============================================================================

# Plot a 2D slice through a 3D dataset with a slider to adjust the position of
# the slice in the third dimension.
def plot_sliceviewer(input_data, axes=None, contours=False, logcb=False,
                     cb='Viridis'):

    # Delay import to here, as ipywidgets is not part of the base plotly package
    try:
        from plotly.graph_objs import FigureWidget
        from ipywidgets import VBox, HBox, IntSlider, Label
    except ImportError:
        print("Sorry, the sliceviewer requires ipywidgets which was not found "
              "on this system")
        return

    # Check input dataset
    value_list, ndim = check_input(input_data)

    if ndim > 2:

        if axes is None:
            axes = [dimensionCoord(value_list[0].dimensions.labels[ndim-2]),
                    dimensionCoord(value_list[0].dimensions.labels[ndim-1])]

        # Convert coords to dimensions
        axes_dims = []
        for i in range(len(axes)):
            axes_dims.append(coordDimension(axes[i]))

        # Define starting index for slider
        indx = 0

        # We want to slice out everything that is not in axes
        dims = input_data.dimensions
        labels = dims.labels
        shapes = dims.shape
        slider_nx = [] # size of the coordinate array
        slider_dims = [] # coordinate variables for the sliders, e.g. d[Coord.X]
        slice_labels = [] # save dimensions tags for the sliders, e.g. Dim.X
        for idim in range(len(labels)):
            if labels[idim] not in axes_dims:
                slider_nx.append(shapes[idim])
                slider_dims.append(input_data[dimensionCoord(labels[idim])])
                slice_labels.append(labels[idim])
        # Store coordinates of dimensions that will be in sliders
        slider_x = []
        for dim in slider_dims:
           slider_x.append(dim.numpy)
        nslices = len(slice_labels)

        # Use the machinery in plot_image to make the layout
        data, layout, values, transpose = plot_image(input_data, axes=axes,
                                          contours=contours, logcb=logcb,
                                          cb=cb, plot=False)

        # Create a figure widget
        fig = FigureWidget(data=data, layout=layout)
        # Initialise a tuple for the VBox
        vb = fig,
        # Define containers for sliders
        lab = []
        slider = []
        updates = []
        # Define function to update slices
        def update_slice(change):
            zarray = input_data
            dims = input_data.dimensions
            labels = dims.labels
            # Once again, slice out everything not in axes.
            # The dimensions to be sliced have been saved in slice_labels
            for idim in range(nslices):
                lab[idim].value = str(slider_x[idim][slider[idim].value])
                zarray = zarray[slice_labels[idim], slider[idim].value]
            fig.data[0].z = zarray[Data.Value, value_list[0].name].numpy
            # If dimensions don't match, transpose the data
            if transpose:
                fig.data[0].z = fig.data[0].z.T
            if logcb:
                with np.errstate(invalid="ignore"):
                    fig.data[0].z = np.log10(fig.data[0].z)

        # Now begin loop to construct sliders
        for i in range(len(slider_nx)):
            # Add a label widget to display the value of the z coordinate
            lab.append(Label(value=str(slider_x[i][indx])))
            # Add an IntSlider to slide along the z dimension of the array
            slider.append(IntSlider(
                value=indx,
                min=0,
                max=slider_nx[i]-1,
                step=1,
                description="",
                continuous_update = True,
                readout=False
            ))
            # Add an observer to the slider
            slider[i].observe(update_slice, names="value")
            # Add coordinate name and unit
            title = Label(value=axis_label(slider_dims[i]))
            vb += (HBox([title, slider[i], lab[i]]),)

        # Update the slice for initial load of figure
        update_slice(0)
        vb = VBox(vb)
        vb.layout.align_items = 'center'
        return vb

    else:
        raise RuntimeError("Unsupported number of dimensions in sliceviewer.")

#===============================================================================

# Convert coordinate edges to centers
def edges_to_centers(x):
    return 0.5 * (x[1:] + x[:-1])

# Convert coordinate centers to edges
def centers_to_edges(x):
    e = edges_to_centers(x)
    return np.concatenate([[2.0*x[0]-e[0]],e,[2.0*x[-1]-e[-1]]])

# Make an axis label with "Name [unit]"
def axis_label(var):
    if var.is_coord:
        label = "{}".format(var.tag)
        label = label.replace("Coord.","")
    else:
        label = "{}".format(var.name)
    if var.unit != units.dimensionless:
        label += " [{}]".format(var.unit)
    return label
