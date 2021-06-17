# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .formatters import make_formatter
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
    def __init__(
            self,
            scipp_obj_dict,
            labels=None,  # dim -> coord name
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
        self.masks = {}
        self.errorbars = {}

        # TODO use option to provide keys here
        array = next(iter(scipp_obj_dict.values()))

        self.name = list(scipp_obj_dict.keys())[0]
        self.dims = scipp_obj_dict[self.name].dims
        self.labels = {dim: dim for dim in self.dims}
        if labels is not None:
            self.labels.update(labels)
        self._formatters = {
            dim: make_formatter(array, self.labels[dim])
            for dim in array.dims
        }
        if positions:
            if not array.meta[positions].dims:
                raise ValueError(f"{positions} cannot be 0 dimensional"
                                 f" on input object\n\n{array}")
            else:
                self.position_dims = array.meta[positions].dims
        else:
            self.position_dims = None

        # Set cmap extend state: if we have sliders (= key "0" is found in
        # self.axes), then we need to extend.
        # We also need to extend if vmin or vmax are set.
        self.extend_cmap = "neither"
        if (len(self.dims) > view_ndims) or ((vmin is not None) and
                                             (vmax is not None)):
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

        # Get the colormap and normalization
        self.params["values"] = parse_params(globs=globs)
        self.params["masks"] = parse_params(params=masks,
                                            defaults={
                                                "cmap": "gray",
                                                "cbar": False,
                                                "under_color": None,
                                                "over_color": None
                                            },
                                            globs=masks_globs)

        for name, array in scipp_obj_dict.items():

            # Determine whether error bars should be plotted or not
            has_variances = array.variances is not None
            if name in self.errorbars:
                self.errorbars[name] &= has_variances
            else:
                self.errorbars[name] = has_variances

            # Save masks information
            self.masks[name] = [m for m in array.masks]
            self.masks[name] = {
                "color": self.params["masks"]["color"],
                "cmap": self.params["masks"]["cmap"],
                "names": {}
            }
            for m in array.masks:
                self.masks[name]["names"][m] = self.params["masks"]["show"]

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
        self.view.figure.initialize_toolbar(log_axis_buttons=self.dims)
        if self.profile is not None:
            self.profile.initialize_toolbar(log_axis_buttons=self.dims)
        self.controller.render(*args, **kwargs)
        if hasattr(self.view.figure, "fig"):
            self.fig = self.view.figure.fig
        if hasattr(self.view.figure, "ax"):
            self.ax = self.view.figure.ax

    def validate(self):
        """
        Validation checks before plotting.
        """
        multid_coord = self.model.get_multid_coord()

        # Protect against having a multi-dimensional coord along a slider axis
        for dim in self.dims:
            if dim in self.model.dims and (dim == multid_coord):
                raise DimensionError("A ragged coordinate cannot lie along "
                                     "a slider dimension, it must be one of "
                                     "the displayed dimensions.")

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
