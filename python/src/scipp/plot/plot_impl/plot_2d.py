# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from typing import List

from scipp import config
from scipp.plot.render import render_plot
from scipp.plot.slicer import Slicer
from scipp.plot.tools import centers_to_edges, edges_to_centers, parse_params
from scipp.utils import name_with_unit
from scipp._scipp.core import Variable, histogram as scipp_histogram
from .plot_request import PlotRequest, TwoDPlotKwargs

# Other imports
import numpy as np
import ipywidgets as widgets
import matplotlib.pyplot as plt
import warnings


def plot_2d(to_plot: List[PlotRequest]):
    """
    Plot a 2D slice through a N dimensional dataset. For every dimension above
    2, a slider is created to adjust the position of the slice in that
    particular dimension.
    """
    # This check is duplicated to assist the IDE to deduct we are using a scalar and not a list
    reference_elem = to_plot[0]

    assert isinstance(reference_elem.user_kwargs, TwoDPlotKwargs)

    fig, axes, cax = _get_mpl_axis(to_plot)

    sliced = []
    for requested, i_axis, i_cax in zip(to_plot, axes, cax):
        sv = Slicer2d(requested, fig=fig, ax=i_axis, cax=i_cax)
        sliced.append(sv.members)

    if reference_elem.user_kwargs.mpl_axes is None:
        render_plot(figure=fig, widgets=sv.vbox, filename=reference_elem.user_kwargs.filename)

    return sliced


def _get_mpl_axis(to_plot: List[PlotRequest]):
    # Get or create matplotlib axes
    is_subplot = len(to_plot) > 1
    has_any_variances = any(i.user_kwargs.variances for i in to_plot)
    reference_elem = to_plot[0]

    fig = None

    cax = [[None, None] if has_any_variances else [None] for _ in to_plot]

    if reference_elem.user_kwargs.mpl_axes is not None:
        if is_subplot:
            # Passing in MPL axis and the data structs behind it need completing.
            # However, a discussion on dropping MPL axis vs implementing this needs to happen
            # We can come back and implement this as / when we need it
            raise NotImplementedError("Passing in custom MPL axis is not supported with 2d subplots")

        if isinstance(reference_elem.user_kwargs.mpl_axes, dict):
            axes = [None, None]
            for key, val in reference_elem.user_kwargs.mpl_axes.items():
                if key == "ax" or key == "ax_values":
                    axes[0] = val
                if key == "cax" or key == "cax_values":
                    cax[0] = val
                if key == "ax_variances":
                    axes[1] = val
                if key == "cax_variances":
                    cax[1] = val
        else:
            # Case where only a single axis is passed in
            axes = [reference_elem.user_kwargs.mpl_axes]
    else:
        nrows = len(to_plot) if isinstance(to_plot, list) else 1
        ncols = 2 if has_any_variances else 1
        fig, axes = plt.subplots(
            nrows=nrows, ncols=ncols,
            figsize=(config.plot.width / config.plot.dpi,
                     config.plot.height / ncols / config.plot.dpi),
            dpi=config.plot.dpi)
        # Pack into list if we got a scalar axis returned
        if has_any_variances:
            axes = [[col1, col2] for col1, col2 in axes]
        elif nrows == 1 and ncols == 1:
            axes = [[axes]]
        else:
            axes = [[col] for col in axes]
    return fig, axes, cax


