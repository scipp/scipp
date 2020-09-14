# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import config
from .engine_2d import PlotEngine2d
from .render import render_plot
# from .profiler import Profiler
from .tools import to_bin_edges, parse_params
# from .widgets import PlotWidgets
from .._utils import name_with_unit
from .._scipp import core as sc
from .. import detail

# Other imports
import numpy as np
import ipywidgets as ipw
import matplotlib.pyplot as plt
from matplotlib.axes import Subplot
import warnings
import io
import os

class PlotView2d:
    def __init__(self,
                 # scipp_obj_dict=None,
                 # axes=None,
                 # masks=None,
                 controller=None,
                 ax=None,
                 cax=None,
                 aspect=None,
                 cmap=None,
                 norm=None,
                 title=None,
                 cbar=None,
                 unit=None,
                 log=None,
                 vmin=None,
                 vmax=None,
                 color=None,
                 logx=False,
                 logy=False,
                 mask_cmap=None,
                 mask_names=None,
                 resolution=None):

        self.controller = controller

        # self.extent = {"x": [1, 2], "y": [1, 2]}
        # self.logx = logx
        # self.logy = logy
        # self.vminmax = {"vmin": vmin, "vmax": vmax}
        # self.global_vmin = np.Inf
        # self.global_vmax = np.NINF
        # self.vslice = None
        # self.dslice = None
        # self.mslice = None
        self.xlim_updated = False
        self.ylim_updated = False
        self.current_lims = {"x": np.zeros(2), "y": np.zeros(2)}

        self.profile_hover_connection = None
        self.profile_pick_connection = None
        # self.button_dims = [None, None]
        # self.dim_to_xy = {}
        # self.cslice = None
        # self.autoscale_cbar = False

        # if resolution is not None:
        #     if isinstance(resolution, int):
        #         self.image_resolution = {"x": resolution, "y": resolution}
        #     else:
        #         self.image_resolution = resolution
        # else:
        #     self.image_resolution = {
        #         "x": config.plot.width,
        #         "y": config.plot.height
        #     }
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

        self.image = self.make_default_imshow(cmap=cmap, norm=norm, aspect=aspect, picker=5)
        # self.ax.set_title(self.engine.name)
        self.ax.set_title(title)
        if cbar:
            self.cbar = plt.colorbar(self.image, ax=self.ax, cax=self.cax)
            self.cbar.set_label(unit)
            # self.cbar.ax.set_picker(5)
        if self.cax is None:
            self.cbar.ax.yaxis.set_label_coords(-1.1, 0.5)
        # self.members["image"] = self.image
        # self.members["colorbar"] = self.cbar
        self.mask_image = {}
        # if len(self.masks[self.engine.name]) > 0:
            # self.members["masks"] = {}
        for m in mask_names:
            self.mask_image[m] = self.make_default_imshow(
                cmap=mask_cmap,
                norm=norm,
                aspect=aspect)
        if logx:
            self.ax.set_xscale("log")
        if logy:
            self.ax.set_yscale("log")
        plt.tight_layout(pad=1.5)


        # # Call update_slice once to make the initial image
        # self.controller.update_axes()

        # self.figure = self.fig.canvas
        # self.vbox = widgets.VBox(self.vbox)
        # self.vbox.layout.align_items = 'center'
        # self.members["fig"] = self.fig
        # self.members["ax"] = self.ax

        # Connect changes in axes limits to resampling function
        self.ax.callbacks.connect('xlim_changed', self.check_for_xlim_update)
        self.ax.callbacks.connect('ylim_changed', self.check_for_ylim_update)

        # # if self.cbar is not None:
        # #     self.fig.canvas.mpl_connect('pick_event', self.rescale_colorbar)

        return


    def _ipython_display_(self):
        # try:
        #     return self.fig.canvas._ipython_display_()
        # except AttributeError:
        #     return display(self.fig)
        return self._to_widget()._ipython_display_()

    def _to_widget(self):

        if hasattr(self.fig.canvas, "widgets"):
            return self.fig.canvas
        else:
            buf = io.BytesIO()
            self.fig.savefig(buf, format='png')
            buf.seek(0)
            return ipw.Image(value=buf.getvalue())

    def savefig(self, filename=None):
        self.fig.savefig(filename, bbox_inches="tight")


    # def _ipython_display_(self):
    #     return self._to_widget()._ipython_display_()

    # def _to_widget(self):
    #     # widgets_ = [self.figure, self.widgets]
    #     # if self.overview["additional_widgets"] is not None:
    #     #     wdgts.append(self.overview["additional_widgets"])
    #     return ipw.VBox([self.figure, self.widgets.container])

    def make_default_imshow(self, cmap, norm, aspect=None, picker=None):
        return self.ax.imshow([[1.0, 1.0], [1.0, 1.0]],
                              norm=norm,
                              extent=[1, 2, 1, 2],
                              origin="lower",
                              aspect=aspect,
                              interpolation="nearest",
                              cmap=cmap,
                              picker=picker)


    def rescale_to_data(self, vmin, vmax):
        # vmin = None
        # vmax = None
        # if button is None:
        #     # If the colorbar has been clicked, then ignore globally set
        #     # limits, as the click signals the user wants to change the
        #     # colorscale.
        #     vmin =  self.vminmax["vmin"]
        #     vmax =  self.vminmax["vmax"]
        # if vmin is None:
        #     vmin = self.controller.get_slice_min()
        # if vmax is None:
        #     vmax = self.controller.get_slice_min()

        self.image.set_clim([vmin, vmax])
        for m, im in self.mask_image.items():
            im.set_clim([vmin, vmax])
        self.fig.canvas.draw_idle()

    def toggle_mask(self, change):
        im = self.mask_image[change["owner"].mask_name]
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
            *sorted(self.controller.xlims[self.controller.name][self.controller.axparams["x"]["dim"]].values))
        xylims["y"] = np.clip(
            self.ax.get_ylim(),
            *sorted(self.controller.xlims[self.controller.name][self.controller.axparams["y"]["dim"]].values))

        dx = np.abs(self.current_lims["x"][1] - self.current_lims["x"][0])
        dy = np.abs(self.current_lims["y"][1] - self.current_lims["y"][0])
        diffx = np.abs(self.current_lims["x"] - xylims["x"]) / dx
        diffy = np.abs(self.current_lims["y"] - xylims["y"]) / dy
        diff = diffx.sum() + diffy.sum()

        # Only resample image if the changes in axes limits are large enough to
        # avoid too many updates while panning.
        if diff > 0.1:

            self.current_lims = xylims
            self.controller.update_viewport(xylims)
            # for xy, param in self.engine.axparams.items():
            #     # Create coordinate axes for resampled image array
            #     self.engine.xyrebin[xy] = sc.Variable(
            #         dims=[param["dim"]],
            #         values=np.linspace(xylims[xy][0], xylims[xy][1],
            #                            self.image_resolution[xy] + 1),
            #         unit=self.engine.data_arrays[self.engine.name].coords[param["dim"]].unit)
            # self.engine.update_image(extent=np.array(list(xylims.values())).flatten())
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
                    alist[0] = (*self.controller.xlims[self.controller.name][
                        self.controller.axparams["x"]["dim"]].values, *self.controller.xlims[
                            self.controller.name][self.controller.axparams["y"]["dim"]].values)
                    # Insert the new tuple
                    self.fig.canvas.toolbar._nav_stack._elements[0][
                        key] = tuple(alist)



    def update_axes(self, axparams, axformatter, axlocator, logx, logy):

        self.current_lims['x'] = axparams["x"]["lims"]
        self.current_lims['y'] = axparams["y"]["lims"]

        is_log = {"x": logx, "y": logy}

        # Set axes labels
        self.ax.set_xlabel(axparams["x"]["labels"])
        self.ax.set_ylabel(axparams["y"]["labels"])

        for xy, param in axparams.items():
            axis = getattr(self.ax, "{}axis".format(xy))
            # is_log = getattr(self.controller, "log{}".format(xy))
            axis.set_major_formatter(
                axformatter[param["dim"]][is_log[xy]])
            axis.set_major_locator(
                axlocator[param["dim"]][is_log[xy]])

        # Set axes limits and ticks
        extent_array = np.array([axparams["x"]["lims"], axparams["y"]["lims"]]).flatten()
        with warnings.catch_warnings():
            warnings.filterwarnings("ignore", category=UserWarning)
            self.image.set_extent(extent_array)
            # if len(self.masks[self.name]) > 0:
            for m, im in self.mask_image.items():
                im.set_extent(extent_array)
            self.ax.set_xlim(axparams["x"]["lims"])
            self.ax.set_ylim(axparams["y"]["lims"])

        self.reset_home_button()
        # self.rescale_to_data()


    def update_data(self, new_values):
        self.image.set_data(new_values["values"])
        if new_values["extent"] is not None:
            self.image.set_extent(new_values["extent"])
        for m in self.mask_image:
            if new_values["masks"][m] is not None:
                self.mask_image[m].set_data(new_values["masks"][m])
            else:
                self.mask_image[m].set_visible(False)
                self.mask_image[m].set_url("hide")
            if new_values["extent"] is not None:
                self.mask_image[m].set_extent(new_values["extent"])
        self.fig.canvas.draw_idle()






    def update_profile_connection(self, connect):
        # Connect picking events
        if connect:
            self.profile_pick_connection = self.fig.canvas.mpl_connect('pick_event', self.keep_or_delete_profile)
            self.profile_hover_connection = self.fig.canvas.mpl_connect('motion_notify_event', self.update_profile)
        else:
            self.fig.canvas.mpl_disconnect(self.profile_pick_connection)
            self.fig.canvas.mpl_disconnect(self.profile_hover_connection)



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
