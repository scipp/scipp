# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import config
from .plot_1d import Slicer1d
from .render import render_plot
from .slicer import Slicer
from .tools import to_bin_edges, parse_params
from .._utils import name_with_unit
from .._scipp import core as sc
from .. import detail

# Other imports
import numpy as np
import ipywidgets as widgets
import matplotlib.pyplot as plt
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
            extra_dims="sliders"):
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
                  extra_dims=extra_dims)

    if ax is None:
        render_plot(figure=sv.fig, widgets=sv.vbox, filename=filename)

    return sv.members


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
                 extra_dims=None):

        super().__init__(scipp_obj_dict=scipp_obj_dict,
                         axes=axes,
                         masks=masks,
                         cmap=cmap,
                         log=log,
                         vmin=vmin,
                         vmax=vmax,
                         color=color,
                         aspect=aspect,
                         button_options=['X', 'Y'],
                         extra_dims=extra_dims)

        self.members["images"] = {}
        self.axparams = {"x": {}, "y": {}}
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
        self.button_dims = [None, None]
        self.dim_to_xy = {}
        self.cslice = None

        extra_dims_options = {"sliders": 0, "profiles": 1}
        self.extra_dims = extra_dims_options[extra_dims]
        # if extra_dims not in extra_dims_options:
        #     raise RuntimeError("Unknown handling method for extra_dims: {}. "
        #                        "Possible choices are {}.".format(extra_dims, extra_dims_options))
        # if 
        # self.use_slider_for_extra_dims = True


        # self.profile = profile
        # if self.profile is not None:
        #     self.profile = sc.Dim(self.profile)
        # self.first_profile_plotted = False
        self.profile_viewer = None

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
        # self.ax_extra_dims = None
        self.cax = cax
        print('got to here 1')
        if self.ax is None:
            # nrows = 1 + (self.profile is not None)
            self.fig, self.ax = plt.subplots(
                1,
                1,
                figsize=(config.plot.width / config.plot.dpi,
                         config.plot.height / config.plot.dpi),
                dpi=config.plot.dpi)
            # if self.profile is not None:
            #     self.ax = mpl_axes[0]
            #     self.ax_profile = mpl_axes[1]
            # else:
            #     self.ax = mpl_axes
        print('got to here 2')

        self.im = dict()
        self.cbar = None
        print('got to here 3')

        self.im["values"] = self.make_default_imshow(
            self.params["values"][self.name]["cmap"])
        self.ax.set_title(self.name)
        if self.params["values"][self.name]["cbar"]:
            self.cbar = plt.colorbar(self.im["values"],
                                     ax=self.ax,
                                     cax=self.cax)
            self.cbar.set_label(name_with_unit(var=self.data_array, name=""))
        if self.cax is None:
            self.cbar.ax.yaxis.set_label_coords(-1.1, 0.5)
        self.members["images"] = self.im["values"]
        self.members["colorbar"] = self.cbar
        if self.params["masks"][self.name]["show"]:
            self.im["masks"] = self.make_default_imshow(
                cmap=self.params["masks"][self.name]["cmap"])
        if self.logx:
            self.ax.set_xscale("log")
        if self.logy:
            self.ax.set_yscale("log")
        print('got to here 4')

        # Call update_slice once to make the initial image
        self.update_axes()
        print('got to here 5')
        self.vbox = widgets.VBox(self.vbox)
        self.vbox.layout.align_items = 'center'
        self.members["fig"] = self.fig
        self.members["ax"] = self.ax

        # Connect changes in axes limits to resampling function
        self.ax.callbacks.connect('xlim_changed', self.check_for_xlim_update)
        self.ax.callbacks.connect('ylim_changed', self.check_for_ylim_update)

        # if self.profile is not None:
        #     # Connect profile plot to mouse cursor
        #     self.fig.canvas.mpl_connect('button_press_event', self.update_profile)
        #     # self.fig.canvas.mpl_connect('motion_notify_event', self.update_profile)

        return

    def make_default_imshow(self, cmap):
        return self.ax.imshow([[1.0, 1.0], [1.0, 1.0]],
                              norm=self.params["values"][self.name]["norm"],
                              extent=np.array(list(
                                  self.extent.values())).flatten(),
                              origin="lower",
                              aspect=self.aspect,
                              interpolation="nearest",
                              cmap=cmap)

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
            print(dim, self.slider[dim].disabled)
            if self.slider[dim].disabled:
                but_val = button.value
                print(dim, button.value)
                if but_val is not None:
                    but_val = but_val.lower()
                    self.extent[but_val] = self.slider_xlims[self.name][dim].values
                    self.axparams[but_val]["lims"] = self.extent[but_val].copy()
                    if getattr(self,
                               "log" + but_val) and (self.extent[but_val][0] <= 0):
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
        print(self.axparams)
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
            self.im["values"].set_extent(extent_array)
            if self.params["masks"][self.name]["show"]:
                self.im["masks"].set_extent(extent_array)
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
                    alist[0] = (
                        *self.slider_xlims[self.name][self.button_dims[1]].values,
                        # self.slider_xlims[self.name][self.button_dims[1]][1],
                        *self.slider_xlims[self.name][self.button_dims[0]].values)
                        # self.slider_xlims[self.name][self.button_dims[0]][1])

                        # self.slider_xlims[self.name][self.button_dims[1]][0],
                        # self.slider_xlims[self.name][self.button_dims[1]][1],
                        # self.slider_xlims[self.name][self.button_dims[0]][0],
                        # self.slider_xlims[self.name][self.button_dims[0]][1])
                    # self.slider_xlims[self.name][self.button_dims[1]][0]
                    # Insert the new tuple
                    self.fig.canvas.toolbar._nav_stack._elements[0][
                        key] = tuple(alist)

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
        self.vslice = self.data_array
        if self.params["masks"][self.name]["show"]:
            self.mslice = self.masks

        if self.extra_dims:
            self.vslice = self.vslice.astype(sc.dtype.float32)
            self.vslice.unit = sc.units.counts
            # detail.move_to_data_array(
            #     data=sc.Variable(dims=self.vslice.dims,
            #                      unit=sc.units.counts,
            #                      values=self.vslice.values,
            #                      dtype=sc.dtype.float32))
            # self.vslice.coords[self.xyrebin["x"].dims[0]] = self.xyedges["x"]
            # self.vslice.coords[self.xyrebin["y"].dims[0]] = self.xyedges["y"]

        # Slice along dimensions with active sliders
        for dim, val in self.slider.items():
            if not val.disabled:
                # Resample instead of slicing
                if self.extra_dims:
                    # self.vslice = self.vslice[val.dim, val.value]
                    self.vslice = self.resample_image(self.vslice,
                        coord_edges={dim: self.slider_coord[self.name][dim]},
                        rebin_edges={dim: self.slider_xlims[self.name][dim]})[dim, 0]
                    depth = self.slider_xlims[self.name][dim][dim, 1] - self.slider_xlims[self.name][dim][dim, 0]
                    depth.unit = sc.units.one
                    self.vslice *= depth
                else:
                    self.lab[dim].value = self.make_slider_label(
                        self.slider_label[self.name][dim]["coord"], val.value)
                    self.vslice = self.vslice[val.dim, val.value]
                    # At this point, after masks were combined, all their
                    # dimensions should be contained in the data_array.dims.
                    if self.params["masks"][
                            self.name]["show"] and dim in self.mslice.dims:
                        self.mslice = self.mslice[val.dim, val.value]
        # In the case of unaligned data, we may want to auto-scale the colorbar
        # as we slice through dimensions. Colorbar limits are allowed to grow
        # but not shrink.
        if self.vslice.unaligned is not None:
            self.vslice = sc.histogram(self.vslice)
            self.vslice.variances = None
            self.autoscale_cbar = True
        else:
            self.autoscale_cbar = False
        self.vslice = detail.move_to_data_array(
            data=sc.Variable(dims=self.vslice.dims,
                             unit=sc.units.counts,
                             values=self.vslice.values,
                             dtype=sc.dtype.float32))
        self.vslice.coords[self.xyrebin["x"].dims[0]] = self.xyedges["x"]
        self.vslice.coords[self.xyrebin["y"].dims[0]] = self.xyedges["y"]

        # if self.profile is not None:
        #     # print(self.slider_coord[self.name])
        #     # print(self.slider_coord[self.name][self.profile])
        #     self.vslice.coords[self.profile] = self.slider_coord[self.name][self.profile]

        if self.params["masks"][self.name]["show"]:
            self.vslice.masks["all"] = self.mslice
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
        # Update imade with resampling
        self.update_image()
        return

    def toggle_mask(self, change):
        self.members["masks"][change["owner"].masks_group][change["owner"].masks_name].set_visible(change["new"])
        # self.im["masks"].set_visible(change["new"])
        # change["owner"].description = "Hide masks" if change["new"] else \
        #     "Show masks"
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
            *sorted(self.slider_xlims[self.name][self.button_dims[1]]))
        xylims["y"] = np.clip(
            self.ax.get_ylim(),
            *sorted(self.slider_xlims[self.name][self.button_dims[0]]))

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
        for dim in rebin_edges:
            this_slice = self.select_bins(coord_edges[dim], dim,
                                  rebin_edges[dim][dim, 0],
                                  rebin_edges[dim][dim, -1])
            dslice = dslice[this_slice]

        # dim = self.xyrebin['x'].dims[0]
        # slicex = self.select_bins(self.xyedges['x'], dim,
        #                           self.xyrebin['x'][dim, 0],
        #                           self.xyrebin['x'][dim, -1])
        # dim = self.xyrebin['y'].dims[0]
        # slicey = self.select_bins(self.xyedges['y'], dim,
        #                           self.xyrebin['y'][dim, 0],
        #                           self.xyrebin['y'][dim, -1])
        # dslice = self.vslice[slicex][slicey]

        # # The order of the dimensions that are rebinned matters if 2D coords
        # # are present. We must rebin the base dimension of the 2D coord first.
        # xy = "yx"
        # if len(dslice.coords[self.button_dims[1]].dims) > 1:
        #     xy = "xy"

        for dim, edges in rebin_edges.items():
            print(dslice, dim, edges)
            dslice = sc.rebin(dslice, dim, edges)




        # dslice = sc.rebin(dslice, self.xyrebin[xy[0]].dims[0],
        #                   self.xyrebin[xy[0]])
        # dslice = sc.rebin(dslice, self.xyrebin[xy[1]].dims[0],
        #                   self.xyrebin[xy[1]])





        # slice_dims = self.button_dims.copy()
        # slice_shape = [self.xyrebin["y"].shape[0] - 1,
        #                self.xyrebin["x"].shape[0] - 1]
        # if self.profile is not None:
        #     slice_dims.append(self.profile)
        #     slice_shape.append(self.slider_shape[self.name][self.profile][self.profile])
        # print("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~")
        # print(slice_dims)
        # print(slice_shape)
        # print("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~")


        # # Use Scipp's automatic transpose to match the image x/y axes
        # # TODO: once transpose is available for DataArrays,
        # # use sc.transpose(dslice, self.button_dims) instead.

        # arr = sc.DataArray(coords=rebin_edges,
        #                    data=sc.Variable(dims=self.button_dims,
        #                                     values=np.ones([self.xyrebin["y"].shape[0] - 1,
        #                self.xyrebin["x"].shape[0] - 1]),
        #                                     dtype=dslice.dtype,
        #                                     unit=sc.units.one))
        # # if self.profile is not None:
        # #     arr.coords[self.profile] = self.slider_coord[self.name][self.profile]
        # arr *= dslice



        # Scale by output bins width
        # for key in self.xyrebin:
        #     self.image_pixel_size[key] = self.xyrebin[key].values[1] - self.xyrebin[key].values[0]
        #     dslice /= self.image_pixel_size[key]

        for key, edges in rebin_edges.items():
            self.image_pixel_size[key] = edges.values[1] - edges.values[0]
            dslice /= self.image_pixel_size[key]
        # arr /= self.xyrebin['y'].values[1] - self.xyrebin['y'].values[0]
        return dslice

    def update_image(self, extent=None):
        # The order of the dimensions that are rebinned matters if 2D coords
        # are present. We must rebin the base dimension of the 2D coord first.
        xy = "yx"
        if len(self.vslice.coords[self.button_dims[1]].dims) > 1:
            xy = "xy"

        rebin_edges = {self.xyrebin[xy[0]].dims[0]: self.xyrebin[xy[0]],
                         self.xyrebin[xy[1]].dims[0]: self.xyrebin[xy[1]]}

        im = self.resample_image(self.vslice,
            coord_edges={self.xyrebin[xy[0]].dims[0]: self.xyedges[xy[0]],
                         self.xyrebin[xy[1]].dims[0]: self.xyedges[xy[1]]},
            rebin_edges=rebin_edges)

        # Use Scipp's automatic transpose to match the image x/y axes
        # TODO: once transpose is available for DataArrays,
        # use sc.transpose(dslice, self.button_dims) instead.
        self.dslice = sc.DataArray(coords=rebin_edges,
                           data=sc.Variable(dims=self.button_dims,
                                            values=np.ones([self.xyrebin["y"].shape[0] - 1,
                       self.xyrebin["x"].shape[0] - 1]),
                                            dtype=self.vslice.dtype,
                                            unit=sc.units.one))
        # if self.profile is not None:
        #     arr.coords[self.profile] = self.slider_coord[self.name][self.profile]

        self.dslice *= im

        # return
        if self.params["masks"][self.name]["show"]:
            # Use scipp's automatic broadcast functionality to broadcast
            # lower dimension masks to higher dimensions.
            # TODO: creating a Variable here could become expensive when
            # sliders are being used. We could consider performing the
            # automatic broadcasting once and store it in the Slicer class,
            # but this could create a large memory overhead if the data is
            # large.
            # Here, the data is at most 2D, so having the Variable creation
            # and broadcasting should remain cheap.
            msk = sc.Variable(dims=self.dslice.dims,
                              values=np.ones(self.dslice.shape, dtype=np.int32))
            msk *= sc.Variable(dims=self.dslice.masks["all"].dims,
                               values=self.dslice.masks["all"].values.astype(
                                   np.int32))
            # msk = msk.values

        print(self.dslice)
        # if self.profile is not None:
        #     arr = sc.sum(self.dslice, self.profile).values
        #     if self.params["masks"][self.name]["show"]:
        #         msk = sc.sum(msk, self.profile).values
        # else:
        arr = self.dslice.values
        if self.params["masks"][self.name]["show"]:
            msk = msk.values

        self.im["values"].set_data(arr)
        if extent is not None:
            self.im["values"].set_extent(extent)
        if self.params["masks"][self.name]["show"]:
            self.im["masks"].set_data(self.mask_to_float(msk, arr))
            if extent is not None:
                self.im["masks"].set_extent(extent)

        if self.autoscale_cbar:
            cbar_params = parse_params(globs=self.vminmax,
                                       array=arr,
                                       min_val=self.global_vmin,
                                       max_val=self.global_vmax)
            self.global_vmin = cbar_params["vmin"]
            self.global_vmax = cbar_params["vmax"]
            self.params["values"][self.name]["norm"] = cbar_params["norm"]
            self.im["values"].set_norm(
                self.params["values"][self.name]["norm"])
            if self.params["masks"][self.name]["show"]:
                self.im["masks"].set_norm(
                    self.params["values"][self.name]["norm"])

        return

    def update_profile(self, event):
        # Find indices of pixel where cursor lies
        ix = int((event.xdata - self.current_lims["x"][0]) / self.image_pixel_size["x"])
        iy = int((event.ydata - self.current_lims["y"][0]) / self.image_pixel_size["y"])

        # Slice the 3d cube down to a 1d profile
        prof = self.dslice[self.button_dims[0], iy][self.button_dims[1], ix]

        prof = rebin_edges = {self.xyrebin[xy[0]].dims[0]: self.xyrebin[xy[0]],
                         self.xyrebin[xy[1]].dims[0]: self.xyrebin[xy[1]]}

        prof = self.resample_image(self.data_array,
            coord_edges={self.xyrebin[xy[0]].dims[0]: self.xyedges[xy[0]],
                         self.xyrebin[xy[1]].dims[0]: self.xyedges[xy[1]]},
            rebin_edges=rebin_edges)

        # self.ax_profile.set_title('hahahaha')
        if not self.first_profile_plotted:
            # _ = plot_1d({self.name: prof}, ax=self.ax_profile,
            #             mpl_line_params={
            #                 "color": {self.name: config.plot.color[0]},
            #                 "marker": {self.name: config.plot.marker[0]},
            #                 "linestyle": {self.name: config.plot.linestyle[0]},
            #                 "linewidth": {self.name: config.plot.linewidth[0]},
            #                 })

            self.profile_viewer = Slicer1d({self.name: prof}, ax=self.ax_profile,
                        mpl_line_params={
                            "color": {self.name: config.plot.color[0]},
                            "marker": {self.name: config.plot.marker[0]},
                            "linestyle": {self.name: config.plot.linestyle[0]},
                            "linewidth": {self.name: config.plot.linewidth[0]},
                            })

            self.first_profile_plotted = True
        else:
            # self.ax_profile.set_title('members lines 1')
            # print(self.profile_viewer.members["lines"])
            # self.ax_profile.set_title('members lines 2')
            # print(self.profile_viewer.members["lines"][self.name])
            # self.ax_profile.set_title('members lines 3')
            self.profile_viewer.members["lines"][self.name].set_ydata(prof.values)
        #     self.ax_profile.plot(self.slider_coord[self.name][self.profile].values, prof.values)
