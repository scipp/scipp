# Dataset imports
from dataset import Dataset, Data, dataset, dimensionCoord, coordDimension, sqrt, units
import numpy as np
import copy
# Plotly imports
from plotly.offline import init_notebook_mode, iplot
from plotly.tools import make_subplots
# Re-direct the output of init_notebook_mode to hide it from the unit tests
import io
from contextlib import redirect_stdout
try:
    with redirect_stdout(io.StringIO()):
        init_notebook_mode(connected=True)
except ImportError:
    print("Warning: the current version of this plotting module was designed to"
          " work inside a Jupyter notebook. Other usage has not been tested.")
# Delay import to here, as ipywidgets is not part of plotly
try:
    from plotly.graph_objs import FigureWidget
    from ipywidgets import VBox, HBox, IntSlider, Label
except ImportError:
    raise RuntimeError("Sorry, the sliceviewer requires ipywidgets which was not "
          "found on this system.")
from plotly.colors import DEFAULT_PLOTLY_COLORS

#===============================================================================

# Some global configuration
default = {
    "cb" : { "name" : "Viridis", "log" : False, "min" : None, "max" : None }
}

#===============================================================================

def check_input(input_data, check_multiple_values=True):

    values = []
    ndims = []
    for var in input_data:
        if var.is_data and (var.tag != Data.Variance):
            values.append(var)
            ndims.append(len(var.dimensions))

    if check_multiple_values and (len(values) > 1) and (np.amax(ndims) > 1):
        raise RuntimeError("More than one Data.Value found! Please use e.g."
                           " plot(dataset.subset[Data.Value, 'sample'])"
                           " to select only a single Value.")

    return values, ndims

#===============================================================================

# Wrapper function to plot any kind of dataset
def plot(input_data, axes=None, waterfall=None, collapse=None, **kwargs):

    # A list of datasets is only supported for 1d
    if type(input_data) is list:
        return plot_1d(input_data, axes=axes, **kwargs)
    # Case of a single dataset
    else:
        values, ndims = check_input(input_data, check_multiple_values=False)
        if len(values) > 1:
            # Search through the variables and group the 1D datasets that have the
            # same coordinate axis
            if np.amin(ndims) == 1:
                list_of_1d_variables = dict()
                list_of_2d_variables = []
                list_of_Nd_variables = []
                for i in range(len(values)):
                    if ndims[i] == 1:
                        dims = values[i].dimensions
                        labs = dims.labels
                        key = str(labs[0])
                        if key in list_of_1d_variables.keys():
                            list_of_1d_variables[key].append(input_data.subset[values[i].name])
                        else:
                            list_of_1d_variables[key] = [input_data.subset[values[i].name]]
                    elif ndims[i] == 2:
                        list_of_2d_variables.append(input_data.subset[values[i].name])
                    elif ndims[i] > 2:
                        list_of_Nd_variables.append(input_data.subset[values[i].name])

            # Start creating a large VBox to hold all the plots
            vbox = tuple()
            # Add 1D variables
            color_count = 0
            for key, val in list_of_1d_variables.items():
                data, layout = plot_1d(val, plot=False)
                for l in data:
                    l["marker"] = dict(color=DEFAULT_PLOTLY_COLORS[color_count%10])
                    color_count += 1
                vbox += FigureWidget(data=data, layout=layout),
            # Add 2D variables
            for var in list_of_2d_variables:
                data, layout = plot_image(var, plot=False)[:2]
                vbox += FigureWidget(data=data, layout=layout),
            # Add the remaining variables
            for var in list_of_Nd_variables:
                vbox += plot_sliceviewer(var),
            return VBox(vbox)
        else:
            return plot_auto(input_data, ndim=np.amax(ndims), axes=axes,
                             waterfall=waterfall, collapse=collapse, **kwargs)

#===============================================================================

# Function to automaticall dispatch the input dataset to the appropriate
# plotting function depending on its dimensions
def plot_auto(input_data, ndim=0, axes=None, waterfall=None, collapse=None, **kwargs):

    if ndim == 1:
        return plot_1d(input_data, axes=axes, **kwargs)
    elif ndim == 2:
        if collapse is not None:
            return plot_1d(plot_waterfall(input_data, dim=collapse, axes=axes, plot=False), **kwargs)
        elif waterfall is not None:
            return plot_waterfall(input_data, dim=waterfall, axes=axes, **kwargs)
        else:
            return plot_image(input_data, axes=axes, **kwargs)
    else:
        return plot_sliceviewer(input_data, axes=axes, **kwargs)

