# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

# Import numpy
import numpy as np

# Import scippy
import scippy as sp

# Plotly imports
from IPython.display import display
from plotly.io import write_image
from plotly.colors import DEFAULT_PLOTLY_COLORS
from ipywidgets import VBox, HBox, IntSlider, Label
from plotly.graph_objs import FigureWidget

# =============================================================================

# Some global configuration
default = {
    # The colorbar properties
    "cb": {"name": "Viridis", "log": False, "min": None, "max": None},
    # The default image height (in pixels)
    "height": 600
}

# =============================================================================


# def check_input(input_data, check_multiple_values=True):

#     variables = []
#     ndims = []
#     for name, var in input_data:
#         # if tag.is_data and (tag != sp.Data.Variance):
#         variables.append((name, var))
#         ndims.append(len(var.coords))

#     # if check_multiple_values and (len(values) > 1) and (np.amax(ndims) > 1):
#     #     raise RuntimeError("More than one Data.Value found! Please use e.g."
#     #                        " plot(dataset.subset[Data.Value, 'sample'])"
#     #                        " to select only a single Value.")

#     return variables, ndims

# =============================================================================


def plot(input_data, **kwargs):
    """
    Wrapper function to plot any kind of dataset
    """

    # Create a list of variables which will then be dispatched to the plot_auto
    # function.
    # Search through the variables and group the 1D datasets that have
    # the same coordinate axis.
    # tobeplotted is a dict that holds pairs of
    # [number_of_dimensions, DatasetSlice], or
    # [number_of_dimensions, [List of DatasetSlices]] in the case of
    # 1d sp.Data.
    # TODO: 0D data is currently ignored -> find a nice way of
    # displaying it?
    if not isinstance(input_data, list):
        input_data = [input_data]

    tobeplotted = dict()
    for ds in input_data:
        for name, var in ds:
            coords = var.coords
            ndims = len(coords)
            if ndims == 1:
                # TODO: change this to lab = coords[0] by adding a getitem
                for c in coords:
                    lab = c[0]
                # Make a unique key from the dataset id in case there are more
                # than one dataset with 1D variables with the same coordinates
                key = "{}_{}".format(str(id(ds)), lab)
                if key in tobeplotted.keys():
                    tobeplotted[key][1][name] = ds[name]
                else:
                    tobeplotted[key] = [ndims, {name: ds[name]}]
            elif ndims > 1:
                tobeplotted[name] = [ndims, ds[name]]

    # Plot all the subsets
    color_count = 0
    for key, val in tobeplotted.items():
        if val[0] == 1:
            color = []
            for l in val[1].keys():
                color.append(DEFAULT_PLOTLY_COLORS[color_count % 10])
                color_count += 1
            plot_1d(val[1], color=color, **kwargs)
        else:
            plot_auto(val[1], ndim=val[0], name=key, **kwargs)
    return

# =============================================================================


def plot_auto(input_data, ndim=0, name=None, waterfall=None,
              collapse=None, **kwargs):
    """
    Function to automatically dispatch the input dataset to the appropriate
    plotting function depending on its dimensions
    """

    if collapse is not None:
        plot_collapse(input_data, dim=collapse, name=name, **kwargs)
    elif ndim == 1:
        plot_1d(input_data, **kwargs)
    elif ndim == 2:
        if waterfall is not None:
            plot_waterfall(input_data, name=name, dim=waterfall, **kwargs)
        else:
            plot_image(input_data, name=name, **kwargs)
    else:
        plot_sliceviewer(input_data, name=name, **kwargs)
    return

# =============================================================================


