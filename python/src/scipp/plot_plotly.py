# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

import numpy as np
from collections import namedtuple
from .tools import edges_to_centers, axis_label, parse_colorbar, \
                   process_dimensions, get_coord_array

# Plotly imports
from IPython.display import display
from plotly.io import write_html, write_image
from plotly.colors import DEFAULT_PLOTLY_COLORS
import ipywidgets as widgets
import plotly.graph_objs as go
from plotly.subplots import make_subplots
# from plotly.offline import plot as plotlyplot

# =============================================================================


def plot_plotly(input_data, ndim=0, name=None, config=None,
                collapse=None, projection="2d", **kwargs):
    """
    Function to automatically dispatch the input dataset to the appropriate
    plotting function depending on its dimensions
    """

    if ndim == 1:
        plot_1d(input_data, config=config, **kwargs)
    elif projection.lower() == "2d":
        # if waterfall is not None:
        #     plot_waterfall(input_data, name=name, dim=waterfall, config=config,
        #                    **kwargs)
        # else:
        plot_2d(input_data, name=name, config=config, ndim=ndim, **kwargs)
    elif projection.lower() == "3d":
        plot_3d(input_data, name=name, config=config, ndim=ndim, **kwargs)
    else:
        raise RuntimeError("Wrong projection type.")
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
            # trace2 = dict(x=x, y=y, type='scattergl', line={"shape": 'hv'}, showlegend=False)
            # x, w = edges_to_centers(x)
            histogram = True

        # Define trace
        trace = dict(x=x, y=y, name=ylab, type='scattergl')
        if histogram:
            trace["line"] = {"shape": 'hv'}
            trace["y"] = np.concatenate((trace["y"], [0.0]))
            
            # trace["type"] = 'bar'
            # trace["marker"] = dict(opacity=0.6, line=dict(width=0))
            # trace["width"] = w
            trace["fill"] = 'tozeroy'
        # else:
        #     trace["type"] = 'scattergl'
        if color is not None:
            # if "marker" not in trace.keys():
                # trace["marker"] = dict()
            trace["marker"] = {"color": color[color_count]}
        # Include variance if present
        if var.variances is not None:
            err_dict = dict(
                    type='data',
                    array=np.sqrt(var.variances),
                    visible=True,
                    color=color[color_count])
            if histogram:
                trace2 = dict(x=edges_to_centers(x), y=y, showlegend=False,
                    type='scattergl',
                mode='markers', error_y=err_dict, marker={"color": color[color_count]})
                data.append(trace2)
            else:
                trace["error_y"] = err_dict

        data.append(trace)
        # if histogram:
        #     # # trace2 = dict(x=x, y=y, name=ylab)
        #     # trace2["type"] = 'scattergl'
        #     # line={"shape": 'hv'}
        #     # trace["width"] = w
        #     data.append(trace2)
        color_count += 1

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

    fig = go.Figure(data=data, layout=layout)
    if filename is not None:
        if filename.endswith(".html"):
            write_html(fig=fig, file=filename, auto_open=False)
        else:
            write_image(fig=fig, file=filename)
    else:
        display(fig)
    return


# =============================================================================


# def plot_image(input_data, name=None, axes=None, contours=False, cb=None,
#                plot=True, resolution=128, filename=None, show_variances=False,
#                figsize=None, config=None, **kwargs):
#     """
#     Plot a 2D image.

#     If countours=True, a filled contour plot is produced, if False, then a
#     standard image made of pixels is created.
#     If plot=False, then not plot is produced, instead the data and layout dicts
#     for plotly, as well as a transpose flag, are returned (this is used when
#     calling plot_image from the sliceviewer).
#     """

#     if figsize is None:
#         figsize = [config.width, config.height]

#     # Get coordinates axes and dimensions
#     coords = input_data.coords
#     labels = input_data.labels
#     xcoord, ycoord, xe, ye, xc, yc, xlabs, ylabs, zlabs = \
#         process_dimensions(input_data=input_data, coords=coords,
#                            labels=labels, axes=axes)

#     if contours:
#         plot_type = 'contour'
#     else:
#         plot_type = 'heatmap'

#     # Parse colorbar
#     cbar = parse_colorbar(config.cb, cb, plotly=True)

#     # Make title
#     title = axis_label(var=input_data, name=name, log=cbar["log"])

#     layout = dict(
#         xaxis=dict(title=xcoord),
#         yaxis=dict(title=ycoord),
#         height=figsize[1],
#         width=figsize[0]
#     )
#     if input_data.variances is not None and show_variances:
#         layout["width"] *= 0.5
#         layout["height"] = 0.8 * layout["width"]

#     if plot:

#         # Prepare dictionaries for holding key parameters
#         data = {"values": None, "variances": None}
#         params = {"values": {"cbmin": "min", "cbmax": "max"},
#                   "variances": None}
#         if input_data.variances is not None and show_variances:
#             params["variances"] = {"cbmin": "min_var", "cbmax": "max_var"}

#         for i, (key, val) in enumerate(sorted(params.items())):
#             if val is not None:
#                 arr = getattr(input_data, key)
#                 # Check if dimensions of arrays agree, if not, transpose
#                 if (zlabs[0] == xlabs[0]) and (zlabs[1] == ylabs[0]):
#                     arr = arr.T
#                 # Apply colorbar parameters
#                 if cbar["log"]:
#                     with np.errstate(invalid="ignore", divide="ignore"):
#                         arr = np.log10(arr)
#                 if cbar[val["cbmin"]] is None:
#                     cbar[val["cbmin"]] = np.amin(
#                         arr[np.where(np.isfinite(arr))])
#                 if cbar[val["cbmax"]] is None:
#                     cbar[val["cbmax"]] = np.amax(
#                         arr[np.where(np.isfinite(arr))])
#                 if key == "variances":
#                     arr = np.sqrt(arr)
#                 data[key] = arr

#         imview = ImageViewer(xe, xc, ye, yc, data["values"], data["variances"],
#                              resolution, cbar, plot_type, title, contours)