#===============================================================================

# Plot a 1D spectrum.
#
# Inputs can be either a Dataset(Slice) or a list of Dataset(Slice)s.
# If bars=True, then a bar plot is produced instead of a standard line.
#
# TODO: find a more general way of handling arguments to be sent to plotly,
# probably via a dictionay of arguments
def plot_1d(input_data, logx=False, logy=False, logxy=False, axes=None,
            plot=True):

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

            # Reset axes
            axes_copy = axes

            # Check that data is 1D
            if len(v[0].dimensions) > 1:
                raise RuntimeError("Can only plot 1D data with plot_1d. The "
                                   "input Dataset has {} dimensions. For 2D "
                                   "data, use plot_image. For higher "
                                   "dimensions, use plot_sliceviewer."
                                   .format(len(v[0].dimensions)))

            # Define y: try to see if array contains numbers
            try:
                y = v[0].numpy
            # If .numpy fails, try to extract as an array of strings
            except RuntimeError:
                y = np.array(v[0].data, dtype=np.str)
            name = axis_label(v[0])
            # TODO: getting the shape of the dimension array is done in two
            # steps here because v.dimensions.shape[0] returns garbage. One of
            # the objects is going out of scope, we need to figure out which one
            # to fix this.
            ydims = v[0].dimensions
            ny = ydims.shape[0]

            # Define x
            if axes_copy is None:
                axes_copy = [dimensionCoord(v[0].dimensions.labels[0])]
            coord = item[axes_copy[0]]
            xdims = coordinate_dimensions(coord)
            nx = xdims.shape[0]
            try:
                x = coord.numpy
            except RuntimeError:
                x = np.array(coord.data, dtype=np.str)
            # Check for bin edges
            histogram = False
            if nx == ny + 1:
                x, w = edges_to_centers(x)
                histogram = True
            xlab = axis_label(coord)
            if (coord_check is not None) and (coord.tag != coord_check):
                raise RuntimeError("All Value fields must have the same "
                                   "x-coordinate axis in plot_1d.")
            else:
                coord_check = coord.tag

            # Define trace
            trace = dict(x=x, y=y, name=name)
            if histogram:
                trace["type"] = 'bar'
                trace["marker"] = dict(opacity=0.6, line=dict(width=0))
                trace["width"] = w
            else:
                trace["type"] = 'scatter'
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
        showlegend = True,
        legend = dict(x=0.0, y=1.15, orientation="h"),
        barmode = 'overlay'
        )
    if logx or logxy:
        layout["xaxis"]["type"] = "log"
    if logy or logxy:
        layout["yaxis"]["type"] = "log"

    if plot:
        return iplot(dict(data=data, layout=layout))
    else:
        return data, layout

#===============================================================================

