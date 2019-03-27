# Other imports
import numpy as np
import copy
import io
from contextlib import redirect_stdout

# Dataset imports
from dataset import Dataset, Data, dataset, dimensionCoord, coordDimension, sqrt, units

# Plotly imports
from plotly.offline import init_notebook_mode, iplot
try:
    # Re-direct the output of init_notebook_mode to hide it from the unit tests
    with redirect_stdout(io.StringIO()):
        init_notebook_mode(connected=True)
except ImportError:
    print("Warning: the current version of this plotting module was designed to"
          " work inside a Jupyter notebook. Other usage has not been tested.")
try:
    from plotly.graph_objs import FigureWidget
    from ipywidgets import VBox, HBox, IntSlider, Label
    from IPython.display import display
    ipywidgets_missing = False
except ImportError:
    print("Warning: possibly missing features. It was not possible to import "
          "ipywidgets, which is required for some of the plotting "
          "functionality.")
    ipywidgets_missing = True
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
            # Search through the variables and group the 1D datasets that have
            # the same coordinate axis.
            # tobeplotted is a dict that holds pairs of
            # [number_of_dimensions, DatasetSlice], or
            # [number_of_dimensions, [List of DatasetSlices]] in the case of
            # 1d data.
            # TODO: 0D data is currently ignored -> find a nice way of
            # displaying it?
            tobeplotted = dict()
            for i in range(len(values)):
                if ndims[i] == 1:
                    dims = values[i].dimensions
                    labs = dims.labels
                    key = str(labs[0])
                    if key in tobeplotted.keys():
                        tobeplotted[key][1].append(input_data.subset[values[i].name])
                    else:
                        tobeplotted[key] = [ndims[i], [input_data.subset[values[i].name]]]
                elif ndims[i] > 1:
                    tobeplotted[values[i].name] = [ndims[i], input_data.subset[values[i].name]]

            # Plot all the subsets
            color_count = 0
            for key, val in tobeplotted.items():
                if val[0] == 1:
                    color = []
                    for l in val[1]:
                        color.append(DEFAULT_PLOTLY_COLORS[color_count%10])
                        color_count += 1
                    plot_1d(val[1], color=color)
                else:
                    plot_auto(val[1], ndim=val[0])
            return
        else:
            return plot_auto(input_data, ndim=np.amax(ndims), axes=axes,
                             waterfall=waterfall, collapse=collapse, **kwargs)

#===============================================================================

# Function to automaticall dispatch the input dataset to the appropriate
# plotting function depending on its dimensions
def plot_auto(input_data, ndim=0, axes=None, waterfall=None, collapse=None,
              **kwargs):

    if ndim == 1:
        return plot_1d(input_data, axes=axes, **kwargs)
    elif ndim == 2:
        if collapse is not None:
            return plot_1d(plot_waterfall(input_data, dim=collapse, axes=axes,
                                          plot=False), **kwargs)
        elif waterfall is not None:
            return plot_waterfall(input_data, dim=waterfall, axes=axes,
                                  **kwargs)
        else:
            return plot_image(input_data, axes=axes, **kwargs)
    else:
        return plot_sliceviewer(input_data, axes=axes, **kwargs)

#===============================================================================