def plot_1d(input_data, logx=False, logy=False, logxy=False, axes=None,
            color=None, filename=None):
    """
    Plot a 1D spectrum.

    Inputs can be either a Dataset(Slice) or a list of Dataset(Slice)s.
    If the coordinate of the x-axis contains bin edges, then a bar plot is
    made.

    TODO: find a more general way of handling arguments to be sent to plotly,
    probably via a dictionay of arguments
    """

    data = []
    coord_check = None
    color_count = 0
    for name, var in input_data.items():
        # TODO: find a better way of getting x by accessing the dimension of
        # the coordinate directly instead of iterating over all of them
        coords = var.coords
        for c in coords:
            x = coords[c[0]].values
            xlab = axis_label(coords[c[0]])
        y = var.values
        ylab = axis_label(var=var, name=name)
        # TODO: add variances

        nx = x.shape[0]
        ny = y.shape[0]
        histogram = False
        if nx == ny + 1:
            x, w = edges_to_centers(x)
            histogram = True

        # Define trace
        trace = dict(x=x, y=y, name=ylab)
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
        # # Include variance if present
        # if v[1] is not None:
        #     trace["error_y"] = dict(
        #         type='data',
        #         array=sp.sqrt(v[1]).numpy,
        #         visible=True)

        data.append(trace)

    layout = dict(
        xaxis=dict(title=xlab),
        yaxis=dict(),
        showlegend=True,
        legend=dict(x=0.0, y=1.15, orientation="h"),
        height=default["height"]
    )
    if histogram:
        layout["barmode"] = "overlay"
    if logx or logxy:
        layout["xaxis"]["type"] = "log"
    if logy or logxy:
        layout["yaxis"]["type"] = "log"

    fig = FigureWidget(data=data, layout=layout)
    if filename is not None:
        write_image(fig=fig, file=filename)
    else:
        display(fig)
    return

# =============================================================================

def plot_collapse(input_data, dim=None, name=None, filename=None, **kwargs):
    """
    Collapse higher dimensions into a 1D plot.
    """

    dims = input_data.dims
    labs = dims.labels
    coords = input_data.coords

    # Gather list of dimensions that are to be collapsed
    slice_dims = []
    volume = 1
    for l in labs:
        if l != dim:
            slice_dims.append(l)
            volume *= dims[l]

    # Create temporary Dataset
    ds = sp.Dataset()
    ds.set_coord(dim, sp.Variable([dim], values=coords[dim].values))
    # A dictionary to hold the DataProxy objects
    data = dict()

    # Go through the dims that need to be collapsed, and create an array that
    # holds the range of indices for each dimension
    # Say we have [Dim.Y, 5], and [Dim.Z, 3], then dim_list will contain
    # [[0, 1, 2, 3, 4], [0, 1, 2]]
    dim_list = []
    for l in slice_dims:
         dim_list.append(np.arange(dims[l], dtype=np.int32))
    # Next create a grid of indices
    # grid will contain
    # [ [[0, 1, 2, 3, 4], [0, 1, 2, 3, 4], [0, 1, 2, 3, 4]],
    #   [[0, 0, 0, 0, 0], [1, 1, 1, 1, 1], [2, 2, 2, 2, 2]] ]
    grid = np.meshgrid(*[x for x in dim_list])
    # Reshape the grid to have a 2D array of length volume, i.e.
    # [ [0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 0, 1, 2, 3, 4],
    #   [0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2] ]
    res = np.reshape(grid, (len(slice_dims), volume))
    # Now make a master array which also includes the dimension labels, i.e.
    # [ [Dim.Y, Dim.Y, Dim.Y, Dim.Y, Dim.Y, Dim.Y, Dim.Y, Dim.Y, Dim.Y, Dim.Y, Dim.Y, Dim.Y, Dim.Y, Dim.Y, Dim.Y],
    #   [    0,     1,     2,     3,     4,     0,     1,     2,     3,     4,     0,     1,     2,     3,     4],
    #   [Dim.Z, Dim.Z, Dim.Z, Dim.Z, Dim.Z, Dim.Z, Dim.Z, Dim.Z, Dim.Z, Dim.Z, Dim.Z, Dim.Z, Dim.Z, Dim.Z, Dim.Z],
    #   [    0,     0,     0,     0,     0,     1,     1,     1,     1,     1,     2,     2,     2,     2,     2] ]
    slice_list = []
    for i, l in enumerate(slice_dims):
        slice_list.append([l] * volume)
        slice_list.append(res[i])
    # Finally reshape the master array to look like
    # [ [[Dim.Y, 0], [Dim.Z, 0]], [[Dim.Y, 1], [Dim.Z, 0]], [[Dim.Y, 2], [Dim.Z, 0]],
    #   [[Dim.Y, 3], [Dim.Z, 0]], [[Dim.Y, 4], [Dim.Z, 0]], [[Dim.Y, 0], [Dim.Z, 1]],
    #   [[Dim.Y, 1], [Dim.Z, 1]], [[Dim.Y, 2], [Dim.Z, 1]], [[Dim.Y, 3], [Dim.Z, 1]],
    # ...
    # ]
    slice_list = np.reshape(np.transpose(slice_list), (volume, len(slice_dims), 2))

    # Extract each entry from the slice_list, make temperary dataset and add to
    # input dictionary for plot_1d
    for line in slice_list:
        ds_temp = input_data
        key = ""
        for s in line:
            ds_temp = ds_temp[s[0], s[1]]
            key += "{}-{}-".format(str(s[0]), s[1])
        ds[key] = sp.Variable([dim], values=ds_temp.values)
        data[key] = ds[key]

    # Send the newly created dictionary of DataProxy to the plot_1d function
    plot_1d(data, **kwargs)

    return

