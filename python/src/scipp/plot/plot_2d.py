# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import config
from .render import render_plot
from .slicer import Slicer
from .tools import centers_to_edges, edges_to_centers, parse_params
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
        self.transp = False
        self.xlim_updated = False
        self.ylim_updated = False
        self.current_lims = {"x": np.zeros(2), "y": np.zeros(2)}
        self.button_dims = [None, None]
        self.dim_to_xy = {}
        self.output = widgets.Label()
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
            self.cbar.ax.set_ylabel(
                name_with_unit(var=self.data_array, name=""))
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
        print(self.ax.get_xlim())
        self.update_slice(None)
        # self.output = widgets.Label()
        self.vbox = widgets.VBox(self.vbox + [self.output])
        self.vbox.layout.align_items = 'center'
        self.members["fig"] = self.fig
        self.members["ax"] = self.ax

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
        # update_slice no longer needed because it reacts to change in axis limits?
        # -> won't trigger if axes limits are the same for x and y?
        self.update_slice(None)

        return

    def update_axes(self):
        # Go through the buttons and select the right coordinates for the axes
        # axparams = {"x": {}, "y": {}}
        for dim, button in self.buttons.items():
            if self.slider[dim].disabled:
                but_val = button.value.lower()
                # if not self.histograms[self.name][dim]:
                #     xc = self.slider_x[self.name][dim].values
                #     if self.slider_nx[self.name][dim] < 2:
                #         dx = 0.5 * abs(xc[0])
                #         if dx == 0.0:
                #             dx = 0.5
                #         xmin = xc[0] - dx
                #         xmax = xc[0] + dx
                #         # self.axparams[but_val]["xmin"] = xmin
                #         # self.axparams[but_val]["xmax"] = xmax
                #     else:
                #         xmin = 1.5 * xc[0] - 0.5 * xc[1]
                #         xmax = 1.5 * xc[-1] - 0.5 * xc[-2]
                #     self.extent[but_val] = [xmin, xmax]
                # else:
                #     self.extent[but_val] = self.slider_x[
                #         self.name][dim].values[[0, -1]].astype(np.float)
                self.extent[but_val] = self.slider_xlims[self.name][dim]

                self.axparams[but_val]["lims"] = self.extent[but_val].copy()
                if getattr(self,
                           "log" + but_val) and (self.extent[but_val][0] <= 0):
                    # if not self.histograms[self.name][dim]:
                    #     new_x = centers_to_edges(xc)
                    # else:
                    #     new_x = edges_to_centers(
                    #         self.slider_x[self.name][dim].values)
                    new_x = self.slider_coord[self.name][dim].values
                    self.axparams[but_val]["lims"][0] = new_x[np.searchsorted(
                        new_x, 0)]
                self.axparams[but_val]["labels"] = name_with_unit(
                    self.slider_coord[self.name][dim], name=str(dim))
                self.axparams[but_val]["dim"] = dim
            # else:
                # Get the dimensions of the dimension-coordinates, since
                # buttons can contain non-dimension coordinates
                # self.button_dims[self.buttons[dim].value.lower() ==
                #             "x"] = self.slider_coord[self.name][val.dim].dims
                self.button_dims[self.buttons[dim].value.lower() ==
                            "x"] = self.buttons[dim].dim
                self.dim_to_xy[dim] = self.buttons[dim].value.lower()

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

            # Create bin-edge coordinates in the case of non bin-edges, since
            # rebin only accepts bin edges.
            # xydims = self.xyrebin[xy].dims
            if not self.histograms[self.name][param["dim"]][param["dim"]]:
                # Special handling for 2D coordinates
                if len(self.slider_coord[self.name][param["dim"]].dims) > 1:
                    shp = self.slider_coord[self.name][param["dim"]].shape
                    idim = self.slider_coord[self.name][param["dim"]].dims.index(dim)
                    shp[idim] += 1
                    edges = np.zeros(*shp)
                    print(edges.shape)
                    # for 
                self.xyedges[xy] = sc.Variable(
                    dims=[param["dim"]],
                    values=centers_to_edges(
                        self.slider_coord[self.name][param["dim"]].values),
                    unit=self.slider_coord[self.name][param["dim"]].unit)
            else:
                self.xyedges[xy] = self.slider_coord[self.name][param["dim"]].astype(
                    sc.dtype.float64)
            print("==========================")
            print(self.xyedges)
            print("==========================")

            # Pixel widths used for scaling before rebin step
            self.xywidth[xy] = (self.xyedges[xy][param["dim"], 1:] -
                                self.xyedges[xy][param["dim"], :-1]) / (
                                    self.xyrebin[xy][param["dim"], 1] -
                                    self.xyrebin[xy][param["dim"], 0])
            self.xywidth[xy].unit = sc.units.one


        self.ax.set_xlabel(self.axparams["x"]["labels"])
        self.ax.set_ylabel(self.axparams["y"]["labels"])
        for xy, param in self.axparams.items():
            getattr(self.ax, "{}axis".format(xy)).set_major_formatter(
                self.slider_axformatter[self.name][param["dim"]][getattr(
                    self, "log{}".format(xy))])
            getattr(self.ax, "{}axis".format(xy)).set_major_locator(
                self.slider_axlocator[self.name][param["dim"]][getattr(
                    self, "log{}".format(xy))])


        # Set axes limits and ticks
        with warnings.catch_warnings():
            warnings.filterwarnings("ignore", category=UserWarning)

            # TODO: check if this set_extent is still needed
            self.im["values"].set_extent(extent_array)
            if self.params["masks"][self.name]["show"]:
                self.im["masks"].set_extent(extent_array)

            self.ax.set_xlim(self.axparams["x"]["lims"])
            self.ax.set_ylim(self.axparams["y"]["lims"])

        # Some annoying house-keeping when using X/Y buttons: we need to update
        # the deeply embedded limits set by the Home button in the matplotlib
        # toolbar. The home button actually brings the first element in the
        # navigation stack to the top, so we need to modify the first element
        # in the navigation stack in-place.

        if len(self.fig.canvas.toolbar._nav_stack._elements) > 0:
            # Get the first key in the navigation stack
            key = list(self.fig.canvas.toolbar._nav_stack._elements[0].keys())[0]
            # Construct a new tuple for replacement
            alist = []
            for x in self.fig.canvas.toolbar._nav_stack._elements[0][key]:
                alist.append(x)
            alist[0] = (self.xyedges['x'].values[0], self.xyedges['x'].values[-1],
                        self.xyedges['y'].values[0], self.xyedges['y'].values[-1])
            self.fig.canvas.toolbar._nav_stack._elements[0][key] = tuple(alist)

        return

    def update_slice(self, change):
        """
        Slice data according to new slider value.
        """
        self.vslice = self.data_array
        if self.params["masks"][self.name]["show"]:
            self.mslice = self.masks
        # Slice along dimensions with active sliders
        # button_dims = [None, None]
        for dim, val in self.slider.items():
            if not val.disabled:
                self.lab[dim].value = self.make_slider_label(
                    self.slider_coord[self.name][dim], val.value)
                self.vslice = self.vslice[val.dim, val.value]
                # At this point, after masks were combined, all their
                # dimensions should be contained in the data_array.dims.
                if self.params["masks"][
                        self.name]["show"] and dim in self.mslice.dims:
                    self.mslice = self.mslice[val.dim, val.value]
            # else:
            #     # Get the dimensions of the dimension-coordinates, since
            #     # buttons can contain non-dimension coordinates
            #     # self.button_dims[self.buttons[dim].value.lower() ==
            #     #             "x"] = self.slider_coord[self.name][val.dim].dims
            #     self.button_dims[self.buttons[dim].value.lower() ==
            #                 "x"] = self.buttons[dim].dim

        # # Check if dimensions of arrays agree, if not, plot the transpose
        # for dim_list in self.button_dims:
        #     if len(dim_list) > 1:

        # self.transp = self.vslice.dims != self.button_dims

        # Use scipp automatic transpose?

        # TODO: should do this AFTER the resample, to avoid multiplying a really large array

        # arr = sc.DataArray(coords={
        #     self.button_dims[0]: self.slider_coord[self.name][self.button_dims[0]],
        #     self.button_dims[1]: self.slider_coord[self.name][self.button_dims[1]]
        # },
        #                       data=sc.Variable(dims=self.button_dims,
        #                   values=np.ones([self.slider_shape[self.name][self.button_dims[0]][self.button_dims[0]] - self.histograms[self.name][self.button_dims[0]][self.button_dims[0]],
        #                                   self.slider_shape[self.name][self.button_dims[1]][self.button_dims[1]] - self.histograms[self.name][self.button_dims[1]][self.button_dims[1]]]),
        #                   dtype=self.vslice.dtype,
        #                   unit=sc.units.one))


        # # arr = sc.Variable(dims=self.button_dims,
        # #                   values=np.ones([self.slider_shape[self.name][self.button_dims[0]][self.button_dims[0]],
        # #                                   self.slider_shape[self.name][self.button_dims[1]][self.button_dims[1]]]),
        # #                   dtype=self.vslice.dtype,
        # #                   unit=sc.units.one)
        # print(arr)
        # print(self.vslice)
        # arr *= self.vslice
        # self.vslice = arr
        # msk = msk.values
        # if self.transp:
        #     msk = msk.T

        # In the case of unaligned data, we may want to auto-scale the colorbar
        # as we slice through dimensions. Colorbar limits are allowed to grow
        # but not shrink.
        autoscale_cbar = False
        if self.vslice.unaligned is not None:
            self.vslice = sc.histogram(self.vslice)
            autoscale_cbar = True

        # is_not_linspace = {
        #     "x": not sc.is_linspace(self.xyedges["x"]),
        #     "y": not sc.is_linspace(self.xyedges["y"])
        # }
        is_not_linspace = {
            "x": True,
            "y": True
        }

        # dslice = self.resample_image()
        # # if np.any(list(is_not_linspace.values())):
        # #     # Make a new slice with bin edges and counts (for rebin), and with
        # #     # non-dimension coordinates if requested.
        # #     xy = "xy"
        # #     vslice = sc.DataArray(coords={
        # #         self.xyedges["x"].dims[0]: self.xyedges["x"],
        # #         self.xyedges["y"].dims[0]: self.xyedges["y"]
        # #     },
        # #                           data=sc.Variable(dims=[
        # #                               self.xyedges[xy[not transp]].dims[0],
        # #                               self.xyedges[xy[transp]].dims[0]
        # #                           ],
        # #                                            values=vslice.values,
        # #                                            unit=sc.units.counts))

        # #     # Also include the masks
        # #     if self.params["masks"][self.name]["show"]:
        # #         mslice_dims = []
        # #         for dim in mslice.dims:
        # #             if dim == button_dims[0]:
        # #                 mslice_dims.append(self.xyedges["y"].dims[0])
        # #             elif dim == button_dims[1]:
        # #                 mslice_dims.append(self.xyedges["x"].dims[0])
        # #             else:
        # #                 mslice_dims.append(dim)
        # #         vslice.masks["all"] = sc.Variable(dims=mslice_dims,
        # #                                           values=mslice.values)

        # # # Scale by bin width and then rebin in both directions
        # # if is_not_linspace["x"]:
        # #     vslice *= self.xywidth["x"]
        # #     vslice = sc.rebin(vslice, self.xyrebin["x"].dims[0],
        # #                       self.xyrebin["x"])
        # # if is_not_linspace["y"]:
        # #     vslice *= self.xywidth["y"]
        # #     vslice = sc.rebin(vslice, self.xyrebin["y"].dims[0],
        # #                       self.xyrebin["y"])

        # # # Remember to replace masks slice after rebin
        # # if np.any(list(is_not_linspace.values())):
        # #     if self.params["masks"][self.name]["show"]:
        # #         mslice = vslice.masks["all"]

        # if self.params["masks"][self.name]["show"]:
        #     # Use scipp's automatic broadcast functionality to broadcast
        #     # lower dimension masks to higher dimensions.
        #     # TODO: creating a Variable here could become expensive when
        #     # sliders are being used. We could consider performing the
        #     # automatic broadcasting once and store it in the Slicer class,
        #     # but this could create a large memory overhead if the data is
        #     # large.
        #     # Here, the data is at most 2D, so having the Variable creation
        #     # and broadcasting should remain cheap.
        #     msk = sc.Variable(dims=dslice.dims,
        #                       values=np.ones(dslice.shape, dtype=np.int32))
        #     msk *= sc.Variable(dims=dslice.masks["all"].dims,
        #                        values=dslice.masks["all"].values.astype(np.int32))
        #     msk = msk.values
        #     if self.transp:
        #         msk = msk.T

        # arr = dslice.values
        # if self.transp:
        #     arr = arr.T
        # self.im["values"].set_data(arr)

        self.update_image()


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
            # self.im["masks"].set_data(self.mask_to_float(msk, arr))
            # if autoscale_cbar:
                self.im["masks"].set_norm(self.params["values"][self.name]["norm"])

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
        self.ax.set_title("haha")
        self.xlim_updated = False
        self.ylim_updated = False
        # self.output.value = "scipp"
        xylims = {"x": np.array(self.ax.get_xlim()),
                  "y": np.array(self.ax.get_ylim())}
        self.ax.set_title('got to here 0.1')

        # Make sure we don't overrun the original array bounds
        # xylims["x"][0] = max(xylims["x"][0], self.xyedges["x"].values[0])
        # xylims["x"][1] = min(xylims["x"][1], self.xyedges["x"].values[-1])
        # xylims["y"][0] = max(xylims["y"][0], self.xyedges["y"].values[0])
        # xylims["y"][1] = min(xylims["y"][1], self.xyedges["y"].values[-1])
        xylims["x"][0] = max(xylims["x"][0], self.slider_xlims[self.name][self.button_dims[1]][0])
        self.ax.set_title('got to here 0.11')
        xylims["x"][1] = min(xylims["x"][1], self.slider_xlims[self.name][self.button_dims[1]][1])
        self.ax.set_title('got to here 0.12')
        xylims["y"][0] = max(xylims["y"][0], self.slider_xlims[self.name][self.button_dims[0]][0])
        self.ax.set_title('got to here 0.13')
        xylims["y"][1] = min(xylims["y"][1], self.slider_xlims[self.name][self.button_dims[0]][1])
        self.ax.set_title('got to here 0.2')

        dx = self.current_lims["x"][1] - self.current_lims["x"][0]
        dy = self.current_lims["y"][1] - self.current_lims["y"][0]
        self.ax.set_title('got to here 0.3')
        # self.output.value = str(np.array(xylims.values()).flatten())
        # new_lims = np.array(list(xylims.values())).flatten()
        diffx = np.abs(self.current_lims["x"] - xylims["x"]) / dx
        diffy = np.abs(self.current_lims["y"] - xylims["y"]) / dy
        self.ax.set_title('got to here 0.4')
        diff = diffx.sum() + diffy.sum()
        # diff = np.sum(
        #     np.abs(self.current_lims - new_lims) /
        #     np.abs(self.current_lims))
        self.ax.set_title(str(diff))
        # return
        # return
        if diff > 0.1:
            self.current_lims = xylims
            # self.ax.set_title("{}\n{}".format(str(xylims["x"]), str(xylims["y"])))
            # self.fig.text(np.random.rand(), np.random.rand(), str(np.random.rand()))
            for xy, param in self.axparams.items():
                self.ax.set_title('got to here 1')
                # Create coordinate axes for resampled array to be used as image
                # offset = 2 * (xy == "y")
                self.xyrebin[xy] = sc.Variable(
                    dims=[param["dim"]],
                    values=np.linspace(xylims[xy][0],
                                       xylims[xy][1],
                                       self.image_resolution[xy] + 1),
                    unit=self.slider_coord[self.name][param["dim"]].unit)
                self.ax.set_title('got to here 2')

                # # # Create bin-edge coordinates in the case of non bin-edges, since
                # # # rebin only accepts bin edges.
                # # xydims = self.xyrebin[xy].dims
                # # if not self.histograms[self.name][xydims[0]]:
                # #     self.xyedges[xy] = sc.Variable(
                # #         dims=xydims,
                # #         values=centers_to_edges(
                # #             self.slider_x[self.name][xydims[0]].values),
                # #         unit=self.slider_x[self.name][xydims[0]].unit)
                # # else:
                # #     self.xyedges[xy] = self.slider_x[self.name][xydims[0]].astype(
                # #         sc.dtype.float64)

                # Pixel widths used for scaling before rebin step
                self.xywidth[xy] = (self.xyedges[xy][param["dim"], 1:] -
                                    self.xyedges[xy][param["dim"], :-1]) / (
                                        self.xyrebin[xy][param["dim"], 1] -
                                        self.xyrebin[xy][param["dim"], 0])
                self.xywidth[xy].unit = sc.units.one
                self.ax.set_title('got to here 3')

            # dslice = self.resample_image()

            # if self.params["masks"][self.name]["show"]:
            #     # Use scipp's automatic broadcast functionality to broadcast
            #     # lower dimension masks to higher dimensions.
            #     # TODO: creating a Variable here could become expensive when
            #     # sliders are being used. We could consider performing the
            #     # automatic broadcasting once and store it in the Slicer class,
            #     # but this could create a large memory overhead if the data is
            #     # large.
            #     # Here, the data is at most 2D, so having the Variable creation
            #     # and broadcasting should remain cheap.
            #     msk = sc.Variable(dims=dslice.dims,
            #                       values=np.ones(dslice.shape, dtype=np.int32))
            #     msk *= sc.Variable(dims=dslice.masks["all"].dims,
            #                        values=dslice.masks["all"].values.astype(np.int32))
            #     msk = msk.values
            #     if self.transp:
            #         msk = msk.T

            # arr = dslice.values
            # if self.transp:
            #     arr = arr.T
            # self.im["values"].set_data(arr)
            # self.im["values"].set_extent(np.array(list(xylims.values())).flatten())
            # if self.params["masks"][self.name]["show"]:
            #     self.im["masks"].set_data(self.mask_to_float(msk, arr))
            #     self.im["masks"].set_extent(np.array(list(xylims.values())).flatten())
            self.ax.set_title('got to here 4')
            self.ax.set_title(str(np.array(list(xylims.values())).flatten()))
            # return
            self.update_image(extent=np.array(list(xylims.values())).flatten())
            # return
            self.ax.set_title('got to here 5')




    def resample_image(self):
        # if np.any(list(is_not_linspace.values())):
        # return self.vslice

        # Make a new slice with bin edges and counts (for rebin), and with
        # non-dimension coordinates if requested.
        # TODO: use sc.resample once it is implemented.
        # xy = "xy"
        # dslice = sc.DataArray(coords={
        #     self.xyedges["x"].dims[0]: self.xyedges["x"],
        #     self.xyedges["y"].dims[0]: self.xyedges["y"]
        # },
        #                       data=sc.Variable(dims=[
        #                           self.xyedges[xy[not self.transp]].dims[0],
        #                           self.xyedges[xy[self.transp]].dims[0]
        #                       ],
        #                                        values=self.vslice.values,
        #                                        unit=sc.units.counts))
        print(self.dim_to_xy)
        dslice = sc.DataArray(coords={
            self.xyrebin["x"].dims[0]: self.xyedges["x"],
            self.xyrebin["y"].dims[0]: self.xyedges["y"]
        },
                              data=sc.Variable(dims=[
                                  self.xyrebin[self.dim_to_xy[self.vslice.dims[0]]].dims[0],
                                  self.xyrebin[self.dim_to_xy[self.vslice.dims[1]]].dims[0]
                              ],
                                               values=self.vslice.values,
                                               unit=sc.units.counts))

        # print(dslice)

        # Also include the masks
        if self.params["masks"][self.name]["show"]:
            mslice_dims = []
            for dim in self.mslice.dims:
                if dim == self.button_dims[0]:
                    mslice_dims.append(self.xyedges["y"].dims[0])
                elif dim == self.button_dims[1]:
                    mslice_dims.append(self.xyedges["x"].dims[0])
                else:
                    mslice_dims.append(dim)
            dslice.masks["all"] = sc.Variable(dims=mslice_dims,
                                              values=self.mslice.values)

        # Scale by bin width and then rebin in both directions
        # if is_not_linspace["x"]:
        # self.output.value = str(self.xyrebin["x"])
        dslice *= self.xywidth["x"]*self.xywidth["y"]
        # print("########## DSLICE #########")
        # print(dslice)
        # print(self.xyrebin["x"].dims[0])
        # print(self.xyrebin["x"])
        # print("########## DSLICE END #########")

        # The order of the dimensions that are rebinned matters if 2D coords
        # are present. We must rebin the base dimension of the 2D coord first.
        xy = "yx"
        # print("len(dslice.coords[self.button_dims[1]].dims)", len(dslice.coords[self.button_dims[1]].dims))
        if len(dslice.coords[self.button_dims[1]].dims) > 1:
            xy = "xy"
        dslice = sc.rebin(dslice, self.xyrebin[xy[0]].dims[0],
                              self.xyrebin[xy[0]])
        # # if is_not_linspace["y"]:
        # dslice *= self.xywidth["y"]
        dslice = sc.rebin(dslice, self.xyrebin[xy[1]].dims[0],
                              self.xyrebin[xy[1]])

        # if self.params["masks"][self.name]["show"]:
        #         mslice = dslice.masks["all"]


        arr = sc.DataArray(coords={
            self.xyrebin["x"].dims[0]: self.xyrebin["x"],
            self.xyrebin["y"].dims[0]: self.xyrebin["y"],
        },
                              data=sc.Variable(dims=self.button_dims,
                          values=np.ones([self.xyrebin["y"].shape[0] - 1,
                                          self.xyrebin["x"].shape[0] - 1]),
                          dtype=dslice.dtype,
                          unit=sc.units.one))
        # print(arr)
        # print(dslice)
        arr *= dslice
        # dslice = arr

        return arr


    def update_image(self, extent=None):
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
                               values=dslice.masks["all"].values.astype(np.int32))
            msk = msk.values
            # if self.transp:
            #     msk = msk.T

        arr = dslice.values
        # if self.transp:
        #     arr = arr.T
        self.im["values"].set_data(arr)
        if extent is not None:
            self.im["values"].set_extent(extent)
        if self.params["masks"][self.name]["show"]:
            self.im["masks"].set_data(self.mask_to_float(msk, arr))
            if extent is not None:
                self.im["masks"].set_extent(extent)