#         for key, val in layout.items():
#             imview.fig_val.layout[key] = val
#             if params["variances"] is not None:
#                 imview.fig_var.layout[key] = val

#         if filename is not None:
#             if type(filename) == str:
#                 pos = filename.rfind(".")
#                 root = filename[:pos]
#                 ext = filename[pos:]
#                 val_file = filename
#                 var_file = root + "_err" + ext
#             else:
#                 val_file = filename[0]
#                 var_file = filename[1]

#             if val_file.endswith(".html"):
#                 plotlyplot(imview.fig_val, filename=val_file, auto_open=False)
#                 if (params["variances"] is not None) and show_variances:
#                     plotlyplot(imview.fig_var, filename=var_file,
#                                auto_open=False)
#             else:
#                 write_image(fig=imview.fig_val, file=val_file)
#                 if (params["variances"] is not None) and show_variances:
#                     write_image(fig=imview.fig_var, file=var_file)
#         else:
#             if (params["variances"] is not None) and show_variances:
#                 display(imview.hbox)
#             else:
#                 display(imview.fig_val)
#         return
#     else:
#         data = [dict(
#             x=xe,
#             y=ye,
#             z=[0.0],
#             type=plot_type,
#             colorscale=cbar["name"],
#             colorbar=dict(
#                 title=title,
#                 titleside='right',
#             )
#         )]
#         return data, layout, xlabs, ylabs, cbar

# # =============================================================================


# class ImageViewer:

#     def __init__(self, xe, xc, ye, yc, values, variances, resolution, cb,
#                  plot_type, title, contours):

#         self.xe = xe
#         self.xc = xc
#         self.ye = ye
#         self.yc = yc
#         self.values = values
#         self.variances = variances
#         self.resolution = resolution
#         self.cb = cb
#         self.plot_type = plot_type
#         self.title = title
#         self.contours = contours
#         self.nx = len(self.xe)
#         self.ny = len(self.ye)
#         self.val_range = np.zeros([4], dtype=np.float64)
#         self.var_range = np.zeros([4], dtype=np.float64)

#         self.fig_val = FigureWidget()
#         if self.variances is not None:
#             self.fig_var = FigureWidget()
#             self.hbox = HBox([self.fig_val, self.fig_var])
#         else:
#             self.fig_var = None
#             self.hbox = None

#         # Make an initial low-resolution sampling of the image for plotting
#         self.update_values(layout=None, x_range=[self.xe[0], self.xe[-1]],
#                            y_range=[self.ye[0], self.ye[-1]])

#         # Add a callback to update the view area
#         self.fig_val.layout.on_change(
#             self.update_values,
#             'xaxis.range',
#             'yaxis.range')
#         if self.variances is not None:
#             self.fig_var.layout.on_change(
#                 self.update_variances,
#                 'xaxis.range',
#                 'yaxis.range')

#         return

#     def update_values(self, layout, x_range, y_range, origin=None, res=None):

#         ranges = np.array([x_range[0], x_range[1], y_range[0], y_range[1]],
#                           copy=True)

#         if not np.array_equal(self.val_range, ranges):

#             self.val_range = ranges.copy()
#             if res is None:
#                 res = self.resample_image(x_range, y_range)

#             # The local values array
#             values_loc = np.zeros([res.nye - 1, res.nxe - 1])
#             values_loc[res.jmin:res.nye - res.jmax - 1,
#                        res.imin:res.nxe - res.imax - 1] = \
#                 self.resample_arrays(self.values, res)

#             # Update figure data dict
#             res.datadict["z"] = values_loc
#             res.datadict["colorbar"] = {"title": {"text": self.title,
#                                                   "side": "right"}}

#             # Update the figure
#             updatedict = {'data': [res.datadict]}
#             if origin is not None:
#                 # Make a copy of the layout to update it all at once
#                 layoutdict = Layout(self.fig_var.layout)
#                 updatedict["layout"] = layoutdict
#             self.fig_val.update(updatedict)
#             if (origin is None) and (self.variances is not None):
#                 self.update_variances(layout, x_range, y_range, 'values', res)
#         return

#     def update_variances(self, layout, x_range, y_range, origin=None,
#                          res=None):

#         ranges = np.array([x_range[0], x_range[1], y_range[0], y_range[1]],
#                           copy=True)

#         if not np.array_equal(self.var_range, ranges):

#             self.var_range = ranges.copy()
#             if res is None:
#                 res = self.resample_image(x_range, y_range)

#             # The local variances array
#             variances_loc = np.zeros([res.nye - 1, res.nxe - 1])
#             variances_loc[res.jmin:res.nye - res.jmax - 1,
#                           res.imin:res.nxe - res.imax - 1] = \
#                 self.resample_arrays(self.variances, res)

#             # Update figure data dict
#             res.datadict["z"] = variances_loc
#             res.datadict["colorbar"] = {"title": {"text": "std. dev",
#                                                   "side": "right"}}

#             # Update the figure
#             updatedict = {'data': [res.datadict]}
#             if origin is not None:
#                 # Make a copy of the layout to update it all at once
#                 layoutdict = Layout(self.fig_val.layout)
#                 updatedict["layout"] = layoutdict
#             self.fig_var.update(updatedict)
#             if origin is None:
#                 self.update_values(layout, x_range, y_range, "variances", res)

#         return

#     def resample_image(self, x_range, y_range):

#         # Create a namedtuple to hold the results
#         out = namedtuple("out", ['nx_view', 'ny_view', 'xmin', 'xmax', 'ymin',
#                                  'ymax', 'xe_loc', 'ye_loc', 'nxe', 'nye',
#                                  'imin', 'imax', 'jmin', 'jmax', 'datadict'])

#         # Find indices of xe and ye that are shown in current range
#         x_in_range = np.where(
#             np.logical_and(
#                 self.xe >= x_range[0],
#                 self.xe <= x_range[1]))
#         y_in_range = np.where(
#             np.logical_and(
#                 self.ye >= y_range[0],
#                 self.ye <= y_range[1]))

