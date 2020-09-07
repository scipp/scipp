# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import config
from .engine_2d import PlotEngine2d
from .render import render_plot
# from .profiler import Profiler
from .tools import to_bin_edges, parse_params
from .widgets import PlotWidgets
from .._utils import name_with_unit
from .._scipp import core as sc
from .. import detail

# Other imports
import numpy as np
import ipywidgets as ipw
import matplotlib.pyplot as plt
from matplotlib.axes import Subplot
import warnings
import os


def plot_2d(scipp_obj_dict=None,
            axes=None,
            masks=None,
            filename=None,
            figsize=None,
            ax=None,
            cax=None,
            aspect=None,
            cmap=None,
            log=False,
            vmin=None,
            vmax=None,
            color=None,
            logx=False,
            logy=False,
            logxy=False,
            resolution=None):
    """
    Plot a 2D slice through a N dimensional dataset. For every dimension above
    2, a slider is created to adjust the position of the slice in that
    particular dimension.
    """

    sp = SciPlot2d(scipp_obj_dict=scipp_obj_dict,
                  axes=axes,
                  masks=masks,
                  ax=ax,
                  cax=cax,
                  aspect=aspect,
                  cmap=cmap,
                  log=log,
                  vmin=vmin,
                  vmax=vmax,
                  color=color,
                  logx=logx or logxy,
                  logy=logy or logxy,
                  resolution=resolution)

    if filename is not None:
        sp.fig.savefig(filename, bbox_inches="tight")

    # if ax is None:
    #     render_plot(figure=sv.fig, widgets=sv.vbox, filename=filename)

    return sp


