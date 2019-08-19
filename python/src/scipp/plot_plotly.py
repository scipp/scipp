# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

import numpy as np
from collections import namedtuple
from .tools import edges_to_centers, axis_label, parse_colorbar, \
                   process_dimensions

# Plotly imports
from IPython.display import display
from plotly.io import write_image
from plotly.colors import DEFAULT_PLOTLY_COLORS
from ipywidgets import VBox, HBox, IntSlider, Label
from plotly.graph_objs import FigureWidget, Layout
from plotly.offline import plot as plotlyplot

# =============================================================================


def plot_plotly(input_data, ndim=0, name=None, config=None, waterfall=None,
                collapse=None, **kwargs):
    """
    Function to automatically dispatch the input dataset to the appropriate
    plotting function depending on its dimensions
    """

    if ndim == 1:
        plot_1d(input_data, config=config, **kwargs)
    elif ndim == 2:
        if waterfall is not None:
            plot_waterfall(input_data, name=name, dim=waterfall, config=config,
                           **kwargs)
        else:
            plot_image(input_data, name=name, config=config, **kwargs)
    else:
        plot_sliceviewer(input_data, name=name, config=config, **kwargs)
    return

# =============================================================================


def plot_1d(input_data, logx=False, logy=False, logxy=False, axes=None,
            color=None, filename=None, config=None):
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
        # TODO: find a better way of getting x by accessing the dimension of
        # the coordinate directly instead of iterating over all of them
        coords = var.coords
        for c in coords:
            x = coords[c[0]].values
            xlab = axis_label(coords[c[0]])
        y = var.values
        ylab = axis_label(var=var, name=name)

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
        # Include variance if present
        if var.variances is not None:
            trace["error_y"] = dict(
                type='data',
                array=np.sqrt(var.variances),
                visible=True)

        data.append(trace)

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

    fig = FigureWidget(data=data, layout=layout)
    if filename is not None:
        write_image(fig=fig, file=filename)
    else:
        display(fig)
    return


# =============================================================================


def plot_image(input_data, name=None, axes=None, contours=False, cb=None,
               plot=True, resolution=128, filename=None, show_variances=False,
               figsize=None, config=None, **kwargs):
    """
    Plot a 2D image.

    If countours=True, a filled contour plot is produced, if False, then a
    standard image made of pixels is created.
    If plot=False, then not plot is produced, instead the data and layout dicts
    for plotly, as well as a transpose flag, are returned (this is used when
    calling plot_image from the sliceviewer).
    """

    if figsize is None:
        figsize = [config.width, config.height]

    coords = input_data.coords

    # Get coordinates axes and dimensions
    coords = input_data.coords
    xcoord, ycoord, xe, ye, xc, yc, xlabs, ylabs, zlabs = \
        process_dimensions(input_data=input_data, coords=coords, axes=axes)

    if contours:
        plot_type = 'contour'
    else:
        plot_type = 'heatmap'

    # Parse colorbar
    cbar = parse_colorbar(config.cb, cb, plotly=True)

    # Make title
    title = axis_label(var=input_data, name=name, log=cbar["log"])

    layout = dict(
        xaxis=dict(title=axis_label(xcoord)),
        yaxis=dict(title=axis_label(ycoord)),
        height=figsize[1],
        width=figsize[0]
    )
    if input_data.variances is not None and show_variances:
        layout["width"] *= 0.5
        layout["height"] = 0.8 * layout["width"]

    if plot:

        # Prepare dictionaries for holding key parameters
        data = {"values": None, "variances": None}
        params = {"values": {"cbmin": "min", "cbmax": "max"},
                  "variances": None}
        if input_data.variances is not None and show_variances:
            params["variances"] = {"cbmin": "min_var", "cbmax": "max_var"}

        for i, (key, val) in enumerate(sorted(params.items())):
            if val is not None:
                arr = getattr(input_data, key)
                # Check if dimensions of arrays agree, if not, transpose
                if (zlabs[0] == xlabs[0]) and (zlabs[1] == ylabs[0]):
                    arr = arr.T
                # Apply colorbar parameters
                if cbar["log"]:
                    with np.errstate(invalid="ignore", divide="ignore"):
                        arr = np.log10(arr)
                if cbar[val["cbmin"]] is None:
                    cbar[val["cbmin"]] = np.amin(
                        arr[np.where(np.isfinite(arr))])
                if cbar[val["cbmax"]] is None:
                    cbar[val["cbmax"]] = np.amax(
                        arr[np.where(np.isfinite(arr))])
                if key == "variances":
                    arr = np.sqrt(arr)
                data[key] = arr

        imview = ImageViewer(xe, xc, ye, yc, data["values"], data["variances"],
                             resolution, cbar, plot_type, title, contours)

        for key, val in layout.items():
            imview.fig_val.layout[key] = val
            if params["variances"] is not None:
                imview.fig_var.layout[key] = val

        if filename is not None:
            if type(filename) == str:
                pos = filename.rfind(".")
                root = filename[:pos]
                ext = filename[pos:]
                val_file = filename
                var_file = root + "_err" + ext
            else:
                val_file = filename[0]
                var_file = filename[1]

            if val_file.endswith(".html"):
                plotlyplot(imview.fig_val, filename=val_file, auto_open=False)
                if (params["variances"] is not None) and show_variances:
                    plotlyplot(imview.fig_var, filename=var_file,
                               auto_open=False)
            else:
                write_image(fig=imview.fig_val, file=val_file)
                if (params["variances"] is not None) and show_variances:
                    write_image(fig=imview.fig_var, file=var_file)
        else:
            if (params["variances"] is not None) and show_variances:
                display(imview.hbox)
            else:
                display(imview.fig_val)
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