#         # xmin, xmax... here are array indices, not float coordinates
#         out.xmin = x_in_range[0][0]
#         out.xmax = x_in_range[0][-1]
#         out.ymin = y_in_range[0][0]
#         out.ymax = y_in_range[0][-1]
#         # here we perform a trick so that the edges of the displayed image
#         # is not greyed out if the zoom area slices a pixel in half, only
#         # the pixel inside the view area will be shown and the outer edge
#         # between that last pixel edge and the edge of the view frame area
#         # will be empty. So we extend the selected area with an additional
#         # pixel, if the selected area is inside the global limits of the
#         # full resolution array.
#         out.xmin -= int(out.xmin > 0)
#         out.xmax += int(out.xmax < self.nx - 1)
#         out.ymin -= int(out.ymin > 0)
#         out.ymax += int(out.ymax < self.ny - 1)

#         # Part of the global coordinate arrays that are inside the viewing
#         # area
#         xview = self.xe[out.xmin:out.xmax + 1]
#         yview = self.ye[out.ymin:out.ymax + 1]

#         # Count the number of pixels in the current view
#         out.nx_view = out.xmax - out.xmin
#         out.ny_view = out.ymax - out.ymin

#         # Define x and y edges for histogramming
#         # If the number of pixels in the view area is larger than the
#         # maximum allowed resolution we create some custom pixels
#         if out.nx_view > self.resolution:
#             out.xe_loc = np.linspace(xview[0], xview[-1], self.resolution + 1)
#         else:
#             out.xe_loc = xview
#         if out.ny_view > self.resolution:
#             out.ye_loc = np.linspace(yview[0], yview[-1], self.resolution + 1)
#         else:
#             out.ye_loc = yview

#         # Here we perform another trick. If we plot simply the local arrays
#         # in plotly, the reset axes or home functionality will be lost
#         # because plotly will now think that the data that exists is only
#         # the small window shown after a zoom. So we add a one-pixel
#         # padding area to the local z array. The size of that padding
#         # extends from the edges of the initial full resolution array
#         # (e.g. x=0, y=0) up to the edge of the view area. These large
#         # (and probably elongated) pixels add very little data and will not
#         # show in the view area but allow plotly to recover the full axes
#         # limits if we double-click on the plot
#         xc_loc = edges_to_centers(out.xe_loc)
#         yc_loc = edges_to_centers(out.ye_loc)
#         if out.xmin > 0:
#             out.xe_loc = np.concatenate([self.xe[0:1], out.xe_loc])
#             xc_loc = np.concatenate([self.xc[0:1], xc_loc])
#         if out.xmax < self.nx - 1:
#             out.xe_loc = np.concatenate([out.xe_loc, self.xe[-1:]])
#             xc_loc = np.concatenate([xc_loc, self.xc[-1:]])
#         if out.ymin > 0:
#             out.ye_loc = np.concatenate([self.ye[0:1], out.ye_loc])
#             yc_loc = np.concatenate([self.yc[0:1], yc_loc])
#         if out.ymax < self.ny - 1:
#             out.ye_loc = np.concatenate([out.ye_loc, self.ye[-1:]])
#             yc_loc = np.concatenate([yc_loc, self.yc[-1:]])
#         out.imin = int(out.xmin > 0)
#         out.imax = int(out.xmax < self.nx - 1)
#         out.jmin = int(out.ymin > 0)
#         out.jmax = int(out.ymax < self.ny - 1)
#         out.nxe = len(out.xe_loc)
#         out.nye = len(out.ye_loc)

#         # The 'data' dictionary
#         out.datadict = dict(type=self.plot_type, zmin=self.cb["min"],
#                             zmax=self.cb["max"], colorscale=self.cb["name"])
#         if self.contours:
#             out.datadict["x"] = xc_loc
#             out.datadict["y"] = yc_loc
#         else:
#             out.datadict["x"] = out.xe_loc
#             out.datadict["y"] = out.ye_loc

#         return out

#     def resample_arrays(self, array, res):

#         # Optimize if no re-sampling is required
#         if (res.nx_view < self.resolution) and \
#            (res.ny_view < self.resolution):
#             return array[res.ymin:res.ymax, res.xmin:res.xmax]
#         else:
#             xg, yg = np.meshgrid(self.xc[res.xmin:res.xmax],
#                                  self.yc[res.ymin:res.ymax])
#             xv = np.ravel(xg)
#             yv = np.ravel(yg)
#             # Histogram the data to make a low-resolution image
#             # Using weights in the second histogram allows us to then do
#             # z1/z0 to obtain the averaged data inside the coarse pixels
#             array0, yedges, xedges = np.histogram2d(
#                 yv, xv, bins=(res.ye_loc[res.jmin:res.nye - res.jmax],
#                               res.xe_loc[res.imin:res.nxe - res.imax]))
#             array1, yedges, xedges = np.histogram2d(
#                 yv, xv, bins=(res.ye_loc[res.jmin:res.nye - res.jmax],
#                               res.xe_loc[res.imin:res.nxe - res.imax]),
#                 weights=np.ravel(array[res.ymin:res.ymax, res.xmin:res.xmax]))
#             return array1 / array0

# =============================================================================


# def plot_waterfall(input_data, dim=None, name=None, axes=None, filename=None,
#                    config=None, **kwargs):
#     """
#     Make a 3D waterfall plot
#     """

#     # Get coordinates axes and dimensions
#     coords = input_data.coords
#     labels = input_data.labels
#     xcoord, ycoord, xe, ye, xc, yc, xlabs, ylabs, zlabs = \
#         process_dimensions(input_data=input_data, coords=coords,
#                            labels=labels, axes=axes)

#     data = []
#     z = input_data.values

#     if (zlabs[0] == xlabs[0]) and (zlabs[1] == ylabs[0]):
#         z = z.T
#         zlabs = [ylabs[0], xlabs[0]]

#     pdict = dict(type='scatter3d', mode='lines', line=dict(width=5))
#     adict = dict(z=1)

