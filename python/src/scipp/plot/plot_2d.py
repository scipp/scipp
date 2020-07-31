# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import config
from .render import render_plot
from .slicer import Slicer
from .tools import to_bin_edges, parse_params
from .._utils import name_with_unit
from .._scipp import core as sc

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
            resolution=None):
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
                  resolution=resolution)

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
                 resolution=None):

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

        self.members["images"] = {}
        self.axparams = {"x": {}, "y": {}}
        self.extent = {"x": [1, 2], "y": [1, 2]}
        self.logx = logx
        self.logy = logy
        self.vminmax = {"vmin": vmin, "vmax": vmax}
        self.global_vmin = np.Inf
        self.global_vmax = np.NINF
        self.vslice = None
        self.mslice = None
        self.xlim_updated = False
        self.ylim_updated = False
        self.current_lims = {"x": np.zeros(2), "y": np.zeros(2)}
        self.button_dims = [None, None]
        self.dim_to_xy = {}
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

        # Get or create matplotlib axes
        self.fig = None
        self.ax = ax
        self.cax = cax
        if self.ax is None:
            self.fig, self.ax = plt.subplots(
                1,
                1,
                figsize=(config.plot.width / config.plot.dpi,
                         config.plot.height / config.plot.dpi),
                dpi=config.plot.dpi)

        self.im = dict()
        self.cbar = None

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

        # Call update_slice once to make the initial image
        self.update_axes()
        self.vbox = widgets.VBox(self.vbox)
        self.vbox.layout.align_items = 'center'
        self.members["fig"] = self.fig
        self.members["ax"] = self.ax

        # Connect changes in axes limits to resampling function
        self.ax.callbacks.connect('xlim_changed', self.check_for_xlim_update)
        self.ax.callbacks.connect('ylim_changed', self.check_for_ylim_update)

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
            if self.slider[dim].disabled:
                but_val = button.value.lower()
                self.extent[but_val] = self.slider_xlims[self.name][dim]
                self.axparams[but_val]["lims"] = self.extent[but_val].copy()
                if getattr(self,
                           "log" + but_val) and (self.extent[but_val][0] <= 0):
                    new_x = self.slider_coord[self.name][dim].values
                    self.axparams[but_val]["lims"][0] = new_x[np.searchsorted(
                        new_x, 0)]
                self.axparams[but_val]["labels"] = name_with_unit(
                    self.slider_label[self.name][dim]["coord"],
                    name=self.slider_label[self.name][dim]["name"])
                self.axparams[but_val]["dim"] = dim
                # Get the dimensions corresponding to the x/y buttons
                self.button_dims[button.value.lower() == "x"] = button.dim
                self.dim_to_xy[dim] = button.value.lower()

        extent_array = np.array(list(self.extent.values())).flatten()
        self.current_lims['x'] = extent_array[:2]
        self.current_lims['y'] = extent_array[2:]

        for xy, param in self.axparams.items():
            # Create coordinate axes for resampled array to be used as image
            offset = 2 * (xy == "y")
            self.xyrebin[xy] = sc.Variable(
                dims=[param["dim"]],
                values=np.linspace(extent_array[0 + offset],
                                   extent_array[1 + offset],
                                   self.image_resolution[xy] + 1),
                unit=self.slider_coord[self.name][param["dim"]].unit)

            # TODO: if labels are used on a 2D coordinates, we need to update
            # the axes tick formatter to use xyrebin coords

            # Create bin-edge coordinates in the case of non bin-edges, since
            # rebin only accepts bin edges.
            if not self.histograms[self.name][param["dim"]][param["dim"]]:
                self.xyedges[xy] = to_bin_edges(
                    self.slider_coord[self.name][param["dim"]], param["dim"])
            else:
                self.xyedges[xy] = self.slider_coord[self.name][
                    param["dim"]].astype(sc.dtype.float64)

            # Pixel widths used for scaling before rebin step
            self.xywidth[xy] = (self.xyedges[xy][param["dim"], 1:] -
                                self.xyedges[xy][param["dim"], :-1]) / (
                                    self.xyrebin[xy][param["dim"], 1] -
                                    self.xyrebin[xy][param["dim"], 0])
            self.xywidth[xy].unit = sc.units.one

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
                        self.slider_xlims[self.name][self.button_dims[1]][0],
                        self.slider_xlims[self.name][self.button_dims[1]][1],
                        self.slider_xlims[self.name][self.button_dims[0]][0],
                        self.slider_xlims[self.name][self.button_dims[0]][1])
                    self.slider_xlims[self.name][self.button_dims[1]][0]
                    # Insert the new tuple
                    self.fig.canvas.toolbar._nav_stack._elements[0][
                        key] = tuple(alist)

        return

    def slice_data(self):
        """
        Recursively slice the data along the dimensions of active sliders.
        """
        self.vslice = self.data_array
        if self.params["masks"][self.name]["show"]:
            self.mslice = self.masks
        # Slice along dimensions with active sliders
        for dim, val in self.slider.items():
            if not val.disabled:
                self.lab[dim].value = self.make_slider_label(
                    self.slider_label[self.name][dim]["coord"], val.value)
                self.vslice = self.vslice[val.dim, val.value]
                # At this point, after masks were combined, all their
                # dimensions should be contained in the data_array.dims.
                if self.params["masks"][
                        self.name]["show"] and dim in self.mslice.dims:
                    self.mslice = self.mslice[val.dim, val.value]
        return

    def update_slice(self, change=None):
        """
        Slice data according to new slider value and update the image.
        """
        self.slice_data()
        self.update_image()
        return

    def toggle_masks(self, change):
        self.im["masks"].set_visible(change["new"])
        change["owner"].description = "Hide masks" if change["new"] else \
            "Show masks"
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
        xylims = {
            "x": np.array(self.ax.get_xlim()),
            "y": np.array(self.ax.get_ylim())
        }

        # Make sure we don't overrun the original array bounds
        xylims["x"][0] = max(
            xylims["x"][0],
            self.slider_xlims[self.name][self.button_dims[1]][0])
        xylims["x"][1] = min(
            xylims["x"][1],
            self.slider_xlims[self.name][self.button_dims[1]][1])
        xylims["y"][0] = max(
            xylims["y"][0],
            self.slider_xlims[self.name][self.button_dims[0]][0])
        xylims["y"][1] = min(
            xylims["y"][1],
            self.slider_xlims[self.name][self.button_dims[0]][1])
        dx = self.current_lims["x"][1] - self.current_lims["x"][0]
        dy = self.current_lims["y"][1] - self.current_lims["y"][0]
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

                # Pixel widths used for scaling before rebin step
                self.xywidth[xy] = (self.xyedges[xy][param["dim"], 1:] -
                                    self.xyedges[xy][param["dim"], :-1]) / (
                                        self.xyrebin[xy][param["dim"], 1] -
                                        self.xyrebin[xy][param["dim"], 0])
                self.xywidth[xy].unit = sc.units.one

            self.update_image(extent=np.array(list(xylims.values())).flatten())

        return

    def resample_image(self):

        # Make a new slice with bin edges and counts for using in rebin.
        dslice = sc.DataArray(
            coords={
                self.xyrebin["x"].dims[0]: self.xyedges["x"],
                self.xyrebin["y"].dims[0]: self.xyedges["y"]
            },
            data=sc.Variable(dims=[
                self.xyrebin[self.dim_to_xy[self.vslice.dims[0]]].dims[0],
                self.xyrebin[self.dim_to_xy[self.vslice.dims[1]]].dims[0]
            ],
                             values=self.vslice.values,
                             unit=sc.units.counts))

        # Also include the masks
        if self.params["masks"][self.name]["show"]:
            mslice_dims = []
            for dim in self.mslice.dims:
                if dim == self.button_dims[0]:
                    mslice_dims.append(self.xyrebin["y"].dims[0])
                elif dim == self.button_dims[1]:
                    mslice_dims.append(self.xyrebin["x"].dims[0])
                else:
                    mslice_dims.append(dim)

            dslice.masks["all"] = sc.Variable(dims=mslice_dims,
                                              values=self.mslice.values)

        # Scale by bin width and then rebin in both directions
        dslice *= self.xywidth["x"] * self.xywidth["y"]
        # The order of the dimensions that are rebinned matters if 2D coords
        # are present. We must rebin the base dimension of the 2D coord first.
        xy = "yx"
        if len(dslice.coords[self.button_dims[1]].dims) > 1:
            xy = "xy"
        dslice = sc.rebin(dslice, self.xyrebin[xy[0]].dims[0],
                          self.xyrebin[xy[0]])
        dslice = sc.rebin(dslice, self.xyrebin[xy[1]].dims[0],
                          self.xyrebin[xy[1]])

        # Use Scipp's automatic transpose to match the image x/y axes
        # TODO: once transpose is available for DataArrays,
        # use sc.transpose(dslice, self.button_dims) instead.
        arr = sc.DataArray(coords={
            self.xyrebin["x"].dims[0]: self.xyrebin["x"],
            self.xyrebin["y"].dims[0]: self.xyrebin["y"],
        },
                           data=sc.Variable(dims=self.button_dims,
                                            values=np.ones([
                                                self.xyrebin["y"].shape[0] - 1,
                                                self.xyrebin["x"].shape[0] - 1
                                            ]),
                                            dtype=dslice.dtype,
                                            unit=sc.units.one))
        arr *= dslice
        return arr

    def update_image(self, extent=None):

        # In the case of unaligned data, we may want to auto-scale the colorbar
        # as we slice through dimensions. Colorbar limits are allowed to grow
        # but not shrink.
        autoscale_cbar = False
        if self.vslice.unaligned is not None:
            self.vslice = sc.histogram(self.vslice)
            autoscale_cbar = True

        dslice = self.resample_image()
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
            msk = sc.Variable(dims=dslice.dims,
                              values=np.ones(dslice.shape, dtype=np.int32))
            msk *= sc.Variable(dims=dslice.masks["all"].dims,
                               values=dslice.masks["all"].values.astype(
                                   np.int32))
            msk = msk.values

        arr = dslice.values
        self.im["values"].set_data(arr)
        if extent is not None:
            self.im["values"].set_extent(extent)
        if self.params["masks"][self.name]["show"]:
            self.im["masks"].set_data(self.mask_to_float(msk, arr))
            if extent is not None:
                self.im["masks"].set_extent(extent)

        if autoscale_cbar:
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