# =============================================================================


class ImageViewer:

    def __init__(self, xe, xc, ye, yc, values, variances, resolution, cb,
                 plot_type, title, contours):

        self.xe = xe
        self.xc = xc
        self.ye = ye
        self.yc = yc
        self.values = values
        self.variances = variances
        self.resolution = resolution
        self.cb = cb
        self.plot_type = plot_type
        self.title = title
        self.contours = contours
        self.nx = len(self.xe)
        self.ny = len(self.ye)
        self.val_range = np.zeros([4], dtype=np.float64)
        self.var_range = np.zeros([4], dtype=np.float64)

        self.fig_val = FigureWidget()
        if self.variances is not None:
            self.fig_var = FigureWidget()
            self.hbox = HBox([self.fig_val, self.fig_var])
        else:
            self.fig_var = None
            self.hbox = None

        # Make an initial low-resolution sampling of the image for plotting
        self.update_values(layout=None, x_range=[self.xe[0], self.xe[-1]],
                           y_range=[self.ye[0], self.ye[-1]])

        # Add a callback to update the view area
        self.fig_val.layout.on_change(
            self.update_values,
            'xaxis.range',
            'yaxis.range')
        if self.variances is not None:
            self.fig_var.layout.on_change(
                self.update_variances,
                'xaxis.range',
                'yaxis.range')

        return

    def update_values(self, layout, x_range, y_range, origin=None, res=None):

        ranges = np.array([x_range[0], x_range[1], y_range[0], y_range[1]],
                          copy=True)

        if not np.array_equal(self.val_range, ranges):

            self.val_range = ranges.copy()
            if res is None:
                res = self.resample_image(x_range, y_range)

            # The local values array
            values_loc = np.zeros([res.nye - 1, res.nxe - 1])
            values_loc[res.jmin:res.nye - res.jmax - 1,
                       res.imin:res.nxe - res.imax - 1] = \
                self.resample_arrays(self.values, res)

            # Update figure data dict
            res.datadict["z"] = values_loc
            res.datadict["colorbar"] = {"title": {"text": self.title,
                                                  "side": "right"}}

            # Update the figure
            updatedict = {'data': [res.datadict]}
            if origin is not None:
                # Make a copy of the layout to update it all at once
                layoutdict = Layout(self.fig_var.layout)
                updatedict["layout"] = layoutdict
            self.fig_val.update(updatedict)
            if (origin is None) and (self.variances is not None):
                self.update_variances(layout, x_range, y_range, 'values', res)
        return

    def update_variances(self, layout, x_range, y_range, origin=None,
                         res=None):

        ranges = np.array([x_range[0], x_range[1], y_range[0], y_range[1]],
                          copy=True)

        if not np.array_equal(self.var_range, ranges):

            self.var_range = ranges.copy()
            if res is None:
                res = self.resample_image(x_range, y_range)

            # The local variances array
            variances_loc = np.zeros([res.nye - 1, res.nxe - 1])
            variances_loc[res.jmin:res.nye - res.jmax - 1,
                          res.imin:res.nxe - res.imax - 1] = \
                self.resample_arrays(self.variances, res)

            # Update figure data dict
            res.datadict["z"] = variances_loc
            res.datadict["colorbar"] = {"title": {"text": "std. dev",
                                                  "side": "right"}}

            # Update the figure
            updatedict = {'data': [res.datadict]}
            if origin is not None:
                # Make a copy of the layout to update it all at once
                layoutdict = Layout(self.fig_val.layout)
                updatedict["layout"] = layoutdict
            self.fig_var.update(updatedict)
            if origin is None:
                self.update_values(layout, x_range, y_range, "variances", res)

        return

    def resample_image(self, x_range, y_range):

        # Create a namedtuple to hold the results
        out = namedtuple("out", ['nx_view', 'ny_view', 'xmin', 'xmax', 'ymin',
                                 'ymax', 'xe_loc', 'ye_loc', 'nxe', 'nye',
                                 'imin', 'imax', 'jmin', 'jmax', 'datadict'])

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
        out.xmin = x_in_range[0][0]
        out.xmax = x_in_range[0][-1]
        out.ymin = y_in_range[0][0]
        out.ymax = y_in_range[0][-1]
        # here we perform a trick so that the edges of the displayed image
        # is not greyed out if the zoom area slices a pixel in half, only
        # the pixel inside the view area will be shown and the outer edge
        # between that last pixel edge and the edge of the view frame area
        # will be empty. So we extend the selected area with an additional
        # pixel, if the selected area is inside the global limits of the
        # full resolution array.
        out.xmin -= int(out.xmin > 0)
        out.xmax += int(out.xmax < self.nx - 1)
        out.ymin -= int(out.ymin > 0)
        out.ymax += int(out.ymax < self.ny - 1)

        # Part of the global coordinate arrays that are inside the viewing
        # area
        xview = self.xe[out.xmin:out.xmax + 1]
        yview = self.ye[out.ymin:out.ymax + 1]

        # Count the number of pixels in the current view
        out.nx_view = out.xmax - out.xmin
        out.ny_view = out.ymax - out.ymin

        # Define x and y edges for histogramming
        # If the number of pixels in the view area is larger than the
        # maximum allowed resolution we create some custom pixels
        if out.nx_view > self.resolution:
            out.xe_loc = np.linspace(xview[0], xview[-1], self.resolution + 1)
        else:
            out.xe_loc = xview
        if out.ny_view > self.resolution:
            out.ye_loc = np.linspace(yview[0], yview[-1], self.resolution + 1)
        else:
            out.ye_loc = yview

        # Here we perform another trick. If we plot simply the local arrays
        # in plotly, the reset axes or home functionality will be lost
        # because plotly will now think that the data that exists is only
        # the small window shown after a zoom. So we add a one-pixel
        # padding area to the local z array. The size of that padding
        # extends from the edges of the initial full resolution array
        # (e.g. x=0, y=0) up to the edge of the view area. These large
        # (and probably elongated) pixels add very little data and will not
        # show in the view area but allow plotly to recover the full axes
        # limits if we double-click on the plot
        xc_loc = edges_to_centers(out.xe_loc)[0]
        yc_loc = edges_to_centers(out.ye_loc)[0]
        if out.xmin > 0:
            out.xe_loc = np.concatenate([self.xe[0:1], out.xe_loc])
            xc_loc = np.concatenate([self.xc[0:1], xc_loc])
        if out.xmax < self.nx - 1:
            out.xe_loc = np.concatenate([out.xe_loc, self.xe[-1:]])
            xc_loc = np.concatenate([xc_loc, self.xc[-1:]])
        if out.ymin > 0:
            out.ye_loc = np.concatenate([self.ye[0:1], out.ye_loc])
            yc_loc = np.concatenate([self.yc[0:1], yc_loc])
        if out.ymax < self.ny - 1:
            out.ye_loc = np.concatenate([out.ye_loc, self.ye[-1:]])
            yc_loc = np.concatenate([yc_loc, self.yc[-1:]])
        out.imin = int(out.xmin > 0)
        out.imax = int(out.xmax < self.nx - 1)
        out.jmin = int(out.ymin > 0)
        out.jmax = int(out.ymax < self.ny - 1)
        out.nxe = len(out.xe_loc)
        out.nye = len(out.ye_loc)

        # The 'data' dictionary
        out.datadict = dict(type=self.plot_type, zmin=self.cb["min"],
                            zmax=self.cb["max"], colorscale=self.cb["name"])
        if self.contours:
            out.datadict["x"] = xc_loc
            out.datadict["y"] = yc_loc
        else:
            out.datadict["x"] = out.xe_loc
            out.datadict["y"] = out.ye_loc

        return out

    def resample_arrays(self, array, res):

        # Optimize if no re-sampling is required
        if (res.nx_view < self.resolution) and \
           (res.ny_view < self.resolution):
            return array[res.ymin:res.ymax, res.xmin:res.xmax]
        else:
            xg, yg = np.meshgrid(self.xc[res.xmin:res.xmax],
                                 self.yc[res.ymin:res.ymax])
            xv = np.ravel(xg)
            yv = np.ravel(yg)
            # Histogram the data to make a low-resolution image
            # Using weights in the second histogram allows us to then do
            # z1/z0 to obtain the averaged data inside the coarse pixels
            array0, yedges, xedges = np.histogram2d(
                yv, xv, bins=(res.ye_loc[res.jmin:res.nye - res.jmax],
                              res.xe_loc[res.imin:res.nxe - res.imax]))
            array1, yedges, xedges = np.histogram2d(
                yv, xv, bins=(res.ye_loc[res.jmin:res.nye - res.jmax],
                              res.xe_loc[res.imin:res.nxe - res.imax]),
                weights=np.ravel(array[res.ymin:res.ymax, res.xmin:res.xmax]))
            return array1 / array0