# Plot a 2D image.
#
# If countours=True, a filled contour plot is produced, if False, then a
# standard image made of pixels is created.
# If plot=False, then not plot is produced, instead the data and layout dicts
# for plotly, as well as a transpose flag, are returned (this is used when
# calling plot_image from the sliceviewer).
def plot_image(input_data, axes=None, contours=False, cb=None, plot=True):

    values, ndims = check_input(input_data)
    ndim = np.amax(ndims)

    if axes is not None:
        naxes = len(axes)
    else:
        naxes = 0

    # TODO: this currently allows for plot_image to be called with a 3D dataset
    # and plot=False, which would lead to an error. We should think of a better
    # way to protect against this.
    if ((ndim > 1) and (ndim < 3)) or ((not plot) and (naxes > 1)):

        # Note the order of the axes here: the outermost dimension is the
        # fast dimension and is plotted along x, while the inner (slow)
        # dimension is plotted along y.
        if axes is None:
            dims = values[0].dimensions
            labs = dims.labels
            axes = [ dimensionCoord(label) for label in labs ]
        else:
            axes = axes[-2:]

        # Get coordinates axes and dimensions
        xcoord, ycoord, x, y, xlabs, ylabs, zlabs = \
            process_dimensions(input_data, axes, values[0], ndim)

        if contours:
            plot_type = 'contour'
        else:
            plot_type = 'heatmap'

        # Parse colorbar
        cbar = parse_colorbar(cb)

        title = values[0].name
        if cbar["log"]:
            title = "log\u2081\u2080(" + title + ")"
        if values[0].unit != units.dimensionless:
            title += " [{}]".format(values[0].unit)

        data = [dict(
            x = centers_to_edges(x),
            y = centers_to_edges(y),
            z = [0.0],
            type = plot_type,
            colorscale = cbar["name"],
            colorbar=dict(
                title=title,
                titleside = 'right',
                )
            )]

        # Apply colorbar parameters
        cbcount = (cbar["min"] is not None) + (cbar["max"] is not None)
        if cbcount == 2:
            data[0]["zmin"] = cbar["min"]
            data[0]["zmax"] = cbar["max"]

        layout = dict(
            xaxis = dict(title = axis_label(xcoord)),
            yaxis = dict(title = axis_label(ycoord))
            )

        z = values[0].numpy
        # Check if dimensions of arrays agree, if not, plot the transpose
        if (zlabs[0] == xlabs[0]) and (zlabs[1] == ylabs[0]):
            z = z.T
        if cbar["log"]:
            with np.errstate(invalid="ignore", divide="ignore"):
                z = np.log10(z)
        if cbcount == 1:
            if cbar["min"] is not None:
                data[0]["zmin"] = cbar["min"]
            else:
                data[0]["zmin"] = np.amin(z[np.where(np.isfinite(z))])
            if cbar["max"] is not None:
                data[0]["zmax"] = cbar["max"]
            else:
                data[0]["zmax"] = np.amax(z[np.where(np.isfinite(z))])
        data[0]["z"] = z
        if plot:
            return iplot(dict(data=data, layout=layout))
        else:
            return data, layout, xlabs, ylabs, cbar

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

# Make a 3D waterfall plot
#
def plot_waterfall(input_data, dim=None, axes=None, plot=True):

    values, ndims = check_input(input_data)
    ndim = np.amax(ndims)

    if (ndim > 1) and (ndim < 3):

        if axes is None:
            dims = values[0].dimensions
            labs = dims.labels
            axes = [ dimensionCoord(label) for label in labs ]

        # Get coordinates axes and dimensions
        xcoord, ycoord, x, y, xlabs, ylabs, zlabs = \
            process_dimensions(input_data, axes, values[0], ndim)

        data = []
        z = values[0].numpy

        e = None
        # Check if we need to add variances to dataset list for collapse plot
        if not plot:
            for var in input_data:
                if (var.tag == Data.Variance) and (var.name == values[0].name):
                    e = var.numpy

        if (zlabs[0] == xlabs[0]) and (zlabs[1] == ylabs[0]):
            z = z.T
            zlabs = [ylabs[0], xlabs[0]]
            if e is not None:
                e = e.T

        pdict = dict(type = 'scatter3d', mode = 'lines', line = dict(width=5))
        adict = dict(z = 1)

        if dim is None:
            dim = zlabs[0]

        if dim == zlabs[0]:
            for i in range(len(y)):
                if plot:
                    idict = pdict.copy()
                    idict["x"] = x
                    idict["y"] = [y[i]] * len(x)
                    idict["z"] = z[i,:]
                    data.append(idict)
                    adict["x"] = 3
                    adict["y"] = 1
                else:
                    dset = Dataset()
                    dset[axes[1]] = input_data[axes[1]]
                    dims = dset[axes[1]].dimensions
                    labs = dims.labels
                    key = values[0].name + "_{}".format(i)
                    dset[Data.Value, key] = ([labs[0]], z[i,:])
                    if e is not None:
                        dset[Data.Variance, key] = ([labs[0]], e[i,:])
                    data.append(dset)
        elif dim == zlabs[1]:
            for i in range(len(x)):
                if plot:
                    idict = pdict.copy()
                    idict["x"] = [x[i]] * len(y)
                    idict["y"] = y
                    idict["z"] = z[:,i]
                    data.append(idict)
                    adict["x"] = 1
                    adict["y"] = 3
                else:
                    dset = Dataset()
                    dset[axes[0]] = input_data[axes[0]]
                    dims = dset[axes[0]].dimensions
                    labs = dims.labels
                    key = values[0].name + "_{}".format(i)
                    dset[Data.Value, key] = ([labs[0]], z[:,i])
                    if e is not None:
                        dset[Data.Variance, key] = ([labs[0]], e[:,i])
                    data.append(dset)
        else:
            raise RuntimeError("Something went wrong in plot_waterfall. The "
                               "waterfall dimension is not recognised.")

        if plot:
            layout = dict(
                scene = dict(
                    xaxis = dict(title = axis_label(xcoord)),
                    yaxis = dict(title = axis_label(ycoord)),
                    zaxis = dict(title = "{} [{}]".format(values[0].name, values[0].unit)),
                    aspectmode='manual',
                    aspectratio=adict
                    ),
                showlegend = False
                )
            return iplot(dict(data=data, layout=layout))
        else:
            return data

    else:
        raise RuntimeError("Unsupported number of dimensions in plot_waterfall."
                           " Expected at least 2 dimensions, got {}."
                           .format(ndim))

