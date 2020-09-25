# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .tools import parse_params, make_fake_coord, to_bin_edges, check_log_limits, to_bin_centers
from .widgets import PlotWidgets
from .._utils import name_with_unit, value_to_string
from .._scipp import core as sc

# Other imports
import numpy as np
import matplotlib.ticker as ticker


class PlotController:
    def __init__(self,
                 scipp_obj_dict=None,
                 axes=None,
                 masks=None,
                 cmap=None,
                 log=None,
                 vmin=None,
                 vmax=None,
                 color=None,
                 logx=False,
                 logy=False,
                 logz=False,
                 button_options=None,
                 positions=None,
                 errorbars=None):

        self.model = None
        self.panel = None
        self.profile = None
        self.view = None

        self.axes = axes
        self.logx = logx
        self.logy = logy
        self.logz = logz
        self.axparams = {}
        self.profile_axparams = {}
        self.slice_instead_of_rebin = {}

        # Parse parameters for values and masks
        self.params = {"values": {}, "masks": {}}
        globs = {
            "cmap": cmap,
            "log": log,
            "vmin": vmin,
            "vmax": vmax,
            "color": color
        }
        masks_globs = {"log": log, "vmin": vmin, "vmax": vmax}

        # Save the current profile dimension
        self.profile_dim = None
        # List mask names for each item
        self.masks = {}
        # Size of the slider coordinate arrays
        self.dim_to_shape = {}
        # Store coordinates of dimensions that will be in sliders
        self.coords = {}
        # Store coordinate min and max limits
        self.xlims = {}
        # Store labels for sliders
        self.labels = {}
        # Record which variables are histograms along which dimension
        self.histograms = {}
        # Axes tick formatters
        self.axformatter = {}
        # Axes tick locators
        self.axlocator = {}
        # Save if errorbars should be plotted for a given data entry
        self.errorbars = {}

        self.multid_coord = None

        if errorbars is not None:
            if isinstance(errorbars, bool):
                self.errorbars = {name: errorbars for name in scipp_obj_dict}
            elif isinstance(errorbars, dict):
                self.errorbars = errorbars
            else:
                raise TypeError("Unsupported type for argument "
                                "'errorbars': {}".format(type(errorbars)))

        # Get first item in dict and process dimensions.
        # Dimensions should be the same for all dict items.
        self.name = list(scipp_obj_dict.keys())[0]
        self._process_axes_dimensions(scipp_obj_dict[self.name],
                                      positions=positions)

        # Iterate through data arrays and collect parameters
        for name, array in scipp_obj_dict.items():

            # If non-dimension coord is requested as labels, replace name in
            # dims
            underlying_dim_to_label = {}
            array_dims = array.dims
            for dim in self.axes:
                if dim not in array_dims:
                    underlying_dim = array.coords[dim].dims[-1]
                    underlying_dim_to_label[underlying_dim] = dim
                    array_dims[array_dims.index(underlying_dim)] = dim

            # Get the colormap and normalization
            self.params["values"][name] = parse_params(globs=globs,
                                                       variable=array.data)

            self.params["masks"][name] = parse_params(params=masks,
                                                      defaults={
                                                          "cmap": "gray",
                                                          "cbar": False
                                                      },
                                                      globs=masks_globs)

            # Create a useful map from dim to shape
            self.dim_to_shape[name] = dict(zip(array_dims, array.shape))

            # Store coordinates of dimensions that will be in sliders
            self.coords[name] = {}
            # Store masks with correct dims
            self.masks[name] = {}
            # Store coordinate min and max limits
            self.xlims[name] = {}
            # Store labels for sliders
            self.labels[name] = {}
            # Store axis tick formatters
            self.axformatter[name] = {}
            # Store axis tick locators
            self.axlocator[name] = {}
            # Save information on histograms
            self.histograms[name] = {}

            # Iterate through axes and collect dimensions
            for dim in self.axes:
                self._collect_dim_shapes_and_lims(name, dim, array)

            # Collect masks
            for m, msk in array.masks.items():
                mask_dims = msk.dims
                for dim in mask_dims:
                    if dim not in self.axes:
                        mask_dims[mask_dims.index(
                            dim)] = underlying_dim_to_label[dim]
                self.masks[name][m] = sc.Variable(dims=mask_dims,
                                                  values=msk.values,
                                                  dtype=msk.dtype)

            # Determine whether error bars should be plotted or not
            has_variances = array.variances is not None
            if name in self.errorbars:
                self.errorbars[name] &= has_variances
            else:
                self.errorbars[name] = has_variances

        # Create control widgets (sliders and buttons).
        # Typically one set of slider/buttons for each dimension.
        self.widgets = PlotWidgets(controller=self,
                                   button_options=button_options,
                                   positions=positions)
        return

    def _ipython_display_(self):
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        return self.widgets._to_widget()

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
        return

    def _collect_dim_shapes_and_lims(self, name, dim, array):

        var, formatter, locator = self._axis_label_and_ticks(dim, array, name)

        if len(var.dims) > 1:
            self.multid_coord = dim

        # To allow for 2D coordinates, the histograms are
        # stored as dicts, with one key per dimension of the coordinate
        # self.slider_shape[name][dim] = {}
        dim_shape = None
        self.histograms[name][dim] = {}
        for i, d in enumerate(var.dims):
            self.histograms[name][dim][d] = self.dim_to_shape[name][
                d] == var.shape[i] - 1
            if d == dim:
                dim_shape = var.shape[i]

        if self.histograms[name][dim][dim]:
            self.coords[name][dim] = var
        else:
            self.coords[name][dim] = to_bin_edges(var, dim)

        # The limits for each dimension
        self.xlims[name][dim] = np.array(
            [sc.min(var).value, sc.max(var).value], dtype=np.float)
        if sc.is_sorted(var, dim, order='descending'):
            self.xlims[name][dim] = np.flip(self.xlims[name][dim]).copy()
        # The tick formatter and locator
        self.axformatter[name][dim] = formatter
        self.axlocator[name][dim] = locator

        # Small correction if xmin == xmax
        if self.xlims[name][dim][0] == self.xlims[name][dim][1]:
            if self.xlims[name][dim][0] == 0.0:
                self.xlims[name][dim] = [-0.5, 0.5]
            else:
                dx = 0.5 * abs(self.xlims[name][dim][0])
                self.xlims[name][dim][0] -= dx
                self.xlims[name][dim][1] += dx
        # For xylims, if coord is not bin-edge, we make artificial
        # bin-edge. This is simpler than finding the actual index of
        # the smallest and largest values and computing a bin edge from
        # the neighbours.
        if not self.histograms[name][dim][dim] and dim_shape > 1:
            dx = 0.5 * (self.xlims[name][dim][1] -
                        self.xlims[name][dim][0]) / float(dim_shape - 1)
            self.xlims[name][dim][0] -= dx
            self.xlims[name][dim][1] += dx

        self.xlims[name][dim] = sc.Variable([dim],
                                            values=self.xlims[name][dim],
                                            unit=var.unit)

        self.labels[name][dim] = name_with_unit(var=var)
        return

    def _axis_label_and_ticks(self, dim, data_array, name):
        """
        Get dimensions from requested axis.
        Also retun axes tick formatters and locators.
        """

        # Create some default axis tick formatter, depending on whether log
        # for that axis will be True or False
        formatter = {
            False: ticker.ScalarFormatter(),
            True: ticker.LogFormatterSciNotation()
        }
        locator = {False: ticker.AutoLocator(), True: ticker.LogLocator()}

        var = None

        if dim in data_array.coords:

            dim_coord_dim = data_array.coords[dim].dims[-1]
            tp = data_array.coords[dim].dtype

            if tp == sc.dtype.vector_3_float64:
                var = make_fake_coord(dim,
                                      self.dim_to_shape[name][dim],
                                      unit=data_array.coords[dim].unit)
                form = ticker.FuncFormatter(lambda val, pos: "(" + ",".join([
                    value_to_string(item, precision=2)
                    for item in data_array.coords[dim].values[int(val)]
                ]) + ")" if (int(val) >= 0 and int(val) < self.dim_to_shape[
                    name][dim]) else "")
                formatter.update({False: form, True: form})
                locator[False] = ticker.MaxNLocator(integer=True)

            elif tp == sc.dtype.string:
                var = make_fake_coord(dim,
                                      self.dim_to_shape[name][dim],
                                      unit=data_array.coords[dim].unit)
                form = ticker.FuncFormatter(lambda val, pos: data_array.coords[
                    dim].values[int(val)] if (int(val) >= 0 and int(
                        val) < self.dim_to_shape[name][dim]) else "")
                formatter.update({False: form, True: form})
                locator[False] = ticker.MaxNLocator(integer=True)

            elif dim != dim_coord_dim:
                # non-dimension coordinate
                if dim_coord_dim in data_array.coords:
                    coord = data_array.coords[dim_coord_dim]
                    var = sc.Variable([dim],
                                      values=coord.values,
                                      variances=coord.variances,
                                      unit=coord.unit,
                                      dtype=sc.dtype.float64)
                else:
                    var = make_fake_coord(dim, self.dim_to_shape[name][dim])
                form = ticker.FuncFormatter(
                    lambda val, pos: value_to_string(data_array.coords[
                        dim].values[np.abs(var.values - val).argmin()]))
                formatter.update({False: form, True: form})

            else:
                var = data_array.coords[dim].astype(sc.dtype.float64)

        else:
            # dim not found in data_array.coords
            var = make_fake_coord(dim, self.dim_to_shape[name][dim])

        return var, formatter, locator

    def rescale_to_data(self, button=None):
        """
        Automatically rescale the y axis (1D plot) or the colorbar (2D+3D
        plots) to the minimum and maximum value inside the currently displayed
        data slice.
        """
        data_min, data_max = self.model.rescale_to_data()
        param_min = self.params["values"][self.name]["vmin"]
        param_max = self.params["values"][self.name]["vmax"]
        vmin = param_min if param_min is not None else data_min
        vmax = param_max if param_max is not None else data_max
        vmin, vmax = check_log_limits(
            vmin=vmin, vmax=vmax, log=self.params["values"][self.name]["log"])
        self.view.rescale_to_data(vmin, vmax)
        if self.panel is not None:
            self.panel.rescale_to_data(vmin=vmin,
                                       vmax=vmax,
                                       mask_info=self._get_mask_info())

    def update_axes(self, change=None):
        """
        This function is called when a dimension that is displayed along a
        given axis is changed. This happens for instance when we want to
        flip/transpose a 2D image, or display a new dimension along the x-axis
        in a 1D plot.
        This functions gather the relevant parameters about the axes currently
        selected for display, and then offloads the computation of the new
        state to the model. If then gets the updated data back from the model
        and sends it over to the view for display.
        """
        self.axparams = self._get_axes_parameters()
        self.model.update_axes(self.axparams)
        self.view.update_axes(axparams=self.axparams,
                              axformatter=self.axformatter[self.name],
                              axlocator=self.axlocator[self.name],
                              logx=self.logx,
                              logy=self.logy)
        if self.panel is not None:
            self.panel.update_axes(axparams=self.axparams)
        if self.profile is not None:
            self._toggle_profile_view()
        self.update_data()
        self.rescale_to_data()

    def update_data(self, change=None):
        """
        This function is called when the data in the displayed 1D plot or 2D
        image is to be updated. This happens for instance when we move a slider
        which is navigating an additional dimension. It is also always
        called when update_axes is called since the displayed data needs to be
        updated when the axes have changed.
        """

        if change is not None:
            owner_dim = change["owner"].dim

            # Update readout label
            ind = self.widgets.slider[owner_dim].value
            self.widgets.slider_readout[owner_dim].value = value_to_string(
                to_bin_centers(
                    self.coords[self.name][owner_dim][owner_dim, ind:ind + 2],
                    owner_dim).values[0])

        slices = {}
        info = {"slice_label": ""}
        # Slice along dimensions with active sliders
        for dim, val in self.widgets.slider.items():
            if not val.disabled:
                slices[dim] = self._make_slice_dict(val.value, dim)
                info["slice_label"] = "{},{}:{}-{}".format(
                    info["slice_label"], dim,
                    slices[dim]["location"] - 0.5 * slices[dim]["thickness"],
                    slices[dim]["location"] + 0.5 * slices[dim]["thickness"])

        new_values = self.model.update_data(slices,
                                            mask_info=self._get_mask_info())
        self.view.update_data(new_values)
        if self.panel is not None:
            self.panel.update_data(info)
        if self.profile_dim is not None:
            self.profile.update_slice_area(slices[self.profile_dim])

    def toggle_mask(self, change):
        """
        Hide or show a given mask.
        """
        self.view.toggle_mask(change)

    def _get_axes_parameters(self):
        """
        Gather the information (dimensions, limits, etc...) about the (x, y, z)
        axes that are displayed on the plots.
        """
        axparams = {}
        for dim, button in self.widgets.buttons.items():
            if self.widgets.slider[dim].disabled:
                but_val = button.value.lower()
                xmin = np.Inf
                xmax = np.NINF
                for name in self.xlims:
                    xlims = self.xlims[name][dim].values
                    xmin = min(xmin, xlims[0])
                    xmax = max(xmax, xlims[1])
                axparams[but_val] = {
                    "lims": [xmin, xmax],
                    "log": getattr(self, "log{}".format(but_val)),
                    "hist": {
                        name: self.histograms[name][dim][dim]
                        for name in self.histograms
                    },
                    "dim": dim,
                    "label": self.labels[self.name][dim]
                }
                # Safety check for log axes
                axparams[but_val]["lims"] = check_log_limits(
                    lims=axparams[but_val]["lims"],
                    log=axparams[but_val]["log"])

        return axparams

    def _get_mask_info(self):
        """
        Get information of masks such as their names and whether they should be
        displayed.
        """
        mask_info = {}
        for name in self.widgets.mask_checkboxes:
            mask_info[name] = {
                m: chbx.value
                for m, chbx in self.widgets.mask_checkboxes[self.name].items()
            }
        return mask_info

    def keep_line(self, target=None, name=None, color=None, line_id=None):
        """
        Get a message from either a panel (1d plot) or a view (picking) to keep
        the currently displayed line or profile.
        Send the message to the appropriate target: either the view1d or the
        profile view.
        """
        if name is None:
            name = self.name
        if target == "profile":
            self.profile.keep_line(name=name, color=color, line_id=line_id)
        else:
            self.view.keep_line(name=name, color=color, line_id=line_id)

    def remove_line(self, target=None, line_id=None):
        """
        Get a message from either a panel (1d plot) or a view (picking) to
        remove a given line or profile.
        Send the message to the appropriate target: either the view1d or the
        profile view.
        """
        if target == "profile":
            self.profile.remove_line(line_id=line_id)
        else:
            self.view.remove_line(line_id=line_id)

    def _toggle_profile_view(self, owner=None):
        """
        Show or hide the 1d plot displaying the profile along an additional
        dimension.
        As we do this, we also collect some information on the limits of the
        view area to be displayed.
        """
        if owner is None:
            self.profile_dim = None
            visible = False
            for but in self.widgets.profile_button.values():
                but.button_style = ""
        else:
            self.profile_dim = owner.dim
            self.profile_axparams.clear()
            if owner.button_style == "info":
                owner.button_style = ""
                visible = False
            else:
                owner.button_style = "info"
                for dim, but in self.widgets.profile_button.items():
                    if dim != self.profile_dim:
                        but.button_style = ""
                visible = True

            if visible:
                xmin = np.Inf
                xmax = np.NINF
                for name in self.xlims:
                    xlims = self.xlims[name][self.profile_dim].values
                    xmin = min(xmin, xlims[0])
                    xmax = max(xmax, xlims[1])
                self.profile_axparams = {
                    "x": {
                        "lims": [xmin, xmax],
                        "log": False,
                        "hist": {
                            name: self.histograms[name][self.profile_dim][
                                self.profile_dim]
                            for name in self.histograms
                        },
                        "dim": self.profile_dim,
                        "label": self.labels[self.name][self.profile_dim]
                    }
                }
                # Safety check for log axes
                if self.profile_axparams["x"]["log"] and (
                        self.profile_axparams["x"]["lims"][0] <= 0):
                    self.profile_axparams["x"]["lims"][
                        0] = 1.0e-03 * self.profile_axparams["x"]["lims"][1]
                self.profile.update_axes(
                    axparams=self.profile_axparams,
                    axformatter=self.axformatter[self.name],
                    axlocator=self.axlocator[self.name],
                    logx=False,
                    logy=False)
            else:
                self.view.reset_profile()

        self.profile.toggle_view(visible=visible)
        self.toggle_hover_visibility(False)
        self.view.update_profile_connection(visible=visible)

        if visible:
            self.profile.update_slice_area({
                "location":
                self.widgets.slider[self.profile_dim].value,
                "thickness":
                self.widgets.thickness_slider[self.profile_dim].value
            })

        return

    def update_profile(self, xdata=None, ydata=None):
        """
        This is called from a mouse move event, which requires an update of the
        currently displayed profile.
        We gather the information on which dims should be sliced by the model,
        ask the model to slice down the data, and send the new data returned by
        the model to the profile view.
        """
        slices = {}
        # Slice all dimensions apart from the profile dim
        for dim, val in self.widgets.slider.items():
            if dim != self.profile_dim:
                slices[dim] = self._make_slice_dict(val.value, dim)

        # Get new values from model
        new_values = self.model.update_profile(xdata=xdata,
                                               ydata=ydata,
                                               slices=slices,
                                               axparams=self.profile_axparams,
                                               mask_info=self._get_mask_info())
        # Send new values to the profile view
        self.profile.update_data(new_values)

    def toggle_hover_visibility(self, value):
        """
        Show/hide the profile view depending on the value of the profile button
        in the widgets.
        """
        self.profile.toggle_hover_visibility(value)

    def _make_slice_dict(self, ind, dim):
        return {
            "index":
            ind,
            "location":
            to_bin_centers(self.coords[self.name][dim][dim, ind:ind + 2],
                           dim).values[0],
            "thickness":
            self.widgets.thickness_slider[dim].value
        }