class Slicer2d(Slicer):
    def __init__(self, request: PlotRequest, ax: List, cax: List, fig):
        super().__init__(scipp_obj_dict=request.scipp_objs,
                         axes=request.user_kwargs.axes,
                         values=request.user_kwargs.values,
                         variances=request.user_kwargs.variances,
                         masks=request.user_kwargs.masks,
                         cmap=request.user_kwargs.cmap,
                         log=request.user_kwargs.log,
                         vmin=request.user_kwargs.vmin,
                         vmax=request.user_kwargs.vmax,
                         color=request.user_kwargs.color,
                         aspect=request.user_kwargs.aspect,
                         button_options=['X', 'Y'])

        self._request = request
        self._user_kwargs = request.user_kwargs
        self.fig = fig

        self.members.update({"images": {}, "colorbars": {}})
        self.extent = {"x": [1, 2], "y": [1, 2]}
        self.logx = self._user_kwargs.logx or self._user_kwargs.logxy
        self.logy = self._user_kwargs.logy or self._user_kwargs.logxy
        self.global_vmin = np.Inf
        self.global_vmax = np.NINF

        self.ax = dict()
        self.cax = dict()
        self.im = dict()
        self.cbar = dict()

        self.ax["values"] = ax[0]
        self.cax["values"] = cax[0]
        panels = ["values"]
        if self.params["variances"][self.name]["show"]:
            self.ax["variances"] = ax[1]
            self.cax["variances"] = cax[1]
            panels.append("variances")

        extent_array = np.array(list(self.extent.values())).flatten()
        for key in panels:
            if self.params[key][self.name]["show"]:
                self.im[key] = self.ax[key].imshow(
                    [[1.0, 1.0], [1.0, 1.0]],
                    norm=self.params[key][self.name]["norm"],
                    extent=extent_array,
                    origin="lower",
                    aspect=self.aspect,
                    interpolation="nearest",
                    cmap=self.params[key][self.name]["cmap"])
                self.ax[key].set_title(self.name if key == "values" else "std dev.")
                if self.params[key][self.name]["cbar"]:
                    self.cbar[key] = plt.colorbar(self.im[key],
                                                  ax=self.ax[key],
                                                  cax=self.cax[key])
                    self.cbar[key].ax.set_ylabel(
                        name_with_unit(var=self.data_array, name=""))
                if self.cax[key] is None:
                    self.cbar[key].ax.yaxis.set_label_coords(-1.1, 0.5)
                self.members["images"][key] = self.im[key]
                self.members["colorbars"][key] = self.cbar[key]
                if self.params["masks"][self.name]["show"]:
                    self.im[self.get_mask_key(key)] = self.ax[key].imshow(
                        [[1.0, 1.0], [1.0, 1.0]],
                        extent=extent_array,
                        norm=self.params[key][self.name]["norm"],
                        origin="lower",
                        interpolation="nearest",
                        aspect=self.aspect,
                        cmap=self.params["masks"][self.name]["cmap"])
                if self.logx:
                    self.ax[key].set_xscale("log")
                if self.logy:
                    self.ax[key].set_yscale("log")

        # Call update_slice once to make the initial image
        self.update_axes()
        self.update_slice()
        self.vbox = widgets.VBox(self.vbox)
        self.vbox.layout.align_items = 'center'
        self.members["fig"] = self.fig
        self.members["ax"] = self.ax

        return

    def update_buttons(self, owner, event, dummy):
        toggle_slider = False
        if not self.slider[owner.dim].disabled:
            toggle_slider = True
            self.slider[owner.dim].disabled = True
        for dim, button in self.buttons.items():
            if (button.value == owner.value) and (dim != owner.dim):
                if self.slider[dim].disabled:
                    button.value = owner.old_value
                else:
                    button.value = None
                button.old_value = button.value
                if toggle_slider:
                    self.slider[dim].disabled = False
        owner.old_value = owner.value
        self.update_axes()
        self.update_slice()

        return

    def update_axes(self):
        # Go through the buttons and select the right coordinates for the axes
        axparams = {"x": {}, "y": {}}
        for dim, button in self.buttons.items():
            if self.slider[dim].disabled:
                but_val = button.value.lower()
                # xc = self.slider_x[self.name][dim].values
                if not self.histograms[self.name][dim]:
                    xc = self.slider_x[self.name][dim].values
                    if self.slider_nx[self.name][dim] < 2:
                        dx = 0.5 * abs(xc[0])
                        if dx == 0.0:
                            dx = 0.5
                        xmin = xc[0] - dx
                        xmax = xc[0] + dx
                        axparams[but_val]["xmin"] = xmin
                        axparams[but_val]["xmax"] = xmax
                    else:
                        xmin = 1.5 * xc[0] - 0.5 * xc[1]
                        xmax = 1.5 * xc[-1] - 0.5 * xc[-2]
                    self.extent[but_val] = [xmin, xmax]
                else:
                    self.extent[but_val] = self.slider_x[
                        self.name][dim].values[[0, -1]].astype(np.float)

                axparams[but_val]["lims"] = self.extent[but_val].copy()
                if getattr(self,
                           "log" + but_val) and (self.extent[but_val][0] <= 0):
                    if not self.histograms[self.name][dim]:
                        new_x = centers_to_edges(xc)
                    else:
                        new_x = edges_to_centers(
                            self.slider_x[self.name][dim].values)
                    axparams[but_val]["lims"][0] = new_x[np.searchsorted(
                        new_x, 0)]
                axparams[but_val]["labels"] = name_with_unit(
                    self.slider_x[self.name][dim], name=str(dim))
                axparams[but_val]["dim"] = dim

        extent_array = np.array(list(self.extent.values())).flatten()
        for key in self.ax.keys():
            with warnings.catch_warnings():
                warnings.filterwarnings("ignore", category=UserWarning)
                self.im[key].set_extent(extent_array)
                if self.params["masks"][self.name]["show"]:
                    self.im[self.get_mask_key(key)].set_extent(extent_array)
                self.ax[key].set_xlim(axparams["x"]["lims"])
                self.ax[key].set_ylim(axparams["y"]["lims"])
            self.ax[key].set_xlabel(axparams["x"]["labels"])
            self.ax[key].set_ylabel(axparams["y"]["labels"])
            for xy, param in axparams.items():
                if self.slider_ticks[self.name][param["dim"]] is not None:
                    getattr(self.ax[key], "set_{}ticklabels".format(xy))(
                        self.get_custom_ticks(ax=self.ax[key],
                                              dim=param["dim"],
                                              xy=xy))
        return

    def update_slice(self):
        """
        Slice data according to new slider value.
        """
        vslice = self.data_array
        if self.params["masks"][self.name]["show"]:
            mslice = self.masks
        # Slice along dimensions with active sliders
        button_dims = [None, None]
        for dim, val in self.slider.items():
            if not val.disabled:
                self.lab[dim].value = self.make_slider_label(
                    self.slider_x[self.name][dim], val.value)
                vslice = vslice[val.dim, val.value]
                # At this point, after masks were combined, all their
                # dimensions should be contained in the data_array.dims.
                if self.params["masks"][self.name]["show"]:
                    mslice = mslice[val.dim, val.value]
            else:
                button_dims[self.buttons[dim].value.lower() == "x"] = val.dim

        # Check if dimensions of arrays agree, if not, plot the transpose
        slice_dims = vslice.dims
        transp = slice_dims != button_dims

        if self.params["masks"][self.name]["show"]:
            shape_list = [self.shapes[self.name][bdim] for bdim in button_dims]
            # Use scipp's automatic broadcast functionality to broadcast
            # lower dimension masks to higher dimensions.
            # TODO: creating a Variable here could become expensive when
            # sliders are being used. We could consider performing the
            # automatic broadcasting once and store it in the Slicer class,
            # but this could create a large memory overhead if the data is
            # large.
            # Here, the data is at most 2D, so having the Variable creation
            # and broadcasting should remain cheap.
            msk = Variable(dims=button_dims,
                           values=np.ones(shape_list, dtype=np.int32))
            msk *= Variable(dims=mslice.dims,
                            values=mslice.values.astype(np.int32))

        autoscale_cbar = False
        if vslice.unaligned is not None:
            vslice = scipp_histogram(vslice)
            autoscale_cbar = True

        for key in self.ax.keys():
            arr = getattr(vslice, key)
            if key == "variances":
                arr = np.sqrt(arr)
            if transp:
                arr = arr.T
            self.im[key].set_data(arr)
            if autoscale_cbar:
                vmin_max = {"vmin": self._user_kwargs.vmin, "vmax": self._user_kwargs.vmax}
                cbar_params = parse_params(globs=vmin_max,
                                           array=arr,
                                           min_val=self.global_vmin,
                                           max_val=self.global_vmax)
                self.global_vmin = cbar_params["vmin"]
                self.global_vmax = cbar_params["vmax"]
                self.params[key][self.name]["norm"] = cbar_params["norm"]
                self.im[key].set_norm(self.params[key][self.name]["norm"])
            if self.params["masks"][self.name]["show"]:
                self.im[self.get_mask_key(key)].set_data(
                    self.mask_to_float(msk.values, arr))
                self.im[self.get_mask_key(key)].set_norm(
                    self.params[key][self.name]["norm"])

        return

    def toggle_masks(self, change):
        for key in self.ax.keys():
            self.im[key + "_masks"].set_visible(change["new"])
        change["owner"].description = "Hide masks" if change["new"] else \
            "Show masks"
        return

    def get_mask_key(self, key):
        return key + "_masks"
