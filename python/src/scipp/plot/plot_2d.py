# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import config
# from .plot_1d import Slicer1d
from .plot import plot
from .render import render_plot
from .slicer import Slicer
from ..show import _hex_to_rgb
from .tools import to_bin_edges, parse_params
from .._utils import name_with_unit
from .._scipp import core as sc
from .. import detail

# Other imports
import numpy as np
import ipywidgets as widgets
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle
from matplotlib.collections import PathCollection
import warnings


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
            resolution=None,
            picking=False):
    """
    Plot a 2D slice through a N dimensional dataset. For every dimension above
    2, a slider is created to adjust the position of the slice in that
    particular dimension.
    """

    sv = Slicer2d(scipp_obj_dict=scipp_obj_dict,
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
                  resolution=resolution,
                  picking=picking)

    if ax is None:
        render_plot(figure=sv.fig, widgets=sv.vbox, filename=filename)

    return sv


class Slicer2d(Slicer):
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
                 resolution=None,
                 picking=None):

        super().__init__(scipp_obj_dict=scipp_obj_dict,
                         axes=axes,
                         masks=masks,
                         cmap=cmap,
                         log=log,
                         vmin=vmin,
                         vmax=vmax,
                         color=color,
                         aspect=aspect,
                         button_options=['X', 'Y'])

        # Initial checks
        if picking and self.ndim != 3:
            raise RuntimeError("Picking on a 2d plot is only supported for 3D "
                               "data.")

        self.members["images"] = {}
        self.axparams = {"x": {}, "y": {}}
        self.extent = {"x": [1, 2], "y": [1, 2]}
        self.logx = logx
        self.logy = logy
        self.log = log
        self.vminmax = {"vmin": vmin, "vmax": vmax}
        self.global_vmin = np.Inf
        self.global_vmax = np.NINF
        self.vslice = None
        self.dslice = None
        self.mslice = None
        self.xlim_updated = False
        self.ylim_updated = False
        self.current_lims = {"x": np.zeros(2), "y": np.zeros(2)}
        self.button_dims = [None, None]
        self.dim_to_xy = {}
        self.cslice = None
        self.autoscale_cbar = False

        # Variables for profile plotting from picking
        self.picking = picking
        self.profile_viewer = None
        self.profile_key = None
        self.slice_pos_rectangle = None
        self.profile_scatter = None
        self.profile_update_lock = False
        self.da_with_edges = None
        if self.picking:
            self.da_with_edges = self.make_data_array_with_bin_edges()

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
        self.xyrebin = {}
        self.xyedges = {}
        self.xywidth = {}
        self.image_pixel_size = {}

        # Get or create matplotlib axes
        self.fig = None
        self.ax = ax
        self.ax_picking = None
        self.cax = cax
        self.cbar = None
        if self.ax is None:
            # Note: if picking is enabled, we need to create the subplot axes
            # here, as the creation of new axes triggered by a matplotlib event
            # does not seem to work (maybe it's swallowed by the kernel in a
            # similar way to the print statements?). Hence we cannot just let
            # the call to plot create its own axes.
            nrows = 1 + self.picking
            self.fig, mpl_axes = plt.subplots(
                nrows,
                1,
                figsize=(config.plot.width / config.plot.dpi,
                         nrows * config.plot.height / config.plot.dpi),
                dpi=config.plot.dpi)
            if self.picking:
                self.ax = mpl_axes[0]
                self.ax_picking = mpl_axes[1]
                self.ax_picking.set_ylim([
                    self.params["values"][self.name]["vmin"],
                    self.params["values"][self.name]["vmax"]
                ])
            else:
                self.ax = mpl_axes

        self.image = self.make_default_imshow(
            self.params["values"][self.name]["cmap"], picker=5)
        self.ax.set_title(self.name)
        if self.params["values"][self.name]["cbar"]:
            self.cbar = plt.colorbar(self.image, ax=self.ax, cax=self.cax)
            self.cbar.set_label(name_with_unit(var=self.data_array, name=""))
        if self.cax is None:
            self.cbar.ax.yaxis.set_label_coords(-1.1, 0.5)
        self.members["image"] = self.image
        self.members["colorbar"] = self.cbar
        if len(self.masks[self.name]) > 0:
            self.members["masks"] = {}
            for m in self.masks[self.name]:
                self.members["masks"][m] = self.make_default_imshow(
                    cmap=self.params["masks"][self.name]["cmap"])
        if self.logx:
            self.ax.set_xscale("log")
        if self.logy:
            self.ax.set_yscale("log")

        # Call update_slice once to make the initial image
        self.update_axes()
        self.vbox = widgets.VBox(self.vbox)
        self.vbox.layout.align_items = 'center'
        self.members["fig"] = self.fig
        self.members["ax"] = self.ax

        # Connect changes in axes limits to resampling function
        self.ax.callbacks.connect('xlim_changed', self.check_for_xlim_update)
        self.ax.callbacks.connect('ylim_changed', self.check_for_ylim_update)

        if self.picking:
            # Connect picking events
            self.fig.canvas.mpl_connect('pick_event',
                                        self.keep_or_delete_profile)
            self.fig.canvas.mpl_connect('motion_notify_event',
                                        self.update_profile)

        return

    def make_data_array_with_bin_edges(self):
        da_with_edges = detail.move_to_data_array(
            data=sc.Variable(dims=self.data_array.dims,
                             unit=sc.units.counts,
                             values=self.data_array.values,
                             variances=self.data_array.variances,
                             dtype=sc.dtype.float32))
        for dim, coord in self.slider_coord[self.name].items():
            if self.histograms[self.name][dim][dim]:
                da_with_edges.coords[dim] = coord
            else:
                da_with_edges.coords[dim] = to_bin_edges(coord, dim)
        if len(self.masks[self.name]) > 0:
            for m in self.masks[self.name]:
                da_with_edges.masks[m] = self.data_array.masks[m]
        return da_with_edges

    def make_default_imshow(self, cmap, picker=None):
        return self.ax.imshow([[1.0, 1.0], [1.0, 1.0]],
                              norm=self.params["values"][self.name]["norm"],
                              extent=np.array(list(
                                  self.extent.values())).flatten(),
                              origin="lower",
                              aspect=self.aspect,
                              interpolation="nearest",
                              cmap=cmap,
                              picker=picker)

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
        return

    def update_axes(self):
        # Go through the buttons and select the right coordinates for the axes
        for dim, button in self.buttons.items():
            if self.slider[dim].disabled:
                but_val = button.value
                if but_val is not None:
                    but_val = but_val.lower()
                    self.extent[but_val] = self.slider_xlims[
                        self.name][dim].values
                    self.axparams[but_val]["lims"] = self.extent[but_val].copy(
                    )
                    if getattr(self, "log" +
                               but_val) and (self.extent[but_val][0] <= 0):
                        self.axparams[but_val]["lims"][
                            0] = 1.0e-03 * self.axparams[but_val]["lims"][1]
                    self.axparams[but_val]["labels"] = name_with_unit(
                        self.slider_label[self.name][dim]["coord"],
                        name=self.slider_label[self.name][dim]["name"])
                    self.axparams[but_val]["dim"] = dim
                    # Get the dimensions corresponding to the x/y buttons
                    self.button_dims[but_val == "x"] = button.dim
                    self.dim_to_xy[dim] = but_val

        extent_array = np.array(list(self.extent.values())).flatten()
        self.current_lims['x'] = extent_array[:2]
        self.current_lims['y'] = extent_array[2:]

        # TODO: if labels are used on a 2D coordinates, we need to update
        # the axes tick formatter to use xyrebin coords
        for xy, param in self.axparams.items():
            # Create coordinate axes for resampled array to be used as image
            offset = 2 * (xy == "y")
            self.xyrebin[xy] = sc.Variable(
                dims=[param["dim"]],
                values=np.linspace(extent_array[0 + offset],
                                   extent_array[1 + offset],
                                   self.image_resolution[xy] + 1),
                unit=self.slider_coord[self.name][param["dim"]].unit)

        # Set axes labels
        self.ax.set_xlabel(self.axparams["x"]["labels"])
        self.ax.set_ylabel(self.axparams["y"]["labels"])
        for xy, param in self.axparams.items():
            axis = getattr(self.ax, "{}axis".format(xy))
            is_log = getattr(self, "log{}".format(xy))
            axis.set_major_formatter(
                self.slider_axformatter[self.name][param["dim"]][is_log])
            axis.set_major_locator(
                self.slider_axlocator[self.name][param["dim"]][is_log])

        # Set axes limits and ticks
        with warnings.catch_warnings():
            warnings.filterwarnings("ignore", category=UserWarning)
            self.image.set_extent(extent_array)
            if len(self.masks[self.name]) > 0:
                for m in self.masks[self.name]:
                    self.members["masks"][m].set_extent(extent_array)
            self.ax.set_xlim(self.axparams["x"]["lims"])
            self.ax.set_ylim(self.axparams["y"]["lims"])

        # If there are no multi-d coords, we update the edges and widths only
        # once here.
        if not self.contains_multid_coord[self.name]:
            self.slice_coords()
        # Update the image using resampling
        self.update_slice()

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
                    alist[0] = (*self.slider_xlims[self.name][
                        self.button_dims[1]].values, *self.slider_xlims[
                            self.name][self.button_dims[0]].values)
                    # Insert the new tuple
                    self.fig.canvas.toolbar._nav_stack._elements[0][
                        key] = tuple(alist)

        # Clear profile axes if present and reset to None
        if self.profile_viewer is not None:
            del self.profile_viewer
            self.ax_picking.clear()
            self.profile_viewer = None
            if self.profile_scatter is not None:
                self.ax.collections = []
                self.fig.canvas.draw_idle()
                del self.profile_scatter
                self.profile_scatter = None

        return

    def compute_bin_widths(self, xy, dim):
        """
        Pixel widths used for scaling before rebin step
        """
        self.xywidth[xy] = (self.xyedges[xy][dim, 1:] -
                            self.xyedges[xy][dim, :-1])
        self.xywidth[xy].unit = sc.units.one

    def slice_coords(self):
        """
        Recursively slice the coords along the dimensions of active sliders.
        """
        self.cslice = self.slider_coord[self.name].copy()
        for key in self.cslice:
            # Slice along dimensions with active sliders
            for dim, val in self.slider.items():
                if not val.disabled and val.dim in self.cslice[key].dims:
                    self.cslice[key] = self.cslice[key][val.dim, val.value]

        # Update the xyedges and xywidth
        for xy, param in self.axparams.items():
            # Create bin-edge coordinates in the case of non bin-edges, since
            # rebin only accepts bin edges.
            if not self.histograms[self.name][param["dim"]][param["dim"]]:
                self.xyedges[xy] = to_bin_edges(self.cslice[param["dim"]],
                                                param["dim"])
            else:
                self.xyedges[xy] = self.cslice[param["dim"]].astype(
                    sc.dtype.float64)
            # Pixel widths used for scaling before rebin step
            self.compute_bin_widths(xy, param["dim"])

    def slice_data(self):
        """
        Recursively slice the data along the dimensions of active sliders.
        """
        data_slice = self.data_array

        # Slice along dimensions with active sliders
        for dim, val in self.slider.items():
            if not val.disabled:
                self.lab[dim].value = self.make_slider_label(
                    self.slider_label[self.name][dim]["coord"], val.value)
                data_slice = data_slice[val.dim, val.value]

                # If a profile viewer is present, we also update the position
                # of the slice on the profile plot.
                if self.slice_pos_rectangle is not None:
                    new_pos = self.slider_coord[
                        self.name][dim][dim, val.value].value
                    self.slice_pos_rectangle.set_x(new_pos)
                    if self.histograms[self.name][dim][dim]:
                        self.slice_pos_rectangle.set_width(self.slider_coord[
                            self.name][dim][dim, val.value + 1].value -
                                                           new_pos)
                    else:
                        new_pos -= 0.5 * self.slice_pos_rectangle.get_width()
                    self.slice_pos_rectangle.set_x(new_pos)

        # In the case of unaligned data, we may want to auto-scale the colorbar
        # as we slice through dimensions. Colorbar limits are allowed to grow
        # but not shrink.
        if data_slice.unaligned is not None:
            data_slice = sc.histogram(data_slice)
            data_slice.variances = None
            self.autoscale_cbar = True
        else:
            self.autoscale_cbar = False

        self.vslice = detail.move_to_data_array(
            data=sc.Variable(dims=data_slice.dims,
                             unit=sc.units.counts,
                             values=data_slice.values,
                             variances=data_slice.variances,
                             dtype=sc.dtype.float32))
        self.vslice.coords[self.xyrebin["x"].dims[0]] = self.xyedges["x"]
        self.vslice.coords[self.xyrebin["y"].dims[0]] = self.xyedges["y"]

        # Also include masks
        if len(data_slice.masks) > 0:
            for m in data_slice.masks:
                self.vslice.masks[m] = data_slice.masks[m]

        # Scale by bin width and then rebin in both directions
        # Note that this has to be written as 2 inplace operations to avoid
        # creation of large 2D temporary from broadcast
        self.vslice *= self.xywidth["x"]
        self.vslice *= self.xywidth["y"]

    def update_slice(self, change=None):
        """
        Slice data according to new slider value and update the image.
        """
        # If there are multi-d coords in the data we also need to slice the
        # coords and update the xyedges and xywidth
        if self.contains_multid_coord[self.name]:
            self.slice_coords()
        self.slice_data()
        # Update image with resampling
        self.update_image()
        return

    def toggle_mask(self, change):
        self.members["masks"][change["owner"].masks_name].set_visible(
            change["new"])
        if self.profile_viewer is not None:
            self.profile_viewer[self.profile_key].masks[self.name][
                change["owner"].masks_name].value = change["new"]
        return

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
            *sorted(self.slider_xlims[self.name][self.button_dims[1]].values))
        xylims["y"] = np.clip(
            self.ax.get_ylim(),
            *sorted(self.slider_xlims[self.name][self.button_dims[0]].values))

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
                self.xyrebin[xy] = sc.Variable(
                    dims=[param["dim"]],
                    values=np.linspace(xylims[xy][0], xylims[xy][1],
                                       self.image_resolution[xy] + 1),
                    unit=self.slider_coord[self.name][param["dim"]].unit)
            self.update_image(extent=np.array(list(xylims.values())).flatten())
        return

    def select_bins(self, coord, dim, start, end):
        bins = coord.shape[-1]
        if len(coord.dims) != 1:  # TODO find combined min/max
            return dim, slice(0, bins - 1)
        # scipp treats bins as closed on left and open on right: [left, right)
        first = sc.sum(coord <= start, dim).value - 1
        last = bins - sc.sum(coord > end, dim).value
        if first >= last:  # TODO better handling for decreasing
            return dim, slice(0, bins - 1)
        first = max(0, first)
        last = min(bins - 1, last)
        return dim, slice(first, last + 1)

    def resample_image(self, array, coord_edges, rebin_edges):
        dslice = array
        # Select bins to speed up rebinning
        for dim in rebin_edges:
            this_slice = self.select_bins(coord_edges[dim], dim,
                                          rebin_edges[dim][dim, 0],
                                          rebin_edges[dim][dim, -1])
            dslice = dslice[this_slice]

        # Rebin the data
        for dim, edges in rebin_edges.items():
            dslice = sc.rebin(dslice, dim, edges)

        # Divide by pixel width
        for key, edges in rebin_edges.items():
            self.image_pixel_size[key] = edges.values[1] - edges.values[0]
            dslice /= self.image_pixel_size[key]
        return dslice

    def update_image(self, extent=None):
        # The order of the dimensions that are rebinned matters if 2D coords
        # are present. We must rebin the base dimension of the 2D coord first.
        xy = "yx"
        if len(self.vslice.coords[self.button_dims[1]].dims) > 1:
            xy = "xy"

        rebin_edges = {
            self.xyrebin[xy[0]].dims[0]: self.xyrebin[xy[0]],
            self.xyrebin[xy[1]].dims[0]: self.xyrebin[xy[1]]
        }

        resampled_image = self.resample_image(self.vslice,
                                              coord_edges={
                                                  self.xyrebin[xy[0]].dims[0]:
                                                  self.xyedges[xy[0]],
                                                  self.xyrebin[xy[1]].dims[0]:
                                                  self.xyedges[xy[1]]
                                              },
                                              rebin_edges=rebin_edges)

        # Use Scipp's automatic transpose to match the image x/y axes
        # TODO: once transpose is available for DataArrays,
        # use sc.transpose(dslice, self.button_dims) instead.
        shape = [
            self.xyrebin["y"].shape[0] - 1, self.xyrebin["x"].shape[0] - 1
        ]
        self.dslice = sc.DataArray(coords=rebin_edges,
                                   data=sc.Variable(dims=self.button_dims,
                                                    values=np.ones(shape),
                                                    variances=np.zeros(shape),
                                                    dtype=self.vslice.dtype,
                                                    unit=sc.units.one))

        self.dslice *= resampled_image

        # Update the matplotlib image data
        arr = self.dslice.values
        self.image.set_data(arr)
        if extent is not None:
            self.image.set_extent(extent)

        # Handle masks
        if len(self.masks[self.name]) > 0:
            # Use scipp's automatic broadcast functionality to broadcast
            # lower dimension masks to higher dimensions.
            # TODO: creating a Variable here could become expensive when
            # sliders are being used. We could consider performing the
            # automatic broadcasting once and store it in the Slicer class,
            # but this could create a large memory overhead if the data is
            # large.
            # Here, the data is at most 2D, so having the Variable creation
            # and broadcasting should remain cheap.
            base_mask = sc.Variable(dims=self.dslice.dims,
                                    values=np.ones(self.dslice.shape,
                                                   dtype=np.int32))
            for m in self.masks[self.name]:
                msk = base_mask * sc.Variable(
                    dims=self.dslice.masks[m].dims,
                    values=self.dslice.masks[m].values.astype(np.int32))
                self.members["masks"][m].set_data(
                    self.mask_to_float(msk.values, arr))
                if extent is not None:
                    self.members["masks"].set_extent(extent)

        if self.autoscale_cbar:
            cbar_params = parse_params(globs=self.vminmax,
                                       array=arr,
                                       min_val=self.global_vmin,
                                       max_val=self.global_vmax)
            self.global_vmin = cbar_params["vmin"]
            self.global_vmax = cbar_params["vmax"]
            self.params["values"][self.name]["norm"] = cbar_params["norm"]
            self.image.set_norm(self.params["values"][self.name]["norm"])
            if len(self.masks[self.name]) > 0:
                for m in self.masks[self.name]:
                    self.members["masks"][m].set_norm(
                        self.params["values"][self.name]["norm"])

    def compute_profile(self, event):
        # Find indices of pixel where cursor lies
        dimx = self.xyrebin["x"].dims[0]
        dimy = self.xyrebin["y"].dims[0]
        ix = int((event.xdata - self.current_lims["x"][0]) /
                 self.image_pixel_size[dimx])
        iy = int((event.ydata - self.current_lims["y"][0]) /
                 self.image_pixel_size[dimy])
        # Resample the 3d cube down to a 1d profile
        return self.resample_image(self.da_with_edges,
                                   coord_edges={
                                       dimy: self.da_with_edges.coords[dimy],
                                       dimx: self.da_with_edges.coords[dimx]
                                   },
                                   rebin_edges={
                                       dimy:
                                       self.xyrebin["y"][dimy, iy:iy + 2],
                                       dimx: self.xyrebin["x"][dimx, ix:ix + 2]
                                   })[dimy, 0][dimx, 0]

    def create_profile_viewer(self, prof):
        # We need to extract the data again and replace with the original
        # coordinates, because coordinates have been forced to be bin-edges
        # so that rebin could be used. Also reset original unit.
        to_plot = sc.DataArray(data=sc.Variable(dims=prof.dims,
                                                unit=self.data_array.unit,
                                                values=prof.values,
                                                variances=prof.variances))
        for dim in prof.dims:
            to_plot.coords[dim] = self.slider_coord[self.name][dim]
        if len(prof.masks) > 0:
            for m in prof.masks:
                to_plot.masks[m] = prof.masks[m]
        self.profile_viewer = plot({self.name: to_plot},
                                   ax=self.ax_picking,
                                   logy=self.log)
        self.profile_key = list(self.profile_viewer.keys())[0]
        return to_plot

    def update_profile(self, event):
        if event.inaxes == self.ax:
            prof = self.compute_profile(event)
            if self.profile_viewer is None:
                to_plot = self.create_profile_viewer(prof)

                # Add indicator of range covered by current slice
                dim = to_plot.dims[0]
                xlims = self.ax_picking.get_xlim()
                ylims = self.ax_picking.get_ylim()
                left = to_plot.coords[dim][dim, self.slider[dim].value].value
                if self.histograms[self.name][dim][dim]:
                    width = (
                        to_plot.coords[dim][dim, self.slider[dim].value + 1] -
                        to_plot.coords[dim][dim, self.slider[dim].value]).value
                else:
                    width = 0.01 * (xlims[1] - xlims[0])
                    left -= 0.5 * width
                self.slice_pos_rectangle = Rectangle((left, ylims[0]),
                                                     width,
                                                     ylims[1] - ylims[0],
                                                     facecolor="lightgray",
                                                     zorder=-10)
                self.ax_picking.add_patch(self.slice_pos_rectangle)

            else:
                self.profile_viewer[self.profile_key].update_slice(
                    {"vslice": {
                        self.name: prof
                    }})
            self.toggle_visibility_of_hover_plot(True)
        elif self.profile_viewer is not None:
            self.toggle_visibility_of_hover_plot(False)

    def toggle_visibility_of_hover_plot(self, value):
        # If the mouse moves off the image, we hide the profile. If it moves
        # back onto the image, we show the profile
        self.profile_viewer[self.profile_key].members["lines"][
            self.name].set_visible(value)
        if self.profile_viewer[self.profile_key].errorbars[self.name]:
            for item in self.profile_viewer[
                    self.profile_key].members["error_y"][self.name]:
                if item is not None:
                    for it in item:
                        it.set_visible(value)
        mask_dict = self.profile_viewer[self.profile_key].members["masks"][
            self.name]
        if len(mask_dict) > 0:
            for m in mask_dict:
                mask_dict[m].set_visible(value if self.profile_viewer[
                    self.profile_key].masks[self.name][m].value else False)
                mask_dict[m].set_gid("onaxes" if value else "offaxes")

    def keep_or_delete_profile(self, event):
        if isinstance(event.artist, PathCollection):
            self.delete_profile(event)
            self.profile_update_lock = True
        elif self.profile_update_lock:
            self.profile_update_lock = False
        else:
            self.keep_profile(event)

    def keep_profile(self, event):
        trace = list(
            self.profile_viewer[self.profile_key].keep_buttons.values())[-1]
        xdata = event.mouseevent.xdata
        ydata = event.mouseevent.ydata
        if self.profile_scatter is None:
            self.profile_scatter = self.ax.scatter([xdata], [ydata],
                                                   c=[trace[2].value],
                                                   picker=5)
        else:
            new_offsets = np.concatenate(
                (self.profile_scatter.get_offsets(), [[xdata, ydata]]), axis=0)
            col = np.array(_hex_to_rgb(trace[2].value) + [255],
                           dtype=np.float) / 255.0
            new_colors = np.concatenate(
                (self.profile_scatter.get_facecolors(), [col]), axis=0)
            self.profile_scatter.set_offsets(new_offsets)
            self.profile_scatter.set_facecolors(new_colors)
        self.profile_viewer[self.profile_key].keep_trace(trace[1])

    def delete_profile(self, event):
        ind = event.ind[0]
        xy = np.delete(self.profile_scatter.get_offsets(), ind, axis=0)
        c = np.delete(self.profile_scatter.get_facecolors(), ind, axis=0)
        self.profile_scatter.set_offsets(xy)
        self.profile_scatter.set_facecolors(c)
        self.fig.canvas.draw_idle()
        # Also remove the line from the 1d plot
        trace = list(
            self.profile_viewer[self.profile_key].keep_buttons.values())[ind]
        self.profile_viewer[self.profile_key].remove_trace(trace[1])
