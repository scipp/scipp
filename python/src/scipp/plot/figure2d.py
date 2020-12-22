# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .. import config
from .figure import PlotFigure
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.colors import Normalize, LogNorm
import warnings


class PlotFigure2d(PlotFigure):
    """
    Class for 2 dimensional plots, based on Matplotlib's `imshow`.
    """
    def __init__(self,
                 ax=None,
                 cax=None,
                 figsize=None,
                 aspect=None,
                 cmap=None,
                 norm=None,
                 name=None,
                 cbar=None,
                 unit=None,
                 masks=None,
                 resolution=None,
                 extend=None,
                 title=None,
                 xlabel=None,
                 ylabel=None):

        super().__init__(ax=ax,
                         cax=cax,
                         figsize=figsize,
                         title=name if title is None else title,
                         ndim=2,
                         xlabel=xlabel,
                         ylabel=ylabel)

        if aspect is None:
            aspect = config.plot.aspect

        self.cmap = cmap
        self.norm = norm
        self.masks_cmap = masks["cmap"]
        # print(self.masks_cmap.name)

        # print(cmap)
        # print(norm)

        # self.image_colors = self.make_default_imshow(cmap=cmap,
        #                                       norm=norm,
        #                                       aspect=aspect,
        #                                       picker=5)

        # self.image_values = self.make_default_imshow(cmap=cmap,
        #                                       norm=norm,
        #                                       aspect=aspect,
        #                                       picker=5)

        ones = np.ones([2, 2])

        image_params = {"extent": [1, 2, 1, 2],
                        "origin": "lower",
                        "aspect": aspect,
                        "interpolation": "nearest"}


        self.image_colors = self.ax.imshow(self.cmap(self.norm(ones)),
                              zorder=1,
                              **image_params)

        self.image_values = self.ax.imshow(ones,
                              norm=self.norm,
                              cmap=self.cmap,
                              picker=5,
                              zorder=2,
                              alpha=0.0,
                              **image_params)
        # self.image_values.set_alpha(0.0)