#===============================================================================

# Plot a 2D slice through a 3D dataset with a slider to adjust the position of
# the slice in the third dimension.
def plot_sliceviewer(input_data, axes=None, contours=False, cb=None):

    # Delay import to here, as ipywidgets is not part of the base plotly package
    try:
        from plotly.graph_objs import FigureWidget
        from ipywidgets import VBox, HBox, IntSlider, Label
    except ImportError:
        print("Sorry, the sliceviewer requires ipywidgets which was not found "
              "on this system.")
        return

    # Check input dataset
    value_list, ndims = check_input(input_data)
    ndim = np.amax(ndims)

    if ndim > 2:

        if axes is None:
            dims = value_list[0].dimensions
            labs = dims.labels
            axes = [ dimensionCoord(label) for label in labs ]

        # Use the machinery in plot_image to make the layout
        data, layout, xlabs, ylabs, cbar = plot_image(input_data, axes=axes,
                                                      contours=contours, cb=cb,
                                                      plot=False)

        # Create a SliceViewer object
        sv = SliceViewer(plotly_data=data, plotly_layout=layout,
                         input_data=input_data, axes=axes,
                         value_name=value_list[0].name, cb=cbar)

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

    def __init__(self, plotly_data, plotly_layout, input_data, axes, value_name,
                 cb):

        # Make a deep copy of the input data
        self.input_data = copy.deepcopy(input_data)

        # Get the dimensions of the image to be displayed
        naxes = len(axes)
        self.xcoord = self.input_data[axes[naxes-1]]
        self.ycoord = self.input_data[axes[naxes-2]]
        self.ydims = self.ycoord.dimensions
        self.ylabs = self.ydims.labels
        self.xdims = self.xcoord.dimensions
        self.xlabs = self.xdims.labels

        # Need these to avoid things running out of scope
        self.dims = self.input_data.dimensions
        self.labels = self.dims.labels
        self.shapes = self.dims.shape

        # Size of the slider coordinate arrays
        self.slider_nx = []
        # Save dimensions tags for sliders, e.g. Dim.X
        self.slider_dims = []
        # Coordinate variables for sliders, e.g. d[Coord.X]
        self.slider_coords = []
        # Store coordinates of dimensions that will be in sliders
        self.slider_x = []
        for ax in axes[:-2]:
            coord = self.input_data[ax]
            self.slider_coords.append(self.input_data[ax])
            dims = coord.dimensions
            labs = dims.labels
            # TODO: This loop is necessary; Ideally we would like to do
            #   self.slider_nx.append(self.input_data[ax].dimensions.shape[0])
            #   self.slider_dims.append(self.input_data[ax].dimensions.labels[0])
            #   self.slider_x.append(self.input_data[ax].numpy)
            # but we are running into scope problems.
            for j in range(len(self.labels)):
                if self.labels[j] == labs[0]:
                    self.slider_nx.append(self.shapes[j])
                    self.slider_dims.append(self.labels[j])
                    self.slider_x.append(self.input_data[ax].numpy)
        self.nslices = len(self.slider_dims)

        # Initialise Figure and VBox objects
        self.fig = FigureWidget(data=plotly_data, layout=plotly_layout)
        self.vbox = self.fig,

        # Initialise slider and label containers
        self.lab = []
        self.slider = []
        # Collect the remaining arguments
        self.value_name = value_name
        self.cb = cb
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
            title = Label(value=axis_label(self.slider_coords[i]))
            self.vbox += (HBox([title, self.slider[i], self.lab[i]]),)

        # Call update_slice once to make the initial image
        self.update_slice(0)
        self.vbox = VBox(self.vbox)
        self.vbox.layout.align_items = 'center'

        return

    # Define function to update slices
    def update_slice(self, change):
        # The dimensions to be sliced have been saved in slider_dims
        # Slice with first element to avoid modifying underlying dataset
        self.lab[0].value = str(self.slider_x[0][self.slider[0].value])
        zarray = self.input_data[self.slider_dims[0], self.slider[0].value]
        # Then slice additional dimensions if needed
        for idim in range(1, self.nslices):
            self.lab[idim].value = str(self.slider_x[idim][self.slider[idim].value])
            zarray = zarray[self.slider_dims[idim], self.slider[idim].value]

        vslice = zarray[Data.Value, self.value_name]
        z = vslice.numpy

        # Check if dimensions of arrays agree, if not, plot the transpose
        zdims = vslice.dimensions
        zlabs = zdims.labels
        if (zlabs[0] == self.xlabs[0]) and (zlabs[1] == self.ylabs[0]):
            z = z.T
        # Apply colorbar parameters
        if self.cb["log"]:
            with np.errstate(invalid="ignore", divide="ignore"):
                z = np.log10(z)
        if (self.cb["min"] is not None) + (self.cb["max"] is not None) == 1:
            if self.cb["min"] is not None:
                self.fig.data[0].zmin = self.cb["min"]
            else:
                self.fig.data[0].zmin = np.amin(z[np.where(np.isfinite(z))])
            if self.cb["max"] is not None:
                self.fig.data[0].zmax = self.cb["max"]
            else:
                self.fig.data[0].zmax = np.amax(z[np.where(np.isfinite(z))])
        self.fig.data[0].z = z
        return