# =============================================================================


def plot_waterfall(input_data, dim=None, name=None, axes=None, filename=None,
                   config=None, **kwargs):
    """
    Make a 3D waterfall plot
    """

    # Get coordinates axes and dimensions
    coords = input_data.coords
    xcoord, ycoord, xe, ye, xc, yc, xlabs, ylabs, zlabs = \
        process_dimensions(input_data=input_data, coords=coords, axes=axes)

    data = []
    z = input_data.values

    if (zlabs[0] == xlabs[0]) and (zlabs[1] == ylabs[0]):
        z = z.T
        zlabs = [ylabs[0], xlabs[0]]

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
        height=config.height)
    fig = FigureWidget(data=data, layout=layout)
    if filename is not None:
        write_image(fig=fig, file=filename)
    else:
        display(fig)
    return

# =============================================================================


def plot_sliceviewer(input_data, axes=None, contours=False, cb=None,
                     filename=None, name=None, config=None, **kwargs):
    """
    Plot a 2D slice through a 3D dataset with a slider to adjust the position
    of the slice in the third dimension.
    """

    if axes is None:
        axes = input_data.dims

    # Use the machinery in plot_image to make the layout
    data, layout, xlabs, ylabs, cbar = plot_image(input_data, axes=axes,
                                                  contours=contours, cb=cb,
                                                  name=name, plot=False,
                                                  config=config)

    # Create a SliceViewer object
    sv = SliceViewer(plotly_data=data, plotly_layout=layout,
                     input_data=input_data, axes=axes,
                     value_name=name, cb=cbar)

    if filename is not None:
        write_image(fig=sv.fig, file=filename)
    else:
        display(sv.vbox)
    return