#     if dim is None:
#         dim = zlabs[0]

#     if dim == zlabs[0]:
#         for i in range(len(yc)):
#             idict = pdict.copy()
#             idict["x"] = xc
#             idict["y"] = [yc[i]] * len(xc)
#             idict["z"] = z[i, :]
#             data.append(idict)
#             adict["x"] = 3
#             adict["y"] = 1
#     elif dim == zlabs[1]:
#         for i in range(len(xc)):
#             idict = pdict.copy()
#             idict["x"] = [xc[i]] * len(yc)
#             idict["y"] = yc
#             idict["z"] = z[:, i]
#             data.append(idict)
#             adict["x"] = 1
#             adict["y"] = 3
#     else:
#         raise RuntimeError("Something went wrong in plot_waterfall. The "
#                            "waterfall dimension is not recognised.")

#     layout = dict(
#         scene=dict(
#             xaxis=dict(
#                 title=xcoord),
#             yaxis=dict(
#                 title=ycoord),
#             zaxis=dict(
#                 title=axis_label(var=input_data,
#                                  name=name)),
#             aspectmode='manual',
#             aspectratio=adict),
#         showlegend=False,
#         height=config.height)
#     fig = FigureWidget(data=data, layout=layout)
#     if filename is not None:
#         write_image(fig=fig, file=filename)
#     else:
#         display(fig)
#     return

# =============================================================================


def plot_2d(input_data, axes=None, contours=False, cb=None,
                     filename=None, name=None, config=None, figsize=None,
                     show_variances=False, ndim=0, **kwargs):
    """
    Plot a 2D slice through a 3D dataset with a slider to adjust the position
    of the slice in the third dimension.
    """

    if axes is None:
        axes = input_data.dims

    # Get coordinates axes and dimensions
    coords = input_data.coords
    labels = input_data.labels
    xcoord, ycoord, xe, ye, xc, yc, xlabs, ylabs, zlabs = \
        process_dimensions(input_data=input_data, coords=coords,
                           labels=labels, axes=axes)

    if contours:
        plot_type = 'contour'
    else:
        plot_type = 'heatmap'

    # Parse colorbar
    cbar = parse_colorbar(config.cb, cb, plotly=True)

    # Make title
    title = axis_label(var=input_data, name=name, log=cbar["log"])

    # # Use the machinery in plot_image to make the layout
    # data, layout, xlabs, ylabs, cbar = plot_image(input_data, axes=axes,
    #                                               contours=contours, cb=cb,
    #                                               name=name, plot=False,
    #                                               config=config)

    if figsize is None:
        figsize = [config.width, config.height]

    layout = dict(
        xaxis=dict(title=xcoord),
        yaxis=dict(title=ycoord),
        height=figsize[1],
        width=figsize[0]
    )
    if input_data.variances is not None and show_variances:
        # layout["width"] *= 0.5
        layout["height"] = 0.7 * layout["height"]

    data = [dict(
            x=xe,
            y=ye,
            z=[0.0],
            type=plot_type,
            colorscale=cbar["name"],
            colorbar=dict(
                title=title,
                titleside='right',
                lenmode='fraction',
                len=1.05,
                thicknessmode='fraction',
                thickness=0.03
            )
        )]

    # Create a SliceViewer object
    sv = Slicer2d(data=data, layout=layout,
                     input_data=input_data, axes=axes,
                     value_name=name, cb=cbar, show_variances=show_variances)

    if filename is not None:
        write_image(fig=sv.fig, file=filename)
    else:
        display(sv.vbox)
    return

# =============================================================================


