# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .tools import parse_params

import ipywidgets as ipw


class SciPlot:
    def __init__(self,
                 scipp_obj_dict,
                 axes=None,
                 errorbars=None,
                 cmap=None,
                 log=False,
                 vmin=None,
                 vmax=None,
                 color=None,
                 masks=None,
                 positions=None):

        self.controller = None
        self.model = None
        self.panel = None
        self.profile = None
        self.view = None
        self.widgets = None

        # Get first item in dict and process dimensions.
        # Dimensions should be the same for all dict items.
        self.axes = axes
        self.mask_names = {}
        # self.underlying_dim_to_label = {}
        self.dim_label_map = {}
        self.name = list(scipp_obj_dict.keys())[0]
        self._process_axes_dimensions(scipp_obj_dict[self.name],
                                      positions=positions)

        # Scan the input data and collect information
        self.params = {"values": {}, "masks": {}}
        globs = {
            "cmap": cmap,
            "log": log,
            "vmin": vmin,
            "vmax": vmax,
            "color": color
        }
        masks_globs = {"log": log, "vmin": vmin, "vmax": vmax}

        self.errorbars = {}
        self.dim_to_shape = {}
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
                                                       variable=array.data)

            self.params["masks"][name] = parse_params(params=masks,
                                                      defaults={
                                                          "cmap": "gray",
                                                          "cbar": False
                                                      },
                                                      globs=masks_globs)


            # If non-dimension coord is requested as labels, replace name in
            # dims
            # underlying_dim_to_label = {}
            array_dims = array.dims
            for dim in self.axes:
                if dim not in array_dims:
                    # underlying_dim = array.coords[dim].dims[-1]
                    # underlying_dim_to_label[underlying_dim] = dim
                    array_dims[array_dims.index(self.dim_label_map[dim])] = dim

            # Create a useful map from dim to shape
            self.dim_to_shape[name] = dict(zip(array_dims, array.shape))

            # Determine whether error bars should be plotted or not
            has_variances = array.variances is not None
            if name in self.errorbars:
                self.errorbars[name] &= has_variances
            else:
                self.errorbars[name] = has_variances

            # Save mask names
            self.mask_names[name] = [m for m in array.masks]




    def _ipython_display_(self):
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        widget_list = []
        for item in [self.view, self.profile, self.widgets, self.panel]:
            if item is not None:
                widget_list.append(item._to_widget())
        return ipw.VBox(widget_list)



    def _process_axes_dimensions(self, array, positions=None):

        # Process axes dimensions
        if self.axes is None:
            self.axes = array.dims
        # Replace positions in axes if positions set
        if positions is not None:
            self.axes[self.axes.index(
                array.coords[positions].dims[0])] = positions

        # Convert to Dim objects
        for i in range(len(self.axes)):
            if isinstance(self.axes[i], str):
                self.axes[i] = sc.Dim(self.axes[i])

        # Protect against duplicate entries in axes
        if len(self.axes) != len(set(self.axes)):
            raise RuntimeError("Duplicate entry in axes: {}".format(self.axes))
        self.ndim = len(self.axes)

        # Make a map from non-dimension dim label to dimension dim.
        # The map goes both from non-dimension label to dimension label and
        # vice versa.
        for dim in self.axes:
            if dim not in array.dims:
                underlying_dim = array.coords[dim].dims[-1]
                # self.underlying_dim_to_label[array.coords[dim].dims[-1]] = dim
                self.dim_label_map[underlying_dim] = dim
                self.dim_label_map[dim] = underlying_dim

        return














    def savefig(self, filename=None):
        self.view.savefig(filename=filename)

    def _connect_controller_members(self):
        self.controller.model = self.model
        self.controller.panel = self.panel
        self.controller.profile = self.profile
        self.controller.view = self.view

    def as_static(self, keep_widgets=False):
        self.controller.model = None
        self.model = None
        if not keep_widgets:
            self.controller.panel = None
            self.controller.profile = None
            self.controller.view = None
            self.controller = None
            self.panel = None
            self.profile = None
            return self.view.figure
        else:
            return self