# =============================================================================


def plot_image(input_data, name=None, axes=None, contours=False, cb=None, plot=True,
               resolution=128, filename=None):
    """
    Plot a 2D image.

    If countours=True, a filled contour plot is produced, if False, then a
    standard image made of pixels is created.
    If plot=False, then not plot is produced, instead the data and layout dicts
    for plotly, as well as a transpose flag, are returned (this is used when
    calling plot_image from the sliceviewer).
    """

    coords = input_data.coords
    ndim = len(coords)

    if axes is not None:
        naxes = len(axes)
    else:
        naxes = 0

    if (ndim == 2) or ((not plot) and (naxes > 1)):

        # Get coordinates axes and dimensions
        coords = input_data.coords
        xcoord, ycoord, xe, ye, xc, yc, xlabs, ylabs, zlabs = \
            process_dimensions(input_data=input_data, coords=coords, axes=axes)

        if contours:
            plot_type = 'contour'
        else:
            plot_type = 'heatmap'

        # Parse colorbar
        cbar = parse_colorbar(cb)

        # Make title
        title = axis_label(var=input_data, name=name, log=cbar["log"])

        layout = dict(
            xaxis=dict(title=axis_label(xcoord)),
            yaxis=dict(title=axis_label(ycoord)),
            height=default["height"]
        )

        if plot:
            z = input_data.values
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
            imview = ImageViewer(xe, xc, ye, yc, z, resolution, cbar,
                                 plot_type, title, contours)
            for key, val in layout.items():
                imview.fig.layout[key] = val
            if filename is not None:
                write_image(fig=imview.fig, file=filename)
            else:
                display(imview.fig)
            return
        else:
            data = [dict(
                x=xe,
                y=ye,
                z=[0.0],
                type=plot_type,
                colorscale=cbar["name"],
                colorbar=dict(
                    title=title,
                    titleside='right',
                )
            )]
            return data, layout, xlabs, ylabs, cbar

    else:
        raise RuntimeError(
            "Unsupported number of dimensions in plot_image. "
            "Expected at least 2 dimensions, got {}. To plot 1D "
            "data, use plot_1d, and for 3 dimensions and higher,"
            " use plot_sliceviewer. One can also call plot_image"
            "for a Dataset with more than 2 dimensions, but one "
            "must then use plot=False to collect the data and "
            "layout dicts for plotly, as well as a transpose "
            "flag, instead of plotting an image.".format(ndim))

# =============================================================================


