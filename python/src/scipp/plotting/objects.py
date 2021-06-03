# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .tools import parse_params
from .._scipp.core import DimensionError


class PlotDict():
    """
    The Plot object is used as output for the plot command.
    It is a small wrapper around python dict, with an `_ipython_display_`
    representation.
    The dict will contain one entry for each entry in the input supplied to
    the plot function.
    More functionalities can be added in the future.
    """
    def __init__(self, *args, **kwargs):
        self._items = dict(*args, **kwargs)

    def __getitem__(self, key):
        return self._items[key]

    def __len__(self):
        return len(self._items)

    def keys(self):
        return self._items.keys()

    def values(self):
        return self._items.values()

    def items(self):
        return self._items.items()

    def _ipython_display_(self):
        """
        IPython display representation for Jupyter notebooks.
        """
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        """
        Return plot contents into a single VBocx container
        """
        import ipywidgets as ipw
        contents = []
        for item in self.values():
            if item is not None:
                contents.append(item._to_widget())
        return ipw.VBox(contents)

    def show(self):
        """
        """
        for item in self.values():
            item.show()

    def hide_widgets(self):
        for item in self.values():
            item.hide_widgets()

    def close(self):
        """
        Close all plots in dict, making them static.
        """
        for item in self.values():
            item.close()

    def redraw(self):
        """
        Redraw/update  all plots in dict.
        """
        for item in self.values():
            item.redraw()

    def set_draw_no_delay(self, value):
        """
        When set to True, try to update plots as soon as possible.
        This is useful in the case where one wishes to update the plot inside
        a loop (e.g. when listening to a data stream).
        The plot update is then slightly more expensive than when it is set to
        False.
        """
        for item in self.values():
            item.set_draw_no_delay(value)


