# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import config
from .render import render_plot
from .slicer import Slicer
from .tools import centers_to_edges, edges_to_centers, parse_params
from ..utils import name_with_unit
from .._scipp import core as sc

# Other imports
import numpy as np
import ipywidgets as widgets
import matplotlib.pyplot as plt
import warnings


def plot_2d(scipp_obj_dict=None,
            axes=None,
            values=None,
            variances=None,
            masks=None,
            filename=None,
            figsize=None,
            mpl_axes=None,
            aspect=None,
            cmap=None,
            log=False,
            vmin=None,
            vmax=None,
            color=None,
            logx=False,
            logy=False,
            logxy=False):
    """
    Plot a 2D slice through a N dimensional dataset. For every dimension above
    2, a slider is created to adjust the position of the slice in that
    particular dimension.
    """

    sv = Slicer2d(scipp_obj_dict=scipp_obj_dict,
                  axes=axes,
                  values=values,
                  variances=variances,
                  masks=masks,
                  mpl_axes=mpl_axes,
                  aspect=aspect,
                  cmap=cmap,
                  log=log,
                  vmin=vmin,
                  vmax=vmax,
                  color=color,
                  logx=logx or logxy,
                  logy=logy or logxy)

    if mpl_axes is None:
        render_plot(figure=sv.fig, widgets=sv.vbox, filename=filename)

    return sv.members