class Slicer2d:

    def __init__(self, data, layout, input_data, axes,
                 value_name, cb, show_variances):

        self.input_data = input_data
        self.show_variances = show_variances
        self.value_name = value_name
        self.cb = cb

        # Get the dimensions of the image to be displayed
        self.coords = self.input_data.coords
        self.labels = self.input_data.labels
        _, self.xcoord = get_coord_array(self.coords, self.labels, axes[-1])
        _, self.ycoord = get_coord_array(self.coords, self.labels, axes[-2])
        self.xlabs = self.xcoord.dims
        self.ylabs = self.ycoord.dims

        self.labels = self.input_data.dims
        self.shapes = dict(zip(self.labels, self.input_data.shape))

        # Size of the slider coordinate arrays
        self.slider_nx = dict()
        # Save dimensions tags for sliders, e.g. Dim.X
        self.slider_dims = []
        # Store coordinates of dimensions that will be in sliders
        self.slider_x = dict()
        # self.buttons = dict()
        for ax in axes:
            self.slider_dims.append(ax)
        self.ndim = len(self.slider_dims)
        for dim in self.slider_dims:
            key = str(dim)
            self.slider_nx[key] = self.shapes[dim]
            self.slider_x[key] = self.coords[dim].values
            # self.buttons[key]
        self.nslices = len(self.slider_dims)

        # Initialise Figure and VBox objects
        self.fig = None
        # data = {"values": None, "variances": None}
        params = {"values": {"cbmin": "min", "cbmax": "max"},
                  "variances": None}
        if (self.input_data.variances is not None) and self.show_variances:
            params["variances"] = {"cbmin": "min_var", "cbmax": "max_var"}
            self.fig = go.FigureWidget(make_subplots(rows=1, cols=2, horizontal_spacing=0.16))
            data[0]["colorbar"]["x"] = 0.42
            data[0]["colorbar"]["thickness"] = 0.02
            self.fig.add_trace(data[0], row=1, col=1)
            data[0]["colorbar"]["title"] = "variances"
            data[0]["colorbar"]["x"] = 1.0
            self.fig.add_trace(data[0], row=1, col=2)
            # self.fig["variances"] = go.FigureWidget(data=data, layout=layout)
            # self.vbox.append(self.fig["variances"])
            # self.vbox = [HBox(self.vbox)]
            for i in range(2):
                self.fig.update_xaxes(title_text=layout["xaxis"]["title"], row=1, col=i+1)
                self.fig.update_yaxes(title_text=layout["yaxis"]["title"], row=1, col=i+1)
            self.fig.update_layout(height=layout["height"], width=layout["width"])
        else:
            self.fig = go.FigureWidget(data=data, layout=layout)
            # if (self.cb["min"] is not None) + (self.cb["max"] is not None) == 1:
            # vals = self.input_data.values
            # if self.cb["min"] is not None:
            #     self.fig.data[0].zmin = self.cb["min"]
            # else:
            #     self.fig.data[0].zmin = np.amin(vals[np.where(np.isfinite(vals))])
            # if self.cb["max"] is not None:
            #     self.fig.data[0].zmax = self.cb["max"]
            # else:
            #     self.fig.data[0].zmax = np.amax(vals[np.where(np.isfinite(vals))])

        # zlabs = vslice.dims
        for i, (key, val) in enumerate(sorted(params.items())):
            if val is not None:
                arr = getattr(self.input_data, key)
                # vals = self.input_data.values
                if self.cb[val["cbmin"]] is not None:
                    self.fig.data[i].zmin = self.cb[val["cbmin"]]
                else:
                    self.fig.data[i].zmin = np.amin(arr[np.where(np.isfinite(arr))])
                if self.cb[val["cbmax"]] is not None:
                    self.fig.data[i].zmax = self.cb[val["cbmax"]]
                else:
                    self.fig.data[i].zmax = np.amax(arr[np.where(np.isfinite(arr))])


        self.vbox = [self.fig]


        # Initialise slider and label containers
        self.lab = dict()
        self.slider = dict()
        self.buttons = dict()
        # Default starting index for slider
        indx = 0

        # Now begin loop to construct sliders
        button_values = [None] * (self.ndim - 2) + ['X'] + ['Y']
        # print(button_values)
        for i, dim in enumerate(self.slider_dims):
            key = str(dim)
            # Add a label widget to display the value of the z coordinate
            self.lab[key] = widgets.Label(value=str(self.slider_x[key][indx]))
            # Add an IntSlider to slide along the z dimension of the array
            self.slider[key] = widgets.IntSlider(
                value=indx,
                min=0,
                max=self.slider_nx[key] - 1,
                step=1,
                description=key,
                continuous_update=True,
                readout=False, disabled=(i>=self.ndim-2)
            )
            self.buttons[key] = widgets.ToggleButtons(
                options=['X', 'Y'], description='',
                value=button_values[i],
                disabled=False,
                button_style='')
            # print(dir(self.buttons[key]))
            setattr(self.buttons[key], "dim_str", key)
            setattr(self.buttons[key], "old_value", self.buttons[key].value)
            setattr(self.slider[key], "dim_str", key)
            setattr(self.slider[key], "dim", dim)
            # self.buttons[key].observe(self.update_buttons, 'value')
            self.buttons[key].on_msg(self.update_buttons)
            
            # Add an observer to the slider
            self.slider[key].observe(self.update_slice2d, names="value")
            # Add coordinate name and unit
            # title = Label(value=axis_label(self.coords[self.slider_dims[i]]))
            self.vbox.append(widgets.HBox([self.slider[key], self.lab[key], self.buttons[key]]))

        # Call update_slice once to make the initial image
        # if len(self.slider) > 0:
        # self.update_slice2d({"new": 0})
        self.vbox = widgets.VBox(self.vbox)
        self.vbox.layout.align_items = 'center'

        return

    def update_buttons(self, owner, event, dummy):
        # print(owner, event, dummy)
        # print(dir(owner))
        # print(arg1, arg2, arg3)
        # print(change["owner"].dim)
        # print("start")
        # print(owner)
        toggle_slider = False
        switch_button = False
        if not self.slider[owner.dim_str].disabled:
            toggle_slider = True
            self.slider[owner.dim_str].disabled = True
        else:
            switch_button = True

        # print(dir(change["owner"]))
        for key, button in self.buttons.items():
            # print(key, change["owner"].dim, 
            # print(key, change["owner"].dim_str, button.value, change["new"])
            if (button.value == owner.value) and (key != owner.dim_str):
                # print("changing")
                if self.slider[key].disabled:
                    button.value = owner.old_value
                else:
                    button.value = None
                button.old_value = button.value
                if toggle_slider:
                    self.slider[key].disabled = False
        # print("end")
        owner.old_value = owner.value

        return

    # def update_buttons(self, change):
    #     # print(change["owner"].dim)
    #     print("start")
    #     print(change)
    #     print(dir(change["owner"]))
    #     for key, button in self.buttons.items():
    #         # print(key, change["owner"].dim, 
    #         print(key, change["owner"].dim_str, button.value, change["new"])
    #         if (button.value == change["new"]) and (key != change["owner"].dim_str):
    #             print("changing")
    #             button.value = None
    #     print("end")

    #     return

    # Define function to update slices
    def update_slice2d(self, change):
        # # The dimensions to be sliced have been saved in slider_dims
        # # Slice with first element to avoid modifying underlying dataset
        # self.lab[0].value = str(self.slider_x[0][self.slider[0].value])
        # vslice = self.input_data[self.slider_dims[0], self.slider[0].value]
        vslice = self.input_data
        # Then slice additional dimensions if needed
        # for dim in self.slider_dims:
        for key, val in self.slider.items():
            if not val.disabled:
                print("slicing along", val.dim)
                self.lab[key].value = str(
                    self.slider_x[key][change["new"]])
                vslice = vslice[val.dim, change["new"]]

        # vals = vslice.values
        # Check if dimensions of arrays agree, if not, plot the transpose
        zlabs = vslice.dims
        transp = (zlabs[0] == self.xlabs[0]) and (zlabs[1] == self.ylabs[0])
        self.update_z2d(vslice.values, transp, self.cb["log"], 0)


        #     vals = vals.T
        # # Apply colorbar parameters
        # if self.cb["log"]:
        #     with np.errstate(invalid="ignore", divide="ignore"):
        #         vals = np.log10(vals)
        # self.fig.data[0].z = vals

        if (self.input_data.variances is not None) and self.show_variances:
            self.update_z2d(vslice.variances, transp, self.cb["log"], 1)
            # vals = vslice.variances
            # # Check if dimensions of arrays agree, if not, plot the transpose
            # # zlabs = vslice.dims
            # if (zlabs[0] == self.xlabs[0]) and (zlabs[1] == self.ylabs[0]):
            #     vals = vals.T
            # # Apply colorbar parameters
            # if self.cb["log"]:
            #     with np.errstate(invalid="ignore", divide="ignore"):
            #         vals = np.log10(vals)
            # self.fig.data[1].z = vals
        return

    def update_z2d(self, values, transp, log, indx):
        if transp:
            values = values.T
        # Apply colorbar parameters
        if log:
            with np.errstate(invalid="ignore", divide="ignore"):
                values = np.log10(values)
        self.fig.data[indx].z = values


        # data = {"values": None, "variances": None}
        # params = {"values": {"cbmin": "min", "cbmax": "max"},
        #           "variances": None}
        # if (self.input_data.variances is not None) and self.show_variances:
        #     params["variances"] = {"cbmin": "min_var", "cbmax": "max_var"}
        # zlabs = vslice.dims
        # for i, (key, val) in enumerate(sorted(params.items())):
        #     if val is not None:
        #         arr = getattr(vslice, key)
        #         # Check if dimensions of arrays agree, if not, transpose
        #         if (zlabs[0] == self.xlabs[0]) and (zlabs[1] == self.ylabs[0]):
        #             arr = arr.T
        #         # if key == "variances":
        #         #     arr = np.sqrt(arr)
        #         # Apply colorbar parameters
        #         if self.cb["log"]:
        #             with np.errstate(invalid="ignore", divide="ignore"):
        #                 arr = np.log10(arr)
        #         if self.cb[val["cbmin"]] is not None:
        #             self.fig.data[i].zmin = self.cb[val["cbmin"]]
        #         else:
        #             self.fig.data[i].zmin = np.amin(arr[np.where(np.isfinite(arr))])
        #         if self.cb[val["cbmax"]] is not None:
        #             self.fig.data[i].zmax = self.cb[val["cbmax"]]
        #         else:
        #             self.fig.data[i].zmax = np.amax(arr[np.where(np.isfinite(arr))])
        #         self.fig.data[i].z = arr

        return