class Plot:
    """
    Base class for plot objects. It uses the Model-View-Controller pattern to
    separate displayed figures and user-interaction via widgets from the
    operations performed on the data.

    It contains:
      - a `PlotModel`: contains the input data and performs all the heavy
          calculations.
      - a `PlotView`: contains a `PlotFigure` which is displayed and handles
          communications between `PlotController` and `PlotFigure`, as well as
          updating the `PlotProfile` depending on signals captured by the
          `PlotFigure`.
      - some `PlotWidgets`: a base collection of sliders and buttons which
          provide interactivity to the user.
      - a `PlotPanel` (optional): an extra set of widgets which is not part of
          the base `PlotWidgets`.
      - a `PlotProfile` (optional): used to display a profile plot under the
          `PlotFigure` to show one of the slider dimensions as a 1 dimensional
          line plot.
      - a `PlotController`: handles all the communication between all the
          pieces above.
    """
    def __init__(self,
                 scipp_obj_dict,
                 axes=None,
                 errorbars=None,
                 cmap=None,
                 norm=False,
                 vmin=None,
                 vmax=None,
                 color=None,
                 masks=None,
                 positions=None,
                 view_ndims=None):

        self.controller = None
        self.model = None
        self.panel = None
        self.profile = None
        self.view = None
        self.widgets = None

        self.show_widgets = True
        self.view_ndims = view_ndims

        # Shortcut access to the underlying figure for easier modification
        self.fig = None
        self.ax = None

        # Get first item in dict and process dimensions.
        # Dimensions should be the same for all dict items.
        self.axes = None
        self.masks = {}
        self.errorbars = {}
        self.dim_to_shape = {}
        self.coord_shapes = {}
        self.dim_label_map = {}
        self.position_dims = None

        self.name = list(scipp_obj_dict.keys())[0]
        self._process_axes_dimensions(scipp_obj_dict[self.name],
                                      axes=axes,
                                      view_ndims=view_ndims,
                                      positions=positions)

        # Set cmap extend state: if we have sliders (= key "0" is found in
        # self.axes), then we need to extend.
        # We also need to extend if vmin or vmax are set.
        self.extend_cmap = "neither"
        if (0 in self.axes) or ((vmin is not None) and (vmax is not None)):
            self.extend_cmap = "both"
        elif vmin is not None:
            self.extend_cmap = "min"
        elif vmax is not None:
            self.extend_cmap = "max"

        # Scan the input data and collect information
        self.params = {"values": {}, "masks": {}}
        globs = {
            "cmap": cmap,
            "norm": norm,
            "vmin": vmin,
            "vmax": vmax,
            "color": color
        }
        masks_globs = {"norm": norm, "vmin": vmin, "vmax": vmax}

        if errorbars is not None:
            if isinstance(errorbars, bool):
                self.errorbars = {name: errorbars for name in scipp_obj_dict}
            elif isinstance(errorbars, dict):
                self.errorbars = errorbars
            else:
                raise TypeError("Unsupported type for argument "
                                "'errorbars': {}".format(type(errorbars)))

        for name, array in scipp_obj_dict.items():

            # Get the colormap and normalization
            self.params["values"][name] = parse_params(globs=globs,
                                                       variable=array.data,
                                                       name=name)

            self.params["masks"][name] = parse_params(params=masks,
                                                      defaults={
                                                          "cmap": "gray",
                                                          "cbar": False,
                                                          "under_color": None,
                                                          "over_color": None
                                                      },
                                                      globs=masks_globs)

            # If non-dimension coord is requested as labels, replace name in
            # dims
            array_dims = array.dims
            for dim in self.axes.values():
                if dim not in array_dims:
                    array_dims[array_dims.index(self.dim_label_map[dim])] = dim

            # Create a useful map from dim to shape
            self.dim_to_shape[name] = dict(zip(array_dims, array.shape))
            # TODO: once Dim has been replaced by strings, the str(dim) can
            # then here be replaced by dim
            self.coord_shapes[name] = {
                str(dim): coord.shape
                for dim, coord in array.coords.items()
            }

            # Add shapes for dims that have no coord in the original data.
            # They will be replaced by fake coordinates in the model.
            for dim in array_dims:
                if dim not in self.coord_shapes[name]:
                    self.coord_shapes[name][dim] = [
                        self.dim_to_shape[name][dim]
                    ]

            # Determine whether error bars should be plotted or not
            has_variances = array.variances is not None
            if name in self.errorbars:
                self.errorbars[name] &= has_variances
            else:
                self.errorbars[name] = has_variances

            # Save masks information
            self.masks[name] = [m for m in array.masks]
            self.masks[name] = {
                "color": self.params["masks"][name]["color"],
                "cmap": self.params["masks"][name]["cmap"],
                "names": {}
            }
            for m in array.masks:
                self.masks[name]["names"][m] = self.params["masks"][name][
                    "show"]

    def _ipython_display_(self):
        """
        IPython display representation for Jupyter notebooks.
        """
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        """
        Get the SciPlot object as an `ipywidget`.
        """
        import ipywidgets as ipw
        widget_list = [self.view._to_widget()]
        if self.profile is not None:
            widget_list.append(self.profile._to_widget())
        if self.show_widgets:
            widget_list.append(self.widgets._to_widget())
        if self.panel is not None and self.show_widgets:
            widget_list.append(self.panel._to_widget())

        return ipw.VBox(widget_list)

    def hide_widgets(self):
        """
        Hide widgets for 1d and 2d (matplotlib) figures
        """
        self.show_widgets = False if self.view_ndims < 3 else True

    def close(self):
        """
        Send close signal to the view.
        """
        self.view.close()

    def show(self):
        """
        Call the show() method of a matplotlib figure.
        """
        self.view.show()

    def render(self, *args, **kwargs):
        """
        Perform some initial calls to render the figure once all components
        have been created.
        """
        self.controller.render(*args, **kwargs)
        if hasattr(self.view.figure, "fig"):
            self.fig = self.view.figure.fig
        if hasattr(self.view.figure, "ax"):
            self.ax = self.view.figure.ax

    def _process_axes_dimensions(self,
                                 array=None,
                                 axes=None,
                                 view_ndims=None,
                                 positions=None):
        """
        Assign dimensions of input object to figure axes.
        If `axes` is not specified, the dimensions are assigned in the order
        they appear in the input object.
        """

        if positions:
            if not array.meta[positions].dims:
                raise ValueError(f"{positions} cannot be 0 dimensional"
                                 f" on input object\n\n{array}")
            else:
                self.position_dims = array.meta[positions].dims

        array_dims = array.dims
        self.ndim = len(array_dims)

        base_axes = ["xyz"[i] for i in range(view_ndims)]

        # Process axes dimensions
        self.axes = {}
        for i, dim in enumerate(array_dims[::-1]):
            if positions is not None:
                if (dim == positions) or (dim in array.meta[positions].dims):
                    key = "xyz"[("x" in self.axes) + ("y" in self.axes) +
                                ("z" in self.axes)]
                else:
                    key = i - ("x" in self.axes)
            else:
                if i < view_ndims:
                    key = base_axes[i]
                else:
                    key = i - view_ndims
            self.axes[key] = dim

        # Replace axes with supplied axes dimensions
        supplied_axes = {}
        if axes is not None:
            for dim in axes.values():
                if (dim not in self.axes.values()) and (dim not in array.meta):
                    raise RuntimeError("Requested dimension was not found in "
                                       "input data: {}".format(dim))
            supplied_axes.update(axes)
        if positions is not None and (positions not in self.axes.values()):
            supplied_axes.update({"x": positions})

        for key, dim in supplied_axes.items():
            dim_list = list(self.axes.values())
            key_list = list(self.axes.keys())
            underlying_dim = array.meta[dim].dims[
                -1] if dim in array.meta else dim
            if dim in dim_list:
                ind = dim_list.index(dim)
            else:
                # Non-dimension coordinate
                self.dim_label_map[underlying_dim] = dim
                self.dim_label_map[dim] = underlying_dim
                ind = dim_list.index(underlying_dim)
            self.axes[key_list[ind]] = self.axes[key]
            self.axes[key] = underlying_dim  # dim

    def validate(self):
        """
        Validation checks before plotting.
        """
        multid_coord = self.model.get_multid_coord()

        # Protect against having a multi-dimensional coord along a slider axis
        for ax, dim in self.axes.items():
            if isinstance(ax, int) and (dim == multid_coord):
                raise DimensionError("A ragged coordinate cannot lie along "
                                     "a slider dimension, it must be one of "
                                     "the displayed dimensions.")

        # Protect against duplicate entries in axes
        if len(self.axes.values()) != len(set(self.axes.values())):
            raise DimensionError("Duplicate entry in axes: {}".format(
                self.axes))

        # Protect against ill-formed data where multi-dimensional coord does
        # not apply to inner dimension
        if multid_coord is not None:
            multid_coord_dims = self.model.get_data_coord(
                self.name, multid_coord)[0].dims
            if (multid_coord in multid_coord_dims) and (multid_coord !=
                                                        multid_coord_dims[-1]):
                raise DimensionError(
                    "Plot input is ill-constructed. "
                    "When using multi-dimensional coordinates, the named "
                    "dimension of the coordinate must be the same as the "
                    "inner/last dim. "
                    "Here the named dimension is {}, "
                    "while the inner dim is {}.".format(
                        multid_coord, multid_coord_dims[-1]))

    def savefig(self, filename=None):
        """
        Save plot to file.
        Possible file extensions are `.jpg`, `.png` and `.pdf`.
        The default directory for writing the file is the same as the
        directory where the script or notebook is running.
        """
        self.view.savefig(filename=filename)

    def redraw(self):
        """
        Redraw the plot. Use this to update a figure when the underlying data
        has been modified.
        """
        self.controller.redraw()

    def set_draw_no_delay(self, value):
        """
        When set to True, try to update plots as soon as possible.
        This is useful in the case where one wishes to update the plot inside
        a loop (e.g. when listening to a data stream).
        The plot update is then slightly more expensive than when it is set to
        False.
        """
        self.view.set_draw_no_delay(value)
        if self.profile is not None:
            self.profile.set_draw_no_delay(value)