# im2.set_alpha(0.0)
        # cbar = plt.colorbar(im2, ax=ax)
        # cbar.set_alpha(1.0)
        # cbar.draw_all()

        self.cbar = None
        if cbar:
            self.cbar = plt.colorbar(self.image_values,
                                     ax=self.ax,
                                     cax=self.cax,
                                     extend=extend)
            self.cbar.set_label(unit)
            # self.cbar.set_alpha(1.0)
            # self.cbar.draw_all()
        if self.cax is None:
            self.cbar.ax.yaxis.set_label_coords(-1.1, 0.5)
        self.mask_image = {}
        # for m in masks["names"]:
        #     self.mask_image[m] = self.make_default_imshow(cmap=masks["cmap"],
        #                                                   norm=norm,
        #                                                   aspect=aspect,
        #                                                   zorder=2)

    # def make_default_imshow(self,
    #                         cmap,
    #                         norm,
    #                         aspect=None,
    #                         picker=None,
    #                         zorder=1):
    #     """
    #     Make a base `imshow` object whose contents and extents will be later
    #     updated to display a 2d data array.
    #     This is used for both a data image and for masks images.
    #     """
    #     return self.ax.imshow([[1.0, 1.0], [1.0, 1.0]],
    #                           norm=norm,
    #                           extent=[1, 2, 1, 2],
    #                           origin="lower",
    #                           aspect=aspect,
    #                           interpolation="nearest",
    #                           cmap=cmap,
    #                           picker=picker,
    #                           zorder=zorder)

    def rescale_to_data(self, vmin, vmax):
        """
        Rescale the colorbar limits according to the supplied values.
        """
        # self.cmap.set_clim(vmin, vmax)
        # return
        self.norm.vmin = vmin
        self.norm.vmax = vmax
        self.image_values.set_clim(vmin, vmax)
        self.opacify_colorbar()
        # self.cbar.set_alpha(1.0)
        # self.cbar.draw_all()
        self.draw()
        return
        self.image.set_clim([vmin, vmax])
        for m, im in self.mask_image.items():
            im.set_clim([vmin, vmax])
        self.draw()

    def opacify_colorbar(self):
        self.cbar.set_alpha(1.0)
        self.cbar.draw_all()


    def toggle_mask(self, mask_name, visible):
        """
        Show or hide a given mask.
        """
        return
        im = self.mask_image[mask_name]
        if im.get_url() != "hide":
            im.set_visible(visible)
        self.draw()

    def update_axes(self, axparams=None):
        """
        Update axes labels, scales, tick locations and labels, as well as axes
        limits.
        """
        self.ax.set_xlabel(
            axparams["x"]["label"] if self.xlabel is None else self.xlabel)
        self.ax.set_ylabel(
            axparams["y"]["label"] if self.ylabel is None else self.ylabel)
        self.ax.set_xscale(axparams["x"]["scale"])
        self.ax.set_yscale(axparams["y"]["scale"])

        for xy, param in axparams.items():
            axis = getattr(self.ax, "{}axis".format(xy))
            axis.set_major_formatter(
                self.axformatter[param["dim"]][param["scale"]])
            axis.set_major_locator(
                self.axlocator[param["dim"]][param["scale"]])

        # Set axes limits and ticks
        extent_array = np.array([axparams["x"]["lims"],
                                 axparams["y"]["lims"]]).flatten()
        with warnings.catch_warnings():
            warnings.filterwarnings("ignore", category=UserWarning)
            self.image_colors.set_extent(extent_array)
            self.image_values.set_extent(extent_array)
            # for m, im in self.mask_image.items():
            #     im.set_extent(extent_array)
            self.ax.set_xlim(axparams["x"]["lims"])
            self.ax.set_ylim(axparams["y"]["lims"])

    def update_data(self, new_values, info=None):
        """
        Update image array with new values.
        """
        # self.image.set_data(new_values["values"])
        rgba = self.cmap(self.norm(new_values["values"]))
        # print(new_values["masks"])
        if "masks" in new_values:
            indices = np.where(new_values["masks"])
            rgba[indices] = self.masks_cmap(self.norm(new_values["values"][indices]))





        # for m in new_values["masks"]:
        #     if new_values["masks"][m] is not None:
        #         indices = np.where(new_values["masks"][m])
        #         # for i in range(len(indices[0])):
        #         #     rgba[indices[0][i], indices[1][i], :] = [0, 0, 0, 1.0]

        #         # print(indices[0])
        #         # print(rgba[indices].shape)
        #         # print(self.masks_cmap(self.norm(new_values["values"][indices])).shape)
        #         rgba[indices] = self.masks_cmap(self.norm(new_values["values"][indices]))
        #         # rgba = np.where(new_values["masks"][m], self.masks_cmap(self.norm(new_values["values"])), rgba)
        #         # self.mask_image[m].set_data(new_values["masks"][m])
        # # print(rgba.shape)

        self.image_colors.set_data(rgba)
        self.image_values.set_data(new_values["values"])
        if new_values["extent"] is not None:
            self.image_colors.set_extent(new_values["extent"])
            self.image_values.set_extent(new_values["extent"])
        # for m in self.mask_image:
        #     if new_values["masks"][m] is not None:
        #         self.mask_image[m].set_data(new_values["masks"][m])
        #     else:
        #         self.mask_image[m].set_visible(False)
        #         self.mask_image[m].set_url("hide")
        #     if new_values["extent"] is not None:
        #         self.mask_image[m].set_extent(new_values["extent"])
        self.draw()

    def toggle_norm(self, norm=None, vmin=None, vmax=None):
        self.norm = LogNorm(
            vmin=vmin, vmax=vmax) if norm == "log" else Normalize(vmin=vmin,
                                                                  vmax=vmax)
        # print(vmin, vmax)
        self.image_values.set_norm(self.norm)
        self.opacify_colorbar()

        # for m in self.mask_image:
        #     self.mask_image[m].set_norm(new_norm)
        # self.draw()

    def rescale_on_zoom(self, *args, **kwargs):
        if self.toolbar is not None:
            self.toolbar.rescale_on_zoom(*args, **kwargs)
