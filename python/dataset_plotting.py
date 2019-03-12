# Dataset imports
from dataset import Data, dataset, dimensionCoord, coordDimension, sqrt, units
import numpy as np
import copy
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
    if (type(input_data) is dataset.Dataset) or \
       (type(input_data) is dataset.DatasetSlice):
        entries.append(input_data)
    # Case of a list of datasets
    elif type(input_data) is list:
        # Go through the list items:
        for item in input_data:
            if (type(item) is dataset.Dataset) or \
               (type(item) is dataset.DatasetSlice):
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
                raise RuntimeError("Can only plot 1D data with plot_1d. The "
                                   "input Dataset has {} dimensions. For 2D "
                                   "data, use plot_image. For higher "
                                   "dimensions, use plot_sliceviewer."
                                   .format(len(v[0].dimensions)))

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
# If plot=False, then not plot is produced, instead the data and layout dicts
# for plotly, as well as a transpose flag, are returned (this is used when
# calling plot_image from the sliceviewer).
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
            return data, layout, transpose

    else:
        raise RuntimeError("Unsupported number of dimensions in plot_image. "
                           "Expected at least 2 dimensions, got {}. To plot 1D "
                           "data, use plot_1d, and for 3 dimensions and higher,"
                           " use plot_sliceviewer. One can also call plot_image"
                           "for a Dataset with more than 2 dimensions, but one "
                           "must then use plot=False to collect the data and "
                           "layout dicts for plotly, as well as a transpose "
                           "flag, instead of plotting an image.".format(ndim))

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
              "on this system.")
        return

    # Check input dataset
    value_list, ndim = check_input(input_data)

    if ndim > 2:

        if axes is None:
            axes = [dimensionCoord(value_list[0].dimensions.labels[ndim-2]),
                    dimensionCoord(value_list[0].dimensions.labels[ndim-1])]

        # Use the machinery in plot_image to make the layout
        data, layout, transpose = plot_image(input_data, axes=axes, logcb=logcb,
                                             contours=contours, cb=cb,
                                             plot=False)

        # Create a SliceViewer object
        sv = SliceViewer(data=data, layout=layout, input_data=input_data,
                         axes=axes, value_name=value_list[0].name,
                         transpose=transpose, logcb=logcb)

        if hasattr(sv, "vbox"):
            return sv.vbox
        else:
            return

    else:
        raise RuntimeError("Unsupported number of dimensions in "
                           "plot_sliceviewer. Expected at least 3 dimensions, "
                           "got {}. For 2D data, use plot_image, for 1D data, "
                           "use plot_1d.".format(ndim))

#===============================================================================

class SliceViewer:

    def __init__(self, data, layout, input_data, axes, value_name, transpose, logcb):

        # Delay import to here, as ipywidgets is not part of the base plotly package
        try:
            from plotly.graph_objs import FigureWidget
            from ipywidgets import VBox, HBox, IntSlider, Label
        except ImportError:
            print("Sorry, the sliceviewer requires ipywidgets which was not found "
                  "on this system.")
            return

        # Make a deep copy of the input data
        self.input_data = copy.deepcopy(input_data)

        # Convert coords to dimensions
        axes_dims = []
        for i in range(len(axes)):
            axes_dims.append(coordDimension(axes[i]))

        # We want to slice out everything that is not in axes
        self.dims = self.input_data.dimensions
        self.labels = self.dims.labels
        self.shapes = self.dims.shape
        self.slider_nx = [] # size of the coordinate array
        self.slider_dims = [] # coordinate variables for the sliders, e.g. d[Coord.X]
        self.slice_labels = [] # save dimensions tags for the sliders, e.g. Dim.X
        for idim in range(len(self.labels)):
            if self.labels[idim] not in axes_dims:
                self.slider_nx.append(self.shapes[idim])
                self.slider_dims.append(self.input_data[dimensionCoord(self.labels[idim])])
                self.slice_labels.append(self.labels[idim])
        # Store coordinates of dimensions that will be in sliders
        self.slider_x = []
        for dim in self.slider_dims:
           self.slider_x.append(dim.numpy)
        self.nslices = len(self.slice_labels)

        # Initialise Figure and VBox objects
        self.fig = FigureWidget(data=data, layout=layout)
        self.vbox = self.fig,

        # Initialise slider and label containers
        self.lab = []
        self.slider = []
        # Collect the remaining arguments
        self.value_name = value_name
        self.transpose = transpose
        self.logcb = logcb
        # Default starting index for slider
        indx = 0

        # Now begin loop to construct sliders
        for i in range(len(self.slider_nx)):
            # Add a label widget to display the value of the z coordinate
            self.lab.append(Label(value=str(self.slider_x[i][indx])))
            # Add an IntSlider to slide along the z dimension of the array
            self.slider.append(IntSlider(
                value=indx,
                min=0,
                max=self.slider_nx[i]-1,
                step=1,
                description="",
                continuous_update = True,
                readout=False
            ))
            # Add an observer to the slider
            self.slider[i].observe(self.update_slice, names="value")
            # Add coordinate name and unit
            title = Label(value=axis_label(self.slider_dims[i]))
            self.vbox += (HBox([title, self.slider[i], self.lab[i]]),)

        # Call update_slice once to make the initial image
        self.update_slice(0)
        self.vbox = VBox(self.vbox)
        self.vbox.layout.align_items = 'center'

        return

    # Define function to update slices
    def update_slice(self, change):
        # Slice out everything not in axes.
        # The dimensions to be sliced have been saved in slice_labels
        # Slice with first element to avoid modifying underlying dataset
        self.lab[0].value = str(self.slider_x[0][self.slider[0].value])
        zarray = self.input_data[self.slice_labels[0], self.slider[0].value]
        # Then slice additional dimensions if needed
        for idim in range(1, self.nslices):
            self.lab[idim].value = str(self.slider_x[idim][self.slider[idim].value])
            zarray = zarray[self.slice_labels[idim], self.slider[idim].value]
        z = zarray[Data.Value, self.value_name].numpy
        # Check if we need to transpose the data
        if self.transpose:
            z = z.T
        if self.logcb:
            with np.errstate(invalid="ignore"):
                z = np.log10(z)
        self.fig.data[0].z = z
        return

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