class ImageViewer:

    def __init__(self, xe, xc, ye, yc, z, resolution, cb, plot_type, title,
                 contours):

        self.xe = xe
        self.xc = xc
        self.ye = ye
        self.yc = yc
        self.z = z
        self.resolution = resolution
        self.cb = cb
        self.plot_type = plot_type
        self.title = title
        self.contours = contours
        self.nx = len(self.xe)
        self.ny = len(self.ye)

        self.fig = FigureWidget()

        # Make an initial low-resolution sampling of the image for plotting
        self.resample_image(layout=None, x_range=[self.xe[0], self.xe[-1]],
                            y_range=[self.ye[0], self.ye[-1]])

        # Add a callback to update the view area
        self.fig.layout.on_change(
            self.resample_image,
            'xaxis.range',
            'yaxis.range')

        return

    def resample_image(self, layout, x_range, y_range):

        # Find indices of xe and ye that are shown in current range
        x_in_range = np.where(
            np.logical_and(
                self.xe >= x_range[0],
                self.xe <= x_range[1]))
        y_in_range = np.where(
            np.logical_and(
                self.ye >= y_range[0],
                self.ye <= y_range[1]))

        # xmin, xmax... here are array indices, not float coordinates
        xmin = x_in_range[0][0]
        xmax = x_in_range[0][-1]
        ymin = y_in_range[0][0]
        ymax = y_in_range[0][-1]
        # here we perform a trick so that the edges of the displayed image is
        # not greyed out if the zoom area slices a pixel in half, only the
        # pixel inside the view area will be shown and the outer edge between
        # that last pixel edge and the edge of the view frame area will be
        # empty. So we extend the selected area with an additional pixel, if
        # the selected area is inside the global limits of the full resolution
        # array.
        xmin -= int(xmin > 0)
        xmax += int(xmax < self.nx - 1)
        ymin -= int(ymin > 0)
        ymax += int(ymax < self.ny - 1)

        # Par of the global coordinate arrays that are inside the viewing area
        xview = self.xe[xmin:xmax + 1]
        yview = self.ye[ymin:ymax + 1]

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
            z1 = self.z[ymin:ymax, xmin:xmax]
        else:
            xg, yg = np.meshgrid(self.xc[xmin:xmax], self.yc[ymin:ymax])
            xv = np.ravel(xg)
            yv = np.ravel(yg)
            zv = np.ravel(self.z[ymin:ymax, xmin:xmax])
            # Histogram the data to make a low-resolution image
            # Using weights in the second histogram allows us to then do z1/z0
            # to obtain the averaged data inside the coarse pixels
            z0, yedges1, xedges1 = np.histogram2d(
                yv, xv, bins=(ye_loc, xe_loc))
            z1, yedges1, xedges1 = np.histogram2d(
                yv, xv, bins=(ye_loc, xe_loc), weights=zv)
            z1 /= z0

        # Here we perform another trick. If we plot simply the local arrays in
        # plotly, the reset axes or home functionality will be lost because
        # plotly will now think that the data that exists is only the small
        # window shown after a zoom. So we add a one-pixel padding area to the
        # local z array. The size of that padding extends from the edges of the
        # initial full resolution array (e.g. x=0, y=0) up to the edge of the
        # view area. These large (and probably elongated) pixels add very
        # little data and will not show in the view area but allow plotly to
        # recover the full axes limits if we double-click on the plot
        xc_loc = edges_to_centers(xe_loc)[0]
        yc_loc = edges_to_centers(ye_loc)[0]
        if xmin > 0:
            xe_loc = np.concatenate([self.xe[0:1], xe_loc])
            xc_loc = np.concatenate([self.xc[0:1], xc_loc])
        if xmax < self.nx - 1:
            xe_loc = np.concatenate([xe_loc, self.xe[-1:]])
            xc_loc = np.concatenate([xc_loc, self.xc[-1:]])
        if ymin > 0:
            ye_loc = np.concatenate([self.ye[0:1], ye_loc])
            yc_loc = np.concatenate([self.yc[0:1], yc_loc])
        if ymax < self.ny - 1:
            ye_loc = np.concatenate([ye_loc, self.ye[-1:]])
            yc_loc = np.concatenate([yc_loc, self.yc[-1:]])
        imin = int(xmin > 0)
        imax = int(xmax < self.nx - 1)
        jmin = int(ymin > 0)
        jmax = int(ymax < self.ny - 1)
        nxe = len(xe_loc)
        nye = len(ye_loc)

        # The local z array
        z_loc = np.zeros([nye - 1, nxe - 1])
        z_loc[jmin:nye - jmax - 1, imin:nxe - imax - 1] = z1

        # The 'data' dictionary
        datadict = dict(type=self.plot_type, zmin=self.cb["min"],
                        zmax=self.cb["max"], colorscale=self.cb["name"],
                        colorbar={"title": self.title}, z=z_loc)

        if self.contours:
            datadict["x"] = xc_loc
            datadict["y"] = yc_loc
        else:
            datadict["x"] = xe_loc
            datadict["y"] = ye_loc

        # Update the figure
        self.fig.update({'data': [datadict]})

        return