#===============================================================================

# Convert coordinate edges to centers, and return also the widths
def edges_to_centers(x):
    return 0.5 * (x[1:] + x[:-1]), np.ediff1d(x)

# Convert coordinate centers to edges
def centers_to_edges(x):
    e = edges_to_centers(x)[0]
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

# Construct the colorbar using default and input values
def parse_colorbar(cb):
    cbar = default["cb"].copy()
    if cb is not None:
        for key, val in cb.items():
            cbar[key] = val
    return cbar

def coordinate_dimensions(coord):
    dims = coord.dimensions
    ndim = len(dims)
    if ndim != 1:
        raise RuntimeError("Found {} dimensions, expected 1. Only coordinates "
                           "with a single dimension are currently supported. "
                           "If you wish to plot data with a 2D coordinate, "
                           "please use rebin to re-sample the data onto a "
                           "common axis.")
    return dims

# Make x and y arrays from dimensions and check for bins edges
def process_dimensions(input_data, axes, values, ndim):
    xcoord = input_data[axes[1]]
    ycoord = input_data[axes[0]]
    # Check for bin edges
    # TODO: find a better way to handle edges. Currently, we convert from
    # edges to centers and then back to edges afterwards inside the plotly
    # object. This is not optimal and could lead to precision loss issues.
    zdims = values.dimensions
    zlabs = zdims.labels
    nz = zdims.shape
    ydims = coordinate_dimensions(ycoord)
    ylabs = ydims.labels
    ny = ydims.shape
    xdims = coordinate_dimensions(xcoord)
    xlabs = xdims.labels
    nx = xdims.shape
    # Get coordinate arrays
    x = xcoord.numpy
    y = ycoord.numpy
    if nx[0] == nz[ndim-1] + 1:
        x = edges_to_centers(x)[0]
    if ny[0] == nz[ndim-2] + 1:
        y = edges_to_centers(y)[0]
    return xcoord, ycoord, x, y, xlabs, ylabs, zlabs