# Plot a 1D spectrum.
#
# Inputs can be either a Dataset(Slice) or a list of Dataset(Slice)s.
# If the coordinate of the x-axis contains bin edges, then a bar plot is made.
#
# TODO: find a more general way of handling arguments to be sent to plotly,
# probably via a dictionay of arguments
def plot_1d(input_data, logx=False, logy=False, logxy=False, axes=None,
            color=None):

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
    color_count = 0
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
            if color is not None:
                if "marker" not in trace.keys():
                    trace["marker"] = dict()
                trace["marker"]["color"] = color[color_count]
                color_count += 1
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
        )
    if histogram:
        layout["barmode"] = "overlay"
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
def plot_image(input_data, axes=None, contours=False, cb=None, plot=True,
               resolution=128):

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
        xcoord, ycoord, xe, ye, xc, yc, xlabs, ylabs, zlabs = \
            process_dimensions(input_data=input_data, axes=axes,
                               values=values[0], ndim=ndim)

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

        # Check colorbar parameters
        cbcount = (cbar["min"] is not None) + (cbar["max"] is not None)

        layout = dict(
            xaxis = dict(title = axis_label(xcoord)),
            yaxis = dict(title = axis_label(ycoord))
            )

        if plot:
            z = values[0].numpy
            # Check if dimensions of arrays agree, if not, plot the transpose
            if (zlabs[0] == xlabs[0]) and (zlabs[1] == ylabs[0]):
                z = z.T
            # Apply colorbar parameters
            if cbar["log"]:
                with np.errstate(invalid="ignore", divide="ignore"):
                    z = np.log10(z)
            if cbar["min"] is None:
                cbar["min"] = np.amin(z[np.where(np.isfinite(z))])
            if cbar["max"] is None:
                cbar["max"] = np.amax(z[np.where(np.isfinite(z))])
            imview = ImageViewer(xe, xc, ye, yc, z, resolution, cbar, plot_type,
                                 title)
            for key, val in layout.items():
                imview.fig.layout[key] = val
            return display(imview.fig)

        else:
            data = [dict(
                x = xe,
                y = ye,
                z = [0.0],
                type = plot_type,
                colorscale = cbar["name"],
                colorbar=dict(
                    title=title,
                    titleside = 'right',
                    )
                )]
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