# =============================================================================


def plot_waterfall(input_data, dim=None, name=None, axes=None, filename=None):
    """
    Make a 3D waterfall plot
    """

    # Get coordinates axes and dimensions
    coords = input_data.coords
    xcoord, ycoord, xe, ye, xc, yc, xlabs, ylabs, zlabs = \
        process_dimensions(input_data=input_data, coords=coords, axes=axes)

    data = []
    z = input_data.values

    # TODO: add variances
    e = None
    # # Check if we need to add variances to dataset list for collapse plot
    # if not plot:
    #     for name, tag, var in input_data:
    #         if (tag == sp.Data.Variance) and \
    #            (name == values[0].name):
    #             e = var.numpy

    if (zlabs[0] == xlabs[0]) and (zlabs[1] == ylabs[0]):
        z = z.T
        zlabs = [ylabs[0], xlabs[0]]
        if e is not None:
            e = e.T

    pdict = dict(type='scatter3d', mode='lines', line=dict(width=5))
    adict = dict(z=1)

    if dim is None:
        dim = zlabs[0]

    if dim == zlabs[0]:
        for i in range(len(yc)):
            idict = pdict.copy()
            idict["x"] = xc
            idict["y"] = [yc[i]] * len(xc)
            idict["z"] = z[i, :]
            data.append(idict)
            adict["x"] = 3
            adict["y"] = 1
    elif dim == zlabs[1]:
        for i in range(len(xc)):
            idict = pdict.copy()
            idict["x"] = [xc[i]] * len(yc)
            idict["y"] = yc
            idict["z"] = z[:, i]
            data.append(idict)
            adict["x"] = 1
            adict["y"] = 3
    else:
        raise RuntimeError("Something went wrong in plot_waterfall. The "
                           "waterfall dimension is not recognised.")

    layout = dict(
        scene=dict(
            xaxis=dict(
                title=axis_label(xcoord)),
            yaxis=dict(
                title=axis_label(ycoord)),
            zaxis=dict(
                title=axis_label(var=input_data,
                                 name=name)),
            aspectmode='manual',
            aspectratio=adict),
        showlegend=False,
        height=default["height"])
    fig = FigureWidget(data=data, layout=layout)
    if filename is not None:
        write_image(fig=fig, file=filename)
    else:
        display(fig)
    return

    # else:
    #     raise RuntimeError(
    #         "Unsupported number of dimensions in plot_waterfall."
    #         " Expected at least 2 dimensions, got {}." .format(ndim))

# =============================================================================


def plot_sliceviewer(input_data, axes=None, contours=False, cb=None,
                     filename=None):
    """
    Plot a 2D slice through a 3D dataset with a slider to adjust the position
    of the slice in the third dimension.
    """

    # Check input dataset
    value_list, ndims = check_input(input_data)
    ndim = np.amax(ndims)

    if ndim > 2:

        if axes is None:
            dims = value_list[0].dimensions
            labs = dims.labels
            axes = [sp.dimensionCoord(label) for label in labs]

        # Use the machinery in plot_image to make the layout
        data, layout, xlabs, ylabs, cbar = plot_image(input_data, axes=axes,
                                                      contours=contours, cb=cb,
                                                      plot=False)

        # Create a SliceViewer object
        sv = SliceViewer(plotly_data=data, plotly_layout=layout,
                         input_data=input_data, axes=axes,
                         value_name=value_list[0].name, cb=cbar)

        if filename is not None:
            write_image(fig=sv.fig, file=filename)
        else:
            display(sv.vbox)
        return

    else:
        raise RuntimeError("Unsupported number of dimensions in "
                           "plot_sliceviewer. Expected at least 3 dimensions, "
                           "got {}. For 2D data, use plot_image, for 1D data, "
                           "use plot_1d.".format(ndim))

# =============================================================================