def plot_3d(input_data, axes=None, contours=False, cb=None,
                     filename=None, name=None, config=None, figsize=[800, 800],
                     show_variances=False, ndim=0, **kwargs):
    """
    Plot a 2D slice through a 3D dataset with a slider to adjust the position
    of the slice in the third dimension.
    """

    if axes is None:
        axes = input_data.dims

    # # Get coordinates axes and dimensions
    coords = input_data.coords
    dims = input_data.dims
    shape = input_data.shape
    # print(shape[0])
    # print(dims)
    # print(shape)
    # return
    # # labels = input_data.labels
    # # xcoord, ycoord, xe, ye, xc, yc, xlabs, ylabs, zlabs = \
    # #     process_dimensions(input_data=input_data, coords=coords,
    # #                        labels=labels, axes=axes)

    xcoord = coords[dims[-1]]
    xc = xcoord.values
    ycoord = coords[dims[-2]]
    yc = ycoord.values
    if ndim > 2:
        zcoord = coords[dims[-3]]
        zc = zcoord.values



    if contours:
        plot_type = 'contour'
    else:
        plot_type = 'heatmap'

    # Parse colorbar
    cbar = parse_colorbar(config.cb, cb, plotly=True)

    # Make title
    title = axis_label(var=input_data, name=name, log=cbar["log"])

    # # Use the machinery in plot_image to make the layout
    # data, layout, xlabs, ylabs, cbar = plot_image(input_data, axes=axes,
    #                                               contours=contours, cb=cb,
    #                                               name=name, plot=False,
    #                                               config=config)

    if figsize is None:
        figsize = [config.width, config.height]

    layout = dict(
        xaxis=dict(title=axis_label(xcoord)),
        yaxis=dict(title=axis_label(ycoord)),
        # zaxis=dict(title=axis_label(zcoord)),
        height=figsize[1],
        width=figsize[0]
    )
    if ndim > 2:
        layout["zaxis"] = dict(title=axis_label(zcoord))
    if input_data.variances is not None and show_variances:
        layout["width"] *= 0.5
        layout["height"] = 0.8 * layout["width"]

    # data = [dict(
    #         x=xe,
    #         y=ye,
    #         z=[0.0],
    #         type=plot_type,
    #         colorscale=cbar["name"],
    #         colorbar=dict(
    #             title=title,
    #             titleside='right',
    #         )
    #     )]
    # print(np.shape(xc))
    # print(np.sin(xc))
    
    
    if ndim == 2:
        data=[go.Surface(x=xc, y=yc, z=input_data.values, opacity=0.9, colorscale=cbar["name"])]
        fig = go.Figure(data=data, layout=layout)
        if filename is not None:
            write_image(fig=fig, file=filename)
        else:
            display(fig)
        # return
    else:
        x1, y1 = np.meshgrid(xc, yc)
        x2, z2 = np.meshgrid(xc, zc)
        y3, z3 = np.meshgrid(yc, zc)
        if cbar["min"] is not None:
            zmin = cbar["min"]
        else:
            zmin = np.amin(input_data.values[np.where(np.isfinite(input_data.values))])
        if cbar["max"] is not None:
            zmax = cbar["max"]
        else:
            zmax = np.amax(input_data.values[np.where(np.isfinite(input_data.values))])
        data=[go.Surface(x=np.zeros_like(y3), y=y3, z=z3, cmin=zmin, cmax=zmax),
              go.Surface(x=x2, y=np.zeros_like(x2), z=z2, cmin=zmin, cmax=zmax, showscale=False),
              go.Surface(x=x1, y=y1, z=np.zeros_like(x1), cmin=zmin, cmax=zmax, showscale=False)]
        # Create a SliceViewer object
        sv = Slicer3d(data=data, layout=layout,
                         input_data=input_data, axes=axes,
                         value_name=name, cb=cbar)
        if filename is not None:
            write_image(fig=sv.fig, file=filename)
        else:
            display(sv.vbox)

    return