class Slicer2d(Slicer):
    def __init__(self,
                 scipp_obj_dict=None,
                 axes=None,
                 values=None,
                 variances=None,
                 masks=None,
                 mpl_axes=None,
                 aspect=None,
                 cmap=None,
                 log=None,
                 vmin=None,
                 vmax=None,
                 color=None,
                 logx=False,
                 logy=False):

        super().__init__(scipp_obj_dict=scipp_obj_dict,
                         axes=axes,
                         values=values,
                         variances=variances,
                         masks=masks,
                         cmap=cmap,
                         log=log,
                         vmin=vmin,
                         vmax=vmax,
                         color=color,
                         aspect=aspect,
                         button_options=['X', 'Y'])

        self.members.update({"images": {}, "colorbars": {}})
        self.extent = {"x": [1, 2], "y": [1, 2]}
        self.logx = logx
        self.logy = logy
        self.vminmax = {"vmin": vmin, "vmax": vmax}
        self.global_vmin = np.Inf
        self.global_vmax = np.NINF
        self.image_resolution = 0.64 * config.plot.dpi / 96.0
        self.image_resolution = [
            int(self.image_resolution * config.plot.width),
            int(self.image_resolution * config.plot.height)
        ]
        self.xyrebin = {}
        self.xyedges = {}
        self.xywidth = {}

        # Get or create matplotlib axes
        self.fig = None
        cax = [None] * (1 + self.params["variances"][self.name]["show"])
        if mpl_axes is not None:
            if isinstance(mpl_axes, dict):
                ax = [None, None]
                for key, val in mpl_axes.items():
                    if key == "ax" or key == "ax_values":
                        ax[0] = val
                    if key == "cax" or key == "cax_values":
                        cax[0] = val
                    if key == "ax_variances":
                        ax[1] = val
                    if key == "cax_variances":
                        cax[1] = val
            else:
                # Case where only a single axis is given
                ax = [mpl_axes]
        else:
            self.fig, ax = plt.subplots(
                1,
                1 + self.params["variances"][self.name]["show"],
                figsize=(config.plot.width / config.plot.dpi,
                         config.plot.height /
                         (1.0 + self.params["variances"][self.name]["show"]) /
                         config.plot.dpi),
                dpi=config.plot.dpi,
                sharex=True,
                sharey=True)
            if not self.params["variances"][self.name]["show"]:
                ax = [ax]

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
                self.ax[key].set_title(self.name if key ==
                                       "values" else "std dev.")
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
        self.update_slice(None)
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
        self.update_slice(None)

        return

    def update_axes(self):
        # Go through the buttons and select the right coordinates for the axes
        axparams = {"x": {}, "y": {}}
        for dim, button in self.buttons.items():
            if self.slider[dim].disabled:
                but_val = button.value.lower()
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

        # Create coordinate axes for resampled array to be used as image
        self.xyrebin["x"] = sc.Variable(
            dims=[axparams['x']["dim"]],
            values=np.linspace(extent_array[0], extent_array[1],
                               self.image_resolution[0] + 1),
            unit=self.slider_x[self.name][axparams['x']["dim"]].unit)
        self.xyrebin["y"] = sc.Variable(
            dims=[axparams['y']["dim"]],
            values=np.linspace(extent_array[2], extent_array[3],
                               self.image_resolution[1] + 1),
            unit=self.slider_x[self.name][axparams['y']["dim"]].unit)
        self.image_dx = self.xyrebin["x"].values[1] - self.xyrebin["x"].values[
            0]
        self.image_dy = self.xyrebin["y"].values[1] - self.xyrebin["y"].values[
            0]

        # Create bin-edge coordinates in the case of non bin-edges, since rebin
        # only accepts bin edges.
        xdims = self.xyrebin["x"].dims
        if not self.histograms[self.name][xdims[0]]:
            self.xyedges["x"] = sc.Variable(
                dims=xdims,
                values=centers_to_edges(
                    self.slider_x[self.name][xdims[0]].values),
                unit=self.slider_x[self.name][xdims[0]].unit)
        else:
            self.xyedges["x"] = self.slider_x[self.name][xdims[0]].astype(
                sc.dtype.float64)
        ydims = self.xyrebin["y"].dims
        if not self.histograms[self.name][ydims[0]]:
            self.xyedges["y"] = sc.Variable(
                dims=ydims,
                values=centers_to_edges(
                    self.slider_x[self.name][ydims[0]].values),
                unit=self.slider_x[self.name][ydims[0]].unit)
        else:
            self.xyedges["y"] = self.slider_x[self.name][ydims[0]].astype(
                sc.dtype.float64)

        # Compute bin widths for normalization pre-rebin in order to retain the
        # pixel values instead of spreading the data over the pixels.
        self.xywidth["x"] = sc.Variable(
            dims=self.xyrebin["x"].dims,
            values=np.ediff1d(self.xyedges["x"].values) / self.image_dx)
        self.xywidth["y"] = sc.Variable(
            dims=self.xyrebin["y"].dims,
            values=np.ediff1d(self.xyedges["y"].values) / self.image_dy)

        # Set axes limits and ticks
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
        #             self.ax.xaxis.set_major_formatter(self.slider_axformatter[self.name][dim][self.logx])
        # self.ax.xaxis.set_major_locator(self.slider_axlocator[self.name][dim][self.logx])

            print(axparams)
            print(self.slider_x[self.name])
            # print(self.slider_x[self.name]['y'])

            for xy, param in axparams.items():
                getattr(self.ax[key], "{}axis".format(xy)).set_major_formatter(self.slider_axformatter[self.name][dim][getattr(self, "log{}".format(xy))])
                print(self.slider_axlocator[self.name][dim][getattr(self, "log{}".format(xy))])
                getattr(self.ax[key], "{}axis".format(xy)).set_major_locator(self.slider_axlocator[self.name][dim][getattr(self, "log{}".format(xy))])
                
                # self.ax.xaxis.set_major_locator(self.slider_axlocator[self.name][dim][self.logx])

                # if self.slider_ticks[self.name][param["dim"]] is not None:
                #     getattr(self.ax[key], "set_{}ticklabels".format(xy))(
                #         self.get_custom_ticks(ax=self.ax[key],
                #                               dim=param["dim"],
                #                               xy=xy))
        return

    def update_slice(self, change):
        """
        Slice data according to new slider value.
        """
        dslice = self.data_array
        if self.params["masks"][self.name]["show"]:
            mslice = self.masks
        # Slice along dimensions with active sliders
        button_dims = [None, None]
        for dim, val in self.slider.items():
            if not val.disabled:
                self.lab[dim].value = self.make_slider_label(
                    self.slider_x[self.name][dim], val.value)
                dslice = dslice[val.dim, val.value]
                # At this point, after masks were combined, all their
                # dimensions should be contained in the data_array.dims.
                if self.params["masks"][self.name]["show"]:
                    mslice = mslice[val.dim, val.value]
            else:
                # Get the dimensions of the dimension-coordinates, since
                # buttons can contain non-dimension coordinates
                button_dims[self.buttons[dim].value.lower() ==
                            "x"] = self.slider_x[self.name][val.dim].dims[0]

        # Check if dimensions of arrays agree, if not, plot the transpose
        transp = dslice.dims != button_dims

        # In the case of unaligned data, we may want to auto-scale the colorbar
        # as we slice through dimensions. Colorbar limits are allowed to grow
        # but not shrink.
        autoscale_cbar = False
        if dslice.unaligned is not None:
            dslice = sc.histogram(dslice)
            autoscale_cbar = True

        # Make a new slice with bin edges and counts (for rebin), and with
        # non-dimension coordinates if requested.
        xy = "xy"
        vslice = sc.DataArray(coords={
            self.xyedges["x"].dims[0]: self.xyedges["x"],
            self.xyedges["y"].dims[0]: self.xyedges["y"]
        },
                              data=sc.Variable(dims=[
                                  self.xyedges[xy[not transp]].dims[0],
                                  self.xyedges[xy[transp]].dims[0]
                              ],
                                               values=dslice.values,
                                               variances=dslice.variances,
                                               unit=sc.units.counts))

        # Also include the masks
        if self.params["masks"][self.name]["show"]:
            mslice_dims = []
            for dim in mslice.dims:
                if dim == button_dims[0]:
                    mslice_dims.append(self.xyedges["y"].dims[0])
                elif dim == button_dims[1]:
                    mslice_dims.append(self.xyedges["x"].dims[0])
                else:
                    mslice_dims.append(dim)
            vslice.masks["all"] = sc.Variable(dims=mslice_dims,
                                              values=mslice.values)

        # The scaling by bin width and rebin operations below modify the
        # variances in the data, so here we have to manually split the values
        # and variances into separate Variables. We store them in a dict.
        #
        # TODO: Having to rebin once for values and once for variances
        # potentially slows things down quite a bit. In the future, we should
        # use resample instead of rebin, once it will be implemented.
        to_image = {}
        if self.params["variances"][self.name]["show"]:
            eslice = vslice.copy()
            eslice.values = eslice.variances
            eslice.variances = None
            vslice.variances = None
            eslice *= self.xywidth["x"] * self.xywidth["y"]
            eslice = sc.rebin(eslice, self.xyrebin["x"].dims[0],
                              self.xyrebin["x"])
            eslice = sc.rebin(eslice, self.xyrebin["y"].dims[0],
                              self.xyrebin["y"])
            to_image["variances"] = np.sqrt(eslice.values)

        # Scale by bin width and then rebin in both directions
        vslice *= self.xywidth["x"] * self.xywidth["y"]
        vslice = sc.rebin(vslice, self.xyrebin["x"].dims[0], self.xyrebin["x"])
        vslice = sc.rebin(vslice, self.xyrebin["y"].dims[0], self.xyrebin["y"])
        to_image["values"] = vslice.values

        if self.params["masks"][self.name]["show"]:
            mslice = vslice.masks["all"]
            # Use scipp's automatic broadcast functionality to broadcast
            # lower dimension masks to higher dimensions.
            # TODO: creating a Variable here could become expensive when
            # sliders are being used. We could consider performing the
            # automatic broadcasting once and store it in the Slicer class,
            # but this could create a large memory overhead if the data is
            # large.
            # Here, the data is at most 2D, so having the Variable creation
            # and broadcasting should remain cheap.
            msk = sc.Variable(dims=vslice.dims,
                              values=np.ones(vslice.shape, dtype=np.int32))
            msk *= sc.Variable(dims=mslice.dims,
                               values=mslice.values.astype(np.int32))
            msk = msk.values
            if transp:
                msk = msk.T

        for key in self.ax.keys():
            arr = to_image[key]
            if transp:
                arr = arr.T
            self.im[key].set_data(arr)
            if autoscale_cbar:
                cbar_params = parse_params(globs=self.vminmax,
                                           array=arr,
                                           min_val=self.global_vmin,
                                           max_val=self.global_vmax)
                self.global_vmin = cbar_params["vmin"]
                self.global_vmax = cbar_params["vmax"]
                self.params[key][self.name]["norm"] = cbar_params["norm"]
                self.im[key].set_norm(self.params[key][self.name]["norm"])
            if self.params["masks"][self.name]["show"]:
                self.im[self.get_mask_key(key)].set_data(
                    self.mask_to_float(msk, arr))
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
