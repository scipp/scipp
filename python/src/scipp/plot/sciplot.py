# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .tools import parse_params
from .._scipp import core as sc
import ipywidgets as ipw


class SciPlot:
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

        # Get first item in dict and process dimensions.
        # Dimensions should be the same for all dict items.
        self.axes = None
        self.masks = {}
        self.errorbars = {}
        self.dim_to_shape = {}
        self.coord_shapes = {}
        self.dim_label_map = {}

        self.name = list(scipp_obj_dict.keys())[0]
        self._process_axes_dimensions(scipp_obj_dict[self.name],
                                      axes=axes,
                                      view_ndims=view_ndims,
                                      positions=positions)

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
                                                          "cbar": False
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
            self.coord_shapes[name] = {
                dim: array.coords[dim].shape
                for dim in array.coords
            }

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
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        widget_list = []
        for item in [self.view, self.profile, self.widgets, self.panel]:
            if item is not None:
                widget_list.append(item._to_widget())
        return ipw.VBox(widget_list)

    def _process_axes_dimensions(self,
                                 array=None,
                                 axes=None,
                                 view_ndims=None,
                                 positions=None):

        array_dims = array.dims
        self.ndim = len(array_dims)

        base_axes = ["xyz"[i] for i in range(view_ndims)]

        # Process axes dimensions
        self.axes = {}
        for i, dim in enumerate(array_dims[::-1]):
            if i < view_ndims:
                key = base_axes[i]
            else:
                key = i - view_ndims
            self.axes[key] = dim

        if axes is not None:
            # Axes can be incomplete
            for key, ax in axes.items():
                if self.axes[key] != ax:
                    dim_list = list(self.axes.values())
                    key_list = list(self.axes.keys())
                    if ax in dim_list:
                        ind = dim_list.index(ax)
                    else:
                        # Non-dimension coordinate
                        underlying_dim = array.coords[ax].dims[-1]
                        dim = sc.Dim(ax)
                        self.dim_label_map[underlying_dim] = dim
                        self.dim_label_map[dim] = underlying_dim
                        ind = dim_list.index(underlying_dim)
                    self.axes[key_list[ind]] = self.axes[key]
                    self.axes[key] = dim

        # Replace positions in axes if positions set
        if positions is not None:
            self.axes[self.axes.index(
                array.coords[positions].dims[0])] = sc.Dim(positions)

        # Protect against duplicate entries in axes
        if len(self.axes) != len(set(self.axes)):
            raise RuntimeError("Duplicate entry in axes: {}".format(self.axes))

    def savefig(self, filename=None):
        self.view.savefig(filename=filename)