# class Update3d:

#     def __init__(self, parent, idim, dim):
#         self.parent = parent
#         self.idim = idim
#         self.dim = dim
#         self.moving_coord = ["z", "y", "x"]
#         return

#         # Define function to update slices
#     def update_slice(self, change):
#         print(change)
#         # # The dimensions to be sliced have been saved in slider_dims
#         # # Slice with first element to avoid modifying underlying dataset
#         # self.lab[0].value = str(self.slider_x[0][self.slider[0].value])
#         # vslice = self.input_data[self.slider_dims[0], self.slider[0].value]
#         vslice = self.parent.input_data
#         # Then slice additional dimensions if needed
#         # for idim in range(self.nslices):
#         # idim = 0
#         self.parent.lab[self.idim].value = str(
#             self.parent.slider_x[self.idim][change["new"]])
#         vslice = vslice[self.parent.slider_dims[self.idim], change["new"]]

#         z = vslice.values
#         # print(z)

#         # Check if dimensions of arrays agree, if not, plot the transpose
#         zlabs = vslice.dims
#         # if (zlabs[0] == self.xlabs[0]) and (zlabs[1] == self.ylabs[0]):
#         #     z = z.T
#         # Apply colorbar parameters
#         if self.parent.cb["log"]:
#             with np.errstate(invalid="ignore", divide="ignore"):
#                 z = np.log10(z)
#         # if (self.parent.cb["min"] is not None) + (self.parent.cb["max"] is not None) == 1:
#         #     if self.parent.cb["min"] is not None:
#         #         self.parent.fig.data[0].zmin = self.parent.cb["min"]
#         #     else:
#         #         self.parent.fig.data[0].zmin = np.amin(z[np.where(np.isfinite(z))])
#         #     if self.parent.cb["max"] is not None:
#         #         self.parent.fig.data[0].zmax = self.parent.cb["max"]
#         #     else:
#         #         self.parent.fig.data[0].zmax = np.amax(z[np.where(np.isfinite(z))])
#         # data = self.parent.fig.data[self.idim]
#         # coord = getattr(data, self.moving_coord[self.idim])
#         # coord = self.parent.slider_x[self.idim][change["new"]] * np.ones_like(coord)

#         setattr(self.parent.fig.data[self.idim], self.moving_coord[self.idim], self.parent.slider_x[self.idim][change["new"]] * np.ones_like(getattr(self.parent.fig.data[self.idim], self.moving_coord[self.idim])))
#         # print(self.slider_x[idim][self.slider[idim].value] * np.ones_like(self.fig.data[0].x))
#         self.parent.fig.data[self.idim].surfacecolor = z
#         return



