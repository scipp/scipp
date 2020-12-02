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

        self.image = self.make_default_imshow(cmap=cmap,
                                              norm=norm,
                                              aspect=aspect,
                                              picker=5)

        self.cbar = None
        if cbar:
            self.cbar = plt.colorbar(self.image,
                                     ax=self.ax,
                                     cax=self.cax,
                                     extend=extend)
            self.cbar.set_label(unit)
        if self.cax is None:
            self.cbar.ax.yaxis.set_label_coords(-1.1, 0.5)
        self.mask_image = {}
        for m in masks["names"]:
            self.mask_image[m] = self.make_default_imshow(cmap=masks["cmap"],
                                                          norm=norm,
                                                          aspect=aspect)

    def make_default_imshow(self, cmap, norm, aspect=None, picker=None):
        """
        Make a base `imshow` object whose contents and extents will be later
        updated to display a 2d data array.
        This is used for both a data image and for masks images.
        """
        return self.ax.imshow([[1.0, 1.0], [1.0, 1.0]],
                              norm=norm,
                              extent=[1, 2, 1, 2],
                              origin="lower",
                              aspect=aspect,
                              interpolation="nearest",
                              cmap=cmap,
                              picker=picker)

    def rescale_to_data(self, vmin, vmax):
        """
        Rescale the colorbar limits according to the supplied values.
        """
        self.image.set_clim([vmin, vmax])
        for m, im in self.mask_image.items():
            im.set_clim([vmin, vmax])
        self.draw()

    def toggle_mask(self, mask_name, visible):
        """
        Show or hide a given mask.
        """
        im = self.mask_image[mask_name]
        if im.get_url() != "hide":
            im.set_visible(visible)
        self.draw()

    def reset_home_button(self, axparams):
        """
        Some annoying house-keeping when using X/Y buttons: we need to update
        the deeply embedded limits set by the Home button in the matplotlib
        toolbar. The home button actually brings the first element in the
        navigation stack to the top, so we need to modify the first element
        in the navigation stack in-place.
        """
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
                    alist[0] = (*axparams["x"]["lims"], *axparams["y"]["lims"])
                    # Insert the new tuple
                    self.fig.canvas.toolbar._nav_stack._elements[0][
                        key] = tuple(alist)

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
            self.image.set_extent(extent_array)
            for m, im in self.mask_image.items():
                im.set_extent(extent_array)
            self.ax.set_xlim(axparams["x"]["lims"])
            self.ax.set_ylim(axparams["y"]["lims"])

        self.reset_home_button(axparams)

    def update_data(self, new_values, info=None):
        """
        Update image array with new values.
        """
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
        self.draw()

    def toggle_norm(self, norm=None, vmin=None, vmax=None):
        new_norm = LogNorm(
            vmin=vmin, vmax=vmax) if norm == "log" else Normalize(vmin=vmin,
                                                                  vmax=vmax)
        self.image.set_norm(new_norm)
        for m in self.mask_image:
            self.mask_image[m].set_norm(new_norm)

    def rescale_on_zoom(self, *args, **kwargs):
        self.toolbar.rescale_on_zoom(*args, **kwargs)