class SliceViewer:

    def __init__(self, plotly_data, plotly_layout, input_data, axes,
                 value_name, cb):

        # Make a copy of the input data
        self.input_data = input_data.copy()

        # Get the dimensions of the image to be displayed
        naxes = len(axes)
        self.xcoord = self.input_data[axes[naxes - 1]]
        self.ycoord = self.input_data[axes[naxes - 2]]
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
            # self.slider_nx.append(self.input_data[ax].dimensions.shape[0])
            # self.slider_dims.append(self.input_data[ax].dimensions.labels[0])
            # self.slider_x.append(self.input_data[ax].numpy)
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
                max=self.slider_nx[i] - 1,
                step=1,
                description="",
                continuous_update=True,
                readout=False
            ))
            # Add an observer to the slider
            self.slider[i].observe(self.update_slice, names="value")
            # Add coordinate name and unit
            title = Label(value=axis_label(*self.slider_coords[i]))
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
            self.lab[idim].value = str(
                self.slider_x[idim][self.slider[idim].value])
            zarray = zarray[self.slider_dims[idim], self.slider[idim].value]

        vslice = zarray[sp.Data.Value, self.value_name]
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

# =============================================================================


def edges_to_centers(x):
    """
    Convert coordinate edges to centers, and return also the widths
    """
    return 0.5 * (x[1:] + x[:-1]), np.ediff1d(x)


def centers_to_edges(x):
    """
    Convert coordinate centers to edges
    """
    e = edges_to_centers(x)[0]
    return np.concatenate([[2.0 * x[0] - e[0]], e, [2.0 * x[-1] - e[-1]]])


def axis_label(var, name=None, log=False):
    """
    Make an axis label with "Name [unit]"
    """
    if name is not None:
        label = name
    else:
        label = str(var.dims.labels[0]).replace("Dim.", "")

    if log:
        label = "log\u2081\u2080(" + label + ")"
    if var.unit != sp.units.dimensionless:
        label += " [{}]".format(var.unit)


    # if tag.is_coord:
    #     label = "{}".format(tag)
    #     label = label.replace("Coord.", "")
    # else:
    #     label = "{}".format(name)
    # if var.unit != sp.units.dimensionless:
    #     label += " [{}]".format(var.unit)
    return label


def parse_colorbar(cb):
    """
    Construct the colorbar using default and input values
    """
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


def process_dimensions(input_data, coords, axes):
    """
    Make x and y arrays from dimensions and check for bins edges
    """
    # coords = input_data.coords
    zdims = input_data.dims
    zlabs = zdims.labels
    nz = zdims.shape

    if axes is None:
        axes = [l for l in zlabs]
    else:
        axes = axes[-2:]

    # Get coordinate arrays
    xcoord = coords[axes[-1]]
    ycoord = coords[axes[-2]]
    x = xcoord.values
    y = ycoord.values

    # Check for bin edges
    # TODO: find a better way to handle edges. Currently, we convert from
    # edges to centers and then back to edges afterwards inside the plotly
    # object. This is not optimal and could lead to precision loss issues.
    ydims = ycoord.dims
    ylabs = ydims.labels
    ny = ydims.shape
    xdims = xcoord.dims
    xlabs = xdims.labels
    nx = xdims.shape
    # Find the dimension in z that corresponds to x and y
    ix = iy = None
    for i in range(len(zlabs)):
        if zlabs[i] == xlabs[0]:
            ix = i
        if zlabs[i] == ylabs[0]:
            iy = i
    if (ix is None) or (iy is None):
        raise RuntimeError(
            "Dimension of either x ({}) or y ({}) array was not "
            "found in z ({}) array.".format(
                xdims, ydims, zdims))
    if nx[0] == nz[ix]:
        xe = centers_to_edges(x)
        xc = x
    elif nx[0] == nz[ix] + 1:
        xe = x
        xc = edges_to_centers(x)[0]
    else:
        raise RuntimeError("Dimensions of x Coord ({}) and Value ({}) do not "
                           "match.".format(nx[0], nz[ix]))
    if ny[0] == nz[iy]:
        ye = centers_to_edges(y)
        yc = y
    elif ny[0] == nz[iy] + 1:
        ye = y
        yc = edges_to_centers(y)[0]
    else:
        raise RuntimeError("Dimensions of y Coord ({}) and Value ({}) do not "
                           "match.".format(ny[0], nz[iy]))
    return xcoord, ycoord, xe, ye, xc, yc, xlabs, ylabs, zlabs