class ImageViewer:

    def __init__(self, xe, xc, ye, yc, z, resolution, cb, plot_type, title):

        self.xe = xe
        self.xc = xc
        self.ye = ye
        self.yc = yc
        self.z = z
        self.resolution = resolution
        self.cb = cb
        self.plot_type = plot_type
        self.title = title
        self.nx = len(self.xe)
        self.ny = len(self.ye)

        self.fig = FigureWidget()

        # Make an initial low-resolution sampling of the image for plotting
        self.resample_image(layout=None, x_range=[self.xe[0], self.xe[-1]],
                            y_range=[self.ye[0], self.ye[-1]])

        # Add a callback to update the view area
        self.fig.layout.on_change(self.resample_image, 'xaxis.range', 'yaxis.range')

        return

    def resample_image(self, layout, x_range, y_range):

        # Find indices of xe and ye that are shown in current range
        x_in_range = np.where(np.logical_and(self.xe >= x_range[0], self.xe <= x_range[1]))
        y_in_range = np.where(np.logical_and(self.ye >= y_range[0], self.ye <= y_range[1]))

        # xmin, xmax... here are array indices, not float coordinates
        xmin = x_in_range[0][0]
        xmax = x_in_range[0][-1]
        ymin = y_in_range[0][0]
        ymax = y_in_range[0][-1]
        # here we perform a trick so that the edges of the displayed image is
        # not greyed out if the zoom area slices a pixel in half, only the pixel
        # inside the view area will be shown and the outer edge between that
        # last pixel edge and the edge of the view frame area will be empty. So
        # we extend the selected area with an additional pixel, if the selected
        # area is inside the global limits of the full resolution array.
        xmin -= int(xmin > 0)
        xmax += int(xmax < self.nx-1)
        ymin -= int(ymin > 0)
        ymax += int(ymax < self.ny-1)

        # Par of the global coordinate arrays that are inside the viewing area
        xview = self.xe[xmin:xmax+1]
        yview = self.ye[ymin:ymax+1]

        # Count the number of pixels in the current view
        nx_view = xmax - xmin
        ny_view = ymax - ymin

        # Define x and y edges for histogramming
        # If the number of pixels in the view area is larger than the maximum
        # allowed resolution we create some custom pixels
        if nx_view > self.resolution:
            xe_loc = np.linspace(xview[0], xview[-1], self.resolution)
        else:
            xe_loc = xview
        if ny_view > self.resolution:
            ye_loc = np.linspace(yview[0], yview[-1], self.resolution)
        else:
            ye_loc = yview

        # Optimize if no re-sampling is required
        if (nx_view < self.resolution) and (ny_view < self.resolution):
            z1 = self.z[ymin:ymax,xmin:xmax]
        else:
            xg, yg = np.meshgrid(self.xc[xmin:xmax], self.yc[ymin:ymax])
            xv = np.ravel(xg)
            yv = np.ravel(yg)
            zv = np.ravel(self.z[ymin:ymax,xmin:xmax])
            # Histogram the data to make a low-resolution image
            # Using weights in the second histogram allows us to then do z1/z0 to
            # obtain the averaged data inside the coarse pixels
            z0, yedges1, xedges1 = np.histogram2d(yv, xv, bins=(ye_loc, xe_loc))
            z1, yedges1, xedges1 = np.histogram2d(yv, xv, bins=(ye_loc, xe_loc), weights=zv)
            z1 /= z0

        # Here we perform another trick. If we plot simply the local arrays in
        # plotly, the reset axes or home functionality will be lost because
        # plotly will now think that the data that eixsts is only the small
        # window shown after a zoom. So we add a one-pixel padding area to the
        # local z array. The size of that padding extends from the edges of the
        # initial full resolution array (e.g. x=0, y=0) up to the edge of the
        # view area. These large (and probably elongated) pixels add very little
        # data and will not show in the view area but allow plotly to recover
        # the full axes limits if we double-click on the plot
        if xmin > 0:
            xe_loc = np.concatenate([self.xe[0:1], xe_loc])
        if xmax < self.nx-1:
            xe_loc = np.concatenate([xe_loc, self.xe[-1:]])
        if ymin > 0:
            ye_loc = np.concatenate([self.ye[0:1], ye_loc])
        if ymax < self.ny-1:
            ye_loc = np.concatenate([ye_loc, self.ye[-1:]])
        imin = int(xmin > 0)
        imax = int(xmax < self.nx-1)
        jmin = int(ymin > 0)
        jmax = int(ymax < self.ny-1)

        # The local z array
        z_loc = np.zeros([len(ye_loc)-1, len(xe_loc)-1])
        z_loc[jmin:len(ye_loc)-jmax-1,imin:len(xe_loc)-imax-1] = z1

        self.fig.update({'data': [{'type':self.plot_type, 'x':xe_loc,
                                   'y':ye_loc, 'z':z_loc, 'zmin':self.cb["min"],
                                   'zmax':self.cb["max"],
                                   'colorscale':self.cb["name"],
                                   'colorbar':{'title':self.title}}]})
        return

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
        xcoord, ycoord, xe, ye, xc, yc, xlabs, ylabs, zlabs = \
            process_dimensions(input_data=input_data, axes=axes,
                               values=values[0], ndim=ndim)

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
            for i in range(len(yc)):
                if plot:
                    idict = pdict.copy()
                    idict["x"] = xc
                    idict["y"] = [yc[i]] * len(xc)
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
            for i in range(len(xc)):
                if plot:
                    idict = pdict.copy()
                    idict["x"] = [xc[i]] * len(yc)
                    idict["y"] = yc
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

    if ipywidgets_missing:
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

        return display(sv.vbox)

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
    if nx[0] == nz[ndim-1]:
        xe = centers_to_edges(x)
        xc = x
    elif nx[0] == nz[ndim-1] + 1:
        xe = x
        xc = edges_to_centers(x)[0]
    else:
        raise RuntimeError("Dimensions of Coord ({}) and Value ({}) do not "
                           "match.".format(nx[0], nz[ndim-1]))
    if ny[0] == nz[ndim-2]:
        ye = centers_to_edges(y)
        yc = y
    elif ny[0] == nz[ndim-2] + 1:
        ye = y
        yc = edges_to_centers(y)[0]
    else:
        raise RuntimeError("Dimensions of Coord ({}) and Value ({}) do not "
                           "match.".format(ny[0], nz[ndim-2]))
    return xcoord, ycoord, xe, ye, xc, yc, xlabs, ylabs, zlabs