# =============================================================================


class SliceViewer:

    def __init__(self, plotly_data, plotly_layout, input_data, axes,
                 value_name, cb):

        # Make a copy of the input data - Needed?
        self.input_data = input_data

        # Get the dimensions of the image to be displayed
        self.coords = self.input_data.coords
        self.xcoord = self.coords[axes[-1]]
        self.ycoord = self.coords[axes[-2]]
        self.xlabs = self.xcoord.dims
        self.ylabs = self.ycoord.dims

        self.labels = self.input_data.dims
        self.shapes = dict(zip(self.labels, self.input_data.shape))

        # Size of the slider coordinate arrays
        self.slider_nx = []
        # Save dimensions tags for sliders, e.g. Dim.X
        self.slider_dims = []
        # Store coordinates of dimensions that will be in sliders
        self.slider_x = []
        for ax in axes[:-2]:
            self.slider_dims.append(ax)
            self.slider_nx.append(self.shapes[ax])
            self.slider_x.append(self.coords[ax].values)
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
            title = Label(value=axis_label(self.coords[self.slider_dims[i]]))
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
        vslice = self.input_data[self.slider_dims[0], self.slider[0].value]
        # Then slice additional dimensions if needed
        for idim in range(1, self.nslices):
            self.lab[idim].value = str(
                self.slider_x[idim][self.slider[idim].value])
            vslice = vslice[self.slider_dims[idim], self.slider[idim].value]

        z = vslice.values

        # Check if dimensions of arrays agree, if not, plot the transpose
        zlabs = vslice.dims
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


def get_plotly_color(index=0):
    return DEFAULT_PLOTLY_COLORS[index % len(DEFAULT_PLOTLY_COLORS)]