class SciPlot2d():
    def __init__(self,
                 scipp_obj_dict=None,
                 axes=None,
                 masks=None,
                 ax=None,
                 cax=None,
                 aspect=None,
                 cmap=None,
                 log=None,
                 vmin=None,
                 vmax=None,
                 color=None,
                 logx=False,
                 logy=False,
                 resolution=None):

        # super().__init__(scipp_obj_dict=scipp_obj_dict,
        #                  axes=axes,
        #                  masks=masks,
        #                  cmap=cmap,
        #                  log=log,
        #                  vmin=vmin,
        #                  vmax=vmax,
        #                  color=color,
        #                  aspect=aspect,
        #                  button_options=['X', 'Y'])

        self.engine = PlotEngine2d(parent=self,
                         scipp_obj_dict=scipp_obj_dict,
                         axes=axes,
                         masks=masks,
                         cmap=cmap,
                         log=log,
                         vmin=vmin,
                         vmax=vmax,
                         color=color)
                         # aspect=aspect)
                         # button_options=['X', 'Y'])

        self.widgets = PlotWidgets(self, engine=self.engine,
                         button_options=['X', 'Y'])

        self.profile_viewer = None

        # self.members["images"] = {}
        # self.axparams = {"x": {}, "y": {}}
        self.extent = {"x": [1, 2], "y": [1, 2]}
        self.logx = logx
        self.logy = logy
        self.vminmax = {"vmin": vmin, "vmax": vmax}
        self.global_vmin = np.Inf
        self.global_vmax = np.NINF
        self.vslice = None
        self.dslice = None
        self.mslice = None
        self.xlim_updated = False
        self.ylim_updated = False
        self.current_lims = {"x": np.zeros(2), "y": np.zeros(2)}
        # self.button_dims = [None, None]
        # self.dim_to_xy = {}
        self.cslice = None
        self.autoscale_cbar = False

        if resolution is not None:
            if isinstance(resolution, int):
                self.image_resolution = {"x": resolution, "y": resolution}
            else:
                self.image_resolution = resolution
        else:
            self.image_resolution = {
                "x": config.plot.width,
                "y": config.plot.height
            }
        # self.xyrebin = {}
        # # self.xyedges = {}
        # self.xywidth = {}
        # self.image_pixel_size = {}

        # Get or create matplotlib axes
        self.fig = None
        self.ax = ax
        self.cax = cax
        self.cbar = None
        if self.ax is None:
            self.fig, self.ax = plt.subplots(
                1,
                1,
                figsize=(config.plot.width / config.plot.dpi,
                         config.plot.height / config.plot.dpi),
                dpi=config.plot.dpi)

        # Save aspect ratio setting
        # self.aspect = aspect
        if aspect is None:
            aspect = config.plot.aspect

        self.image = self.make_default_imshow(
            self.engine.params["values"][self.engine.name]["cmap"], aspect=aspect, picker=5)
        self.ax.set_title(self.engine.name)
        if self.engine.params["values"][self.engine.name]["cbar"]:
            self.cbar = plt.colorbar(self.image, ax=self.ax, cax=self.cax)
            self.cbar.set_label(name_with_unit(var=self.engine.data_arrays[self.engine.name], name=""))
            # self.cbar.ax.set_picker(5)
        if self.cax is None:
            self.cbar.ax.yaxis.set_label_coords(-1.1, 0.5)
        # self.members["image"] = self.image
        # self.members["colorbar"] = self.cbar
        self.mask_image = {}
        # if len(self.masks[self.engine.name]) > 0:
            # self.members["masks"] = {}
        for m in self.widgets.mask_checkboxes[self.engine.name]:
            self.mask_image[m] = self.make_default_imshow(
                cmap=self.engine.params["masks"][self.engine.name]["cmap"],
                aspect=aspect)
        if self.logx:
            self.ax.set_xscale("log")
        if self.logy:
            self.ax.set_yscale("log")

        # Call update_slice once to make the initial image
        self.engine.update_axes()

        self.figure = self.fig.canvas
        # self.vbox = widgets.VBox(self.vbox)
        # self.vbox.layout.align_items = 'center'
        # self.members["fig"] = self.fig
        # self.members["ax"] = self.ax

        # # Connect changes in axes limits to resampling function
        # self.ax.callbacks.connect('xlim_changed', self.check_for_xlim_update)
        # self.ax.callbacks.connect('ylim_changed', self.check_for_ylim_update)

        # # if self.cbar is not None:
        # #     self.fig.canvas.mpl_connect('pick_event', self.rescale_colorbar)

        return

    def _ipython_display_(self):
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        # widgets_ = [self.figure, self.widgets]
        # if self.overview["additional_widgets"] is not None:
        #     wdgts.append(self.overview["additional_widgets"])
        return ipw.VBox([self.figure, self.widgets.container])

    def make_default_imshow(self, cmap, aspect=None, picker=None):
        return self.ax.imshow([[1.0, 1.0], [1.0, 1.0]],
                              norm=self.engine.params["values"][self.engine.name]["norm"],
                              extent=np.array(list(
                                  self.extent.values())).flatten(),
                              origin="lower",
                              aspect=aspect,
                              interpolation="nearest",
                              cmap=cmap,
                              picker=picker)

    # def update_buttons(self, owner, event, dummy):
    #     toggle_slider = False
    #     if not self.slider[owner.dim].disabled:
    #         toggle_slider = True
    #         self.slider[owner.dim].disabled = True
    #         self.thickness_slider[owner.dim].disabled = True
    #     for dim, button in self.buttons.items():
    #         if (button.value == owner.value) and (dim != owner.dim):
    #             if self.slider[dim].disabled:
    #                 button.value = owner.old_value
    #             else:
    #                 button.value = None
    #             button.old_value = button.value
    #             if toggle_slider:
    #                 self.slider[dim].disabled = False
    #                 self.thickness_slider[dim].disabled = False
    #     owner.old_value = owner.value
    #     self.update_axes()
    #     return

    # def update_axes(self):
    #     # Go through the buttons and select the right coordinates for the axes
    #     for dim, button in self.buttons.items():
    #         if self.slider[dim].disabled:
    #             but_val = button.value.lower()
    #             self.extent[but_val] = self.slider_xlims[self.engine.name][dim].values
    #             self.axparams[but_val]["lims"] = self.extent[but_val].copy()
    #             if getattr(self,
    #                        "log" + but_val) and (self.extent[but_val][0] <= 0):
    #                 self.axparams[but_val]["lims"][
    #                     0] = 1.0e-03 * self.axparams[but_val]["lims"][1]
    #             # self.axparams[but_val]["labels"] = name_with_unit(
    #             #     self.slider_label[self.engine.name][dim]["coord"],
    #             #     name=self.slider_label[self.engine.name][dim]["name"])
    #             self.axparams[but_val]["labels"] = name_with_unit(
    #                 self.engine.data_arrays[self.engine.name].coords[dim])
    #             self.axparams[but_val]["dim"] = dim
    #             # Get the dimensions corresponding to the x/y buttons
    #             self.button_dims[but_val == "x"] = button.dim
    #             self.dim_to_xy[dim] = but_val

    #     extent_array = np.array(list(self.extent.values())).flatten()
    #     self.current_lims['x'] = extent_array[:2]
    #     self.current_lims['y'] = extent_array[2:]

    #     # TODO: if labels are used on a 2D coordinates, we need to update
    #     # the axes tick formatter to use xyrebin coords
    #     for xy, param in self.axparams.items():
    #         # Create coordinate axes for resampled array to be used as image
    #         offset = 2 * (xy == "y")
    #         self.xyrebin[xy] = sc.Variable(
    #             dims=[param["dim"]],
    #             values=np.linspace(extent_array[0 + offset],
    #                                extent_array[1 + offset],
    #                                self.image_resolution[xy] + 1),
    #             unit=self.engine.data_arrays[self.engine.name].coords[param["dim"]].unit)

    #     # Set axes labels
    #     self.ax.set_xlabel(self.axparams["x"]["labels"])
    #     self.ax.set_ylabel(self.axparams["y"]["labels"])
    #     for xy, param in self.axparams.items():
    #         axis = getattr(self.ax, "{}axis".format(xy))
    #         is_log = getattr(self, "log{}".format(xy))
    #         axis.set_major_formatter(
    #             self.slider_axformatter[self.engine.name][param["dim"]][is_log])
    #         axis.set_major_locator(
    #             self.slider_axlocator[self.engine.name][param["dim"]][is_log])

    #     # Set axes limits and ticks
    #     with warnings.catch_warnings():
    #         warnings.filterwarnings("ignore", category=UserWarning)
    #         self.image.set_extent(extent_array)
    #         if len(self.masks[self.engine.name]) > 0:
    #             for m in self.masks[self.engine.name]:
    #                 self.members["masks"][m].set_extent(extent_array)
    #         self.ax.set_xlim(self.axparams["x"]["lims"])
    #         self.ax.set_ylim(self.axparams["y"]["lims"])

    #     # # If there are no multi-d coords, we update the edges and widths only
    #     # # once here.
    #     # if not self.contains_multid_coord[self.engine.name]:
    #     #     self.slice_coords()
    #     # Update the image using resampling
    #     self.update_slice()

    #     # Some annoying house-keeping when using X/Y buttons: we need to update
    #     # the deeply embedded limits set by the Home button in the matplotlib
    #     # toolbar. The home button actually brings the first element in the
    #     # navigation stack to the top, so we need to modify the first element
    #     # in the navigation stack in-place.
    #     if self.fig is not None:
    #         if self.fig.canvas.toolbar is not None:
    #             if len(self.fig.canvas.toolbar._nav_stack._elements) > 0:
    #                 # Get the first key in the navigation stack
    #                 key = list(self.fig.canvas.toolbar._nav_stack._elements[0].
    #                            keys())[0]
    #                 # Construct a new tuple for replacement
    #                 alist = []
    #                 for x in self.fig.canvas.toolbar._nav_stack._elements[0][
    #                         key]:
    #                     alist.append(x)
    #                 alist[0] = (*self.slider_xlims[self.engine.name][
    #                     self.button_dims[1]].values, *self.slider_xlims[
    #                         self.engine.name][self.button_dims[0]].values)
    #                 # Insert the new tuple
    #                 self.fig.canvas.toolbar._nav_stack._elements[0][
    #                     key] = tuple(alist)

    #     self.rescale_to_data()


    #     if self.profile_viewer is not None:
    #         self.update_profile_axes()

    #     return

    # def compute_bin_widths(self, xy, dim):
    #     """
    #     Pixel widths used for scaling before rebin step
    #     """
    #     self.xywidth[xy] = (self.xyedges[xy][dim, 1:] -
    #                         self.xyedges[xy][dim, :-1])
    #     self.xywidth[xy].unit = sc.units.one

    # def slice_coords(self):
    #     """
    #     Recursively slice the coords along the dimensions of active sliders.
    #     """
    #     self.cslice = self.slider_coord[self.engine.name].copy()
    #     for key in self.cslice:
    #         # Slice along dimensions with active sliders
    #         for dim, val in self.slider.items():
    #             if not val.disabled and val.dim in self.cslice[key].dims:
    #                 self.cslice[key] = self.cslice[key][val.dim, val.value]

    #     # Update the xyedges and xywidth
    #     for xy, param in self.axparams.items():
    #         # Create bin-edge coordinates in the case of non bin-edges, since
    #         # rebin only accepts bin edges.
    #         if not self.histograms[self.engine.name][param["dim"]][param["dim"]]:
    #             self.xyedges[xy] = to_bin_edges(self.cslice[param["dim"]],
    #                                             param["dim"])
    #         else:
    #             self.xyedges[xy] = self.cslice[param["dim"]].astype(
    #                 sc.dtype.float64)
    #         # Pixel widths used for scaling before rebin step
    #         self.compute_bin_widths(xy, param["dim"])

    # def prepare_slice_

    def rescale_to_data(self, button=None):
        vmin = None
        vmax = None
        if button is None:
            # If the colorbar has been clicked, then ignore globally set
            # limits, as the click signals the user wants to change the
            # colorscale.
            vmin =  self.vminmax["vmin"]
            vmax =  self.vminmax["vmax"]
        if vmin is None:
            vmin = sc.min(self.engine.dslice.data).value
        if vmax is None:
            vmax = sc.max(self.engine.dslice.data).value
        self.image.set_clim([vmin, vmax])
        for m, im in self.mask_image.items():
            im.set_clim([vmin, vmax])
        self.fig.canvas.draw_idle()


    # def slice_data(self):
    #     """
    #     Recursively slice the data along the dimensions of active sliders.
    #     """
    #     data_slice = self.engine.data_arrays[self.engine.name]

    #     # Slice along dimensions with active sliders
    #     for dim, val in self.slider.items():
    #         if not val.disabled:
    #             # self.lab[dim].value = self.make_slider_label(
    #             #     self.slider_label[self.engine.name][dim]["coord"], val.value)
    #             # print(self.slider_axformatter)
    #             # self.lab[dim].value = self.make_slider_label(
    #             #     val.value, self.slider_axformatter[self.engine.name][dim][False])
    #             # self.lab[dim].value = self.slider_axformatter[self.engine.name][dim][False].format_data_short(val.value)
    #             deltax = self.thickness_slider[dim].value

    #             # print(data_slice)
    #             # print(sc.Variable([dim], values=[val.value - 0.5 * deltax,
    #             #                                                      val.value + 0.5 * deltax],
    #             #                                             unit=data_slice.coords[dim].unit))

    #             # TODO: see if we can call resample_image only once with
    #             # rebin_edges dict containing all dims to be sliced.
    #             data_slice = self.resample_image(data_slice,
    #                     # coord_edges={dim: self.slider_coord[self.engine.name][dim]},
    #                     rebin_edges={dim: sc.Variable([dim], values=[val.value - 0.5 * deltax,
    #                                                                  val.value + 0.5 * deltax],
    #                                                         unit=data_slice.coords[dim].unit)})[dim, 0]
    #                 # depth = self.slider_xlims[self.engine.name][dim][dim, 1] - self.slider_xlims[self.engine.name][dim][dim, 0]
    #                 # depth.unit = sc.units.one
    #             data_slice *= (deltax * sc.units.one)

    #             # data_slice = data_slice[val.dim, val.value]


    #     # Update the xyedges and xywidth
    #     for xy, param in self.axparams.items():
    #         # # Create bin-edge coordinates in the case of non bin-edges, since
    #         # # rebin only accepts bin edges.
    #         # if not self.histograms[self.engine.name][param["dim"]][param["dim"]]:
    #         #     self.xyedges[xy] = to_bin_edges(self.cslice[param["dim"]],
    #         #                                     param["dim"])
    #         # else:
    #         #     self.xyedges[xy] = self.cslice[param["dim"]].astype(
    #         #         sc.dtype.float64)
    #         # # Pixel widths used for scaling before rebin step
    #         # self.compute_bin_widths(xy, param["dim"])
    #         self.xywidth[xy] = (
    #             data_slice.coords[param["dim"]][param["dim"], 1:] -
    #             data_slice.coords[param["dim"]][param["dim"], :-1])
    #         self.xywidth[xy].unit = sc.units.one


    #     self.vslice = data_slice
    #     # Scale by bin width and then rebin in both directions
    #     # Note that this has to be written as 2 inplace operations to avoid
    #     # creation of large 2D temporary from broadcast
    #     self.vslice *= self.xywidth["x"]
    #     self.vslice *= self.xywidth["y"]

        # self.prepare_slice_for_resample(data_slice)

        # # In the case of unaligned data, we may want to auto-scale the colorbar
        # # as we slice through dimensions. Colorbar limits are allowed to grow
        # # but not shrink.
        # if data_slice.unaligned is not None:
        #     data_slice = sc.histogram(data_slice)
        #     data_slice.variances = None
        #     self.autoscale_cbar = True
        # else:
        #     self.autoscale_cbar = False

        # self.vslice = detail.move_to_data_array(
        #     data=sc.Variable(dims=data_slice.dims,
        #                      unit=sc.units.counts,
        #                      values=data_slice.values,
        #                      variances=data_slice.variances,
        #                      dtype=sc.dtype.float32))
        # self.vslice.coords[self.xyrebin["x"].dims[0]] = self.xyedges["x"]
        # self.vslice.coords[self.xyrebin["y"].dims[0]] = self.xyedges["y"]

        # # Also include masks
        # if len(data_slice.masks) > 0:
        #     for m in data_slice.masks:
        #         self.vslice.masks[m] = data_slice.masks[m]

        # # Scale by bin width and then rebin in both directions
        # # Note that this has to be written as 2 inplace operations to avoid
        # # creation of large 2D temporary from broadcast
        # self.vslice *= self.xywidth["x"]
        # self.vslice *= self.xywidth["y"]

    # def prepare_slice_for_resample(self, data_slice):
    #     # In the case of unaligned data, we may want to auto-scale the colorbar
    #     # as we slice through dimensions. Colorbar limits are allowed to grow
    #     # but not shrink.
    #     if data_slice.unaligned is not None:
    #         data_slice = sc.histogram(data_slice)
    #         data_slice.variances = None
    #         self.autoscale_cbar = True
    #     else:
    #         self.autoscale_cbar = False

    #     self.vslice = detail.move_to_data_array(
    #         data=sc.Variable(dims=data_slice.dims,
    #                          unit=sc.units.counts,
    #                          values=data_slice.values,
    #                          variances=data_slice.variances,
    #                          dtype=sc.dtype.float32))
    #     # self.vslice.coords[self.xyrebin["x"].dims[0]] = self.xyedges["x"]
    #     # self.vslice.coords[self.xyrebin["y"].dims[0]] = self.xyedges["y"]
    #     self.vslice.coords[self.xyrebin["x"].dims[0]] = data_slice.coords[self.axparams["x"]["dim"]]
    #     self.vslice.coords[self.xyrebin["y"].dims[0]] = data_slice.coords[self.axparams["y"]["dim"]]

    #     # Also include masks
    #     if len(data_slice.masks) > 0:
    #         for m in data_slice.masks:
    #             self.vslice.masks[m] = data_slice.masks[m]

    #     # Scale by bin width and then rebin in both directions
    #     # Note that this has to be written as 2 inplace operations to avoid
    #     # creation of large 2D temporary from broadcast
    #     self.vslice *= self.xywidth["x"]
    #     self.vslice *= self.xywidth["y"]

    # def update_slice(self, change=None):
    #     """
    #     Slice data according to new slider value and update the image.
    #     """
    #     # # If there are multi-d coords in the data we also need to slice the
    #     # # coords and update the xyedges and xywidth
    #     # if self.contains_multid_coord[self.engine.name]:
    #     #     self.slice_coords()
    #     self.slice_data()
    #     # Update image with resampling
    #     self.update_image()
    #     return

    def toggle_mask(self, change):
        im = self.mask_image[change["owner"].masks_name]
        if im.get_url() != "hide":
            im.set_visible(change["new"])
        self.fig.canvas.draw_idle()
        return

    # def toggle_profile_view(self, change=None):
    #     self.profile_dim = change["owner"].dim
    #     if change["new"]:
    #         self.show_profile_view()
    #     else:
    #         self.hide_profile_view()
    #     return

    def check_for_xlim_update(self, event_ax):
        self.xlim_updated = True
        if self.ylim_updated:
            self.update_bins_from_axes_limits()

    def check_for_ylim_update(self, event_ax):
        self.ylim_updated = True
        if self.xlim_updated:
            self.update_bins_from_axes_limits()

    def update_bins_from_axes_limits(self):
        """
        Update the axis limits and resample the image according to new viewport
        """
        self.xlim_updated = False
        self.ylim_updated = False
        xylims = {}
        # Make sure we don't overrun the original array bounds
        xylims["x"] = np.clip(
            self.ax.get_xlim(),
            *sorted(self.slider_xlims[self.engine.name][self.button_dims[1]].values))
        xylims["y"] = np.clip(
            self.ax.get_ylim(),
            *sorted(self.slider_xlims[self.engine.name][self.button_dims[0]].values))

        dx = np.abs(self.current_lims["x"][1] - self.current_lims["x"][0])
        dy = np.abs(self.current_lims["y"][1] - self.current_lims["y"][0])
        diffx = np.abs(self.current_lims["x"] - xylims["x"]) / dx
        diffy = np.abs(self.current_lims["y"] - xylims["y"]) / dy
        diff = diffx.sum() + diffy.sum()

        # Only resample image if the changes in axes limits are large enough to
        # avoid too many updates while panning.
        if diff > 0.1:
            self.current_lims = xylims
            for xy, param in self.axparams.items():
                # Create coordinate axes for resampled image array
                self.engine.xyrebin[xy] = sc.Variable(
                    dims=[param["dim"]],
                    values=np.linspace(xylims[xy][0], xylims[xy][1],
                                       self.image_resolution[xy] + 1),
                    unit=self.engine.data_arrays[self.engine.name].coords[param["dim"]].unit)
            self.update_image(extent=np.array(list(xylims.values())).flatten())
        return


    def reset_home_button(self):
        # Some annoying house-keeping when using X/Y buttons: we need to update
        # the deeply embedded limits set by the Home button in the matplotlib
        # toolbar. The home button actually brings the first element in the
        # navigation stack to the top, so we need to modify the first element
        # in the navigation stack in-place.
        if self.fig is not None:
            if self.fig.canvas.toolbar is not None:
                if len(self.fig.canvas.toolbar._nav_stack._elements) > 0:
                    # Get the first key in the navigation stack
                    key = list(self.fig.canvas.toolbar._nav_stack._elements[0].
                               keys())[0]
                    # Construct a new tuple for replacement
                    alist = []
                    for x in self.fig.canvas.toolbar._nav_stack._elements[0][
                            key]:
                        alist.append(x)
                    alist[0] = (*self.engine.slider_xlims[self.name][
                        self.button_dims[1]].values, *self.engine.slider_xlims[
                            self.name][self.button_dims[0]].values)
                    # Insert the new tuple
                    self.fig.canvas.toolbar._nav_stack._elements[0][
                        key] = tuple(alist)

    # def select_bins(self, coord, dim, start, end):
    #     bins = coord.shape[-1]
    #     if len(coord.dims) != 1:  # TODO find combined min/max
    #         return dim, slice(0, bins - 1)
    #     # scipp treats bins as closed on left and open on right: [left, right)
    #     first = sc.sum(coord <= start, dim).value - 1
    #     last = bins - sc.sum(coord > end, dim).value
    #     if first >= last:  # TODO better handling for decreasing
    #         return dim, slice(0, bins - 1)
    #     first = max(0, first)
    #     last = min(bins - 1, last)
    #     return dim, slice(first, last + 1)

    # def resample_image(self, array, coord_edges, rebin_edges):
    # def resample_image(self, array, rebin_edges):
    #     dslice = array
    #     # Select bins to speed up rebinning
    #     for dim in rebin_edges:
    #         this_slice = self.select_bins(array.coords[dim], dim,
    #                                       rebin_edges[dim][dim, 0],
    #                                       rebin_edges[dim][dim, -1])
    #         dslice = dslice[this_slice]

    #     # Rebin the data
    #     for dim, edges in rebin_edges.items():
    #         # print(dim)
    #         # print(dslice)
    #         # print(edges)
    #         dslice = sc.rebin(dslice, dim, edges)

    #     # Divide by pixel width
    #     for dim, edges in rebin_edges.items():
    #         # self.image_pixel_size[key] = edges.values[1] - edges.values[0]
    #         # print("edges.values[1]", edges.values[1])
    #         # print(self.image_pixel_size[key])
    #         # dslice /= self.image_pixel_size[key]
    #         dslice /= edges[dim, 1:] - edges[dim, :-1]
    #     return dslice

    # def update_image(self, extent=None):
    #     # The order of the dimensions that are rebinned matters if 2D coords
    #     # are present. We must rebin the base dimension of the 2D coord first.
    #     xy = "yx"
    #     if len(self.vslice.coords[self.button_dims[1]].dims) > 1:
    #         xy = "xy"

    #     dimy = self.xyrebin[xy[0]].dims[0]
    #     dimx = self.xyrebin[xy[1]].dims[0]

    #     rebin_edges = {
    #         dimy: self.xyrebin[xy[0]],
    #         dimx: self.xyrebin[xy[1]]
    #     }

    #     resampled_image = self.resample_image(self.vslice,
    #                                           # coord_edges={
    #                                           #     self.xyrebin[xy[0]].dims[0]:
    #                                           #     self.xyedges[xy[0]],
    #                                           #     self.xyrebin[xy[1]].dims[0]:
    #                                           #     self.xyedges[xy[1]]
    #                                           # },
    #                                           rebin_edges=rebin_edges)

    #     # Use Scipp's automatic transpose to match the image x/y axes
    #     # TODO: once transpose is available for DataArrays,
    #     # use sc.transpose(dslice, self.button_dims) instead.
    #     shape = [
    #         self.xyrebin["y"].shape[0] - 1, self.xyrebin["x"].shape[0] - 1
    #     ]
    #     self.dslice = sc.DataArray(coords=rebin_edges,
    #                                data=sc.Variable(dims=self.button_dims,
    #                                                 values=np.ones(shape),
    #                                                 variances=np.zeros(shape),
    #                                                 dtype=self.vslice.dtype,
    #                                                 unit=sc.units.one))

    #     self.dslice *= resampled_image

    #     # Update the matplotlib image data
    #     arr = self.dslice.values
    #     self.image.set_data(arr)
    #     if extent is not None:
    #         self.image.set_extent(extent)

    #     # Handle masks
    #     if len(self.masks[self.engine.name]) > 0:
    #         # Use scipp's automatic broadcast functionality to broadcast
    #         # lower dimension masks to higher dimensions.
    #         # TODO: creating a Variable here could become expensive when
    #         # sliders are being used. We could consider performing the
    #         # automatic broadcasting once and store it in the Slicer class,
    #         # but this could create a large memory overhead if the data is
    #         # large.
    #         # Here, the data is at most 2D, so having the Variable creation
    #         # and broadcasting should remain cheap.
    #         base_mask = sc.Variable(dims=self.dslice.dims,
    #                                 values=np.ones(self.dslice.shape,
    #                                                dtype=np.int32))
    #         for m in self.masks[self.engine.name]:
    #             if m in self.dslice.masks:
    #                 msk = base_mask * sc.Variable(
    #                     dims=self.dslice.masks[m].dims,
    #                     values=self.dslice.masks[m].values.astype(np.int32))
    #                 self.members["masks"][m].set_data(
    #                     self.mask_to_float(msk.values, arr))
    #                 if extent is not None:
    #                     self.members["masks"][m].set_extent(extent)
    #             else:
    #                 self.members["masks"][m].set_visible(False)
    #                 self.members["masks"][m].set_url("hide")

    #     if self.autoscale_cbar:
    #         cbar_params = parse_params(globs=self.vminmax,
    #                                    array=arr,
    #                                    min_val=self.global_vmin,
    #                                    max_val=self.global_vmax)
    #         self.global_vmin = cbar_params["vmin"]
    #         self.global_vmax = cbar_params["vmax"]
    #         self.engine.params["values"][self.engine.name]["norm"] = cbar_params["norm"]
    #         self.image.set_norm(self.engine.params["values"][self.engine.name]["norm"])
    #         if len(self.masks[self.engine.name]) > 0:
    #             for m in self.masks[self.engine.name]:
    #                 self.members["masks"][m].set_norm(
    #                     self.engine.params["values"][self.engine.name]["norm"])