class Slicer3d:

    def __init__(self, data, layout, input_data, axes,
                 value_name, cb):

        # Make a copy of the input data - Needed?
        self.input_data = input_data

        # Get the dimensions of the image to be displayed
        self.coords = self.input_data.coords
        self.labels = self.input_data.labels
        _, self.xcoord = get_coord_array(self.coords, self.labels, axes[-1])
        _, self.ycoord = get_coord_array(self.coords, self.labels, axes[-2])
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
        self.dim_keys = dict()
        self.moving_coord = ["z", "y", "x"]
        self.buttons = []
        for idim, ax in enumerate(axes):
            self.slider_dims.append(ax)
            self.slider_nx.append(self.shapes[ax])
            self.slider_x.append(self.coords[ax].values)
            self.dim_keys[str(ax)] = [ax, idim]
        self.nslices = len(self.slider_dims)

        # Initialise Figure and VBox objects
        self.fig = go.FigureWidget(data=data)#, layout=layout)
        self.fig.update_layout(scene = dict(
                    xaxis_title=layout["xaxis"]["title"],
                    yaxis_title=layout["yaxis"]["title"],
                    zaxis_title=layout["zaxis"]["title"]))
        self.vbox = [self.fig]

        # Initialise slider and label containers
        self.lab = []
        self.slider = []
        # Collect the remaining arguments
        self.value_name = value_name
        self.cb = cb
        # Default starting index for slider
        indx = 0

        # Now begin loop to construct sliders
        self.updates = []
        for i in range(len(self.slider_nx)):
            print(self.slider_dims[i], self.slider_nx[i])
            initval = self.slider_nx[i]//2
            # Add a label widget to display the value of the z coordinate
            self.lab.append(Label(value=str(self.slider_x[i][indx])))
            # Add an IntSlider to slide along the z dimension of the array
            self.slider.append(IntSlider(
                value=0,
                min=0,
                max=self.slider_nx[i] - 1,
                step=1,
                description=str(self.slider_dims[i]),
                continuous_update=True,
                readout=False
            ))
            # Add an observer to the slider
            # self.updates.append(Update3d(self, i, self.slider_dims[i]))
            # self.slider[i].observe(self.updates[-1].update_slice, names="value")
            self.slider[i].observe(self.update_slice3d, names="value")
            
            # Add coordinate name and unit
            title = Label(value=axis_label(self.coords[self.slider_dims[i]]))
            self.vbox.append(HBox([title, self.slider[i], self.lab[i]]))
            # self.updates[-1].update_slice({"new": initval})
            # self.update_slice3d({"new": initval. "owner":})
            self.slider[i].value = initval

        # # Call update_slice once to make the initial image
        # # if len(self.slider) > 0:
        # self.update_slice3d({"new": 0})
        # self.update_slice3dy(0)
        # self.update_slice3dz(0)
        self.vbox = VBox(self.vbox)
        self.vbox.layout.align_items = 'center'

        return

    # Define function to update slices
    def update_slice3d(self, change):
        print(change)
        print(change["owner"].description)
        dim_str = change["owner"].description
        # # The dimensions to be sliced have been saved in slider_dims
        # # Slice with first element to avoid modifying underlying dataset
        # self.lab[0].value = str(self.slider_x[0][self.slider[0].value])
        # vslice = self.input_data[self.slider_dims[0], self.slider[0].value]
        vslice = self.input_data
        # Then slice additional dimensions if needed
        # for idim in range(self.nslices):
        idim = self.dim_keys[dim_str][1]
        self.lab[idim].value = str(
            self.slider_x[idim][self.slider[idim].value])
        vslice = vslice[self.slider_dims[idim], self.slider[idim].value]

        z = vslice.values
        # print(z)

        # Check if dimensions of arrays agree, if not, plot the transpose
        zlabs = vslice.dims
        # if (zlabs[0] == self.xlabs[0]) and (zlabs[1] == self.ylabs[0]):
        #     z = z.T
        # Apply colorbar parameters
        if self.cb["log"]:
            with np.errstate(invalid="ignore", divide="ignore"):
                z = np.log10(z)
        # if (self.cb["min"] is not None) + (self.cb["max"] is not None) == 1:
        #     if self.cb["min"] is not None:
        #         self.fig.data[0].zmin = self.cb["min"]
        #     else:
        #         self.fig.data[0].zmin = np.amin(z[np.where(np.isfinite(z))])
        #     if self.cb["max"] is not None:
        #         self.fig.data[0].zmax = self.cb["max"]
        #     else:
        #         self.fig.data[0].zmax = np.amax(z[np.where(np.isfinite(z))])

        setattr(self.fig.data[idim], self.moving_coord[idim], self.slider_x[idim][change["new"]] * np.ones_like(getattr(self.parent.fig.data[self.idim], self.moving_coord[self.idim])))

        self.fig.data[0].x = self.slider_x[idim][self.slider[idim].value] * np.ones_like(self.fig.data[0].x)
        # print(self.slider_x[idim][self.slider[idim].value] * np.ones_like(self.fig.data[0].x))
        self.fig.data[0].surfacecolor = z
    #     return


    # # Define function to update slices
    # def update_slice3dy(self, change):
    #     # # The dimensions to be sliced have been saved in slider_dims
    #     # # Slice with first element to avoid modifying underlying dataset
    #     # self.lab[0].value = str(self.slider_x[0][self.slider[0].value])
    #     # vslice = self.input_data[self.slider_dims[0], self.slider[0].value]
    #     vslice = self.input_data
    #     # Then slice additional dimensions if needed
    #     idim = 1
    #     # for idim in range(self.nslices):
    #     self.lab[idim].value = str(
    #         self.slider_x[idim][self.slider[idim].value])
    #     vslice = vslice[self.slider_dims[idim], self.slider[idim].value]

    #     z = vslice.values

    #     # Check if dimensions of arrays agree, if not, plot the transpose
    #     zlabs = vslice.dims
    #     # if (zlabs[0] == self.xlabs[0]) and (zlabs[1] == self.ylabs[0]):
    #     #     z = z.T
    #     # Apply colorbar parameters
    #     if self.cb["log"]:
    #         with np.errstate(invalid="ignore", divide="ignore"):
    #             z = np.log10(z)
    #     if (self.cb["min"] is not None) + (self.cb["max"] is not None) == 1:
    #         if self.cb["min"] is not None:
    #             self.fig.data[1].zmin = self.cb["min"]
    #         else:
    #             self.fig.data[1].zmin = np.amin(z[np.where(np.isfinite(z))])
    #         if self.cb["max"] is not None:
    #             self.fig.data[1].zmax = self.cb["max"]
    #         else:
    #             self.fig.data[1].zmax = np.amax(z[np.where(np.isfinite(z))])
    #     self.fig.data[1].y = self.slider_x[idim][self.slider[idim].value] * np.ones_like(self.fig.data[1].y)
    #     self.fig.data[1].surfacecolor = z
    #     return

    # # Define function to update slices
    # def update_slice3dz(self, change):
    #     # # The dimensions to be sliced have been saved in slider_dims
    #     # # Slice with first element to avoid modifying underlying dataset
    #     # self.lab[0].value = str(self.slider_x[0][self.slider[0].value])
    #     # vslice = self.input_data[self.slider_dims[0], self.slider[0].value]
    #     vslice = self.input_data
    #     # Then slice additional dimensions if needed
    #     idim = 2
    #     # for idim in range(self.nslices):
    #     self.lab[idim].value = str(
    #         self.slider_x[idim][self.slider[idim].value])
    #     vslice = vslice[self.slider_dims[idim], self.slider[idim].value]

    #     z = vslice.values

    #     # Check if dimensions of arrays agree, if not, plot the transpose
    #     zlabs = vslice.dims
    #     # if (zlabs[0] == self.xlabs[0]) and (zlabs[1] == self.ylabs[0]):
    #     #     z = z.T
    #     # Apply colorbar parameters
    #     if self.cb["log"]:
    #         with np.errstate(invalid="ignore", divide="ignore"):
    #             z = np.log10(z)
    #     if (self.cb["min"] is not None) + (self.cb["max"] is not None) == 1:
    #         if self.cb["min"] is not None:
    #             self.fig.data[2].zmin = self.cb["min"]
    #         else:
    #             self.fig.data[2].zmin = np.amin(z[np.where(np.isfinite(z))])
    #         if self.cb["max"] is not None:
    #             self.fig.data[2].zmax = self.cb["max"]
    #         else:
    #             self.fig.data[2].zmax = np.amax(z[np.where(np.isfinite(z))])
    #     self.fig.data[2].z = self.slider_x[idim][self.slider[idim].value] * np.ones_like(self.fig.data[2].z)
    #     self.fig.data[2].surfacecolor = z
    #     return







def get_plotly_color(index=0):
    return DEFAULT_PLOTLY_COLORS[index % len(DEFAULT_PLOTLY_COLORS)]
