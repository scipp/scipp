# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .. import config
from .figure import PlotFigure
from .toolbar import PlotToolbar2d
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.colors import Normalize, LogNorm
import math
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
                 mask_cmap=None,
                 norm=None,
                 name=None,
                 cbar=None,
                 resolution=None,
                 extend=None,
                 title=None,
                 xlabel=None,
                 ylabel=None,
                 grid=False):

        super().__init__(ax=ax,
                         cax=cax,
                         figsize=figsize,
                         title=name if title is None else title,
                         ndim=2,
                         xlabel=xlabel,
                         ylabel=ylabel,
                         toolbar=PlotToolbar2d,
                         grid=grid)

        if aspect is None:
            aspect = config.plot.aspect

        self.cmap = cmap
        self._mask_cmap = mask_cmap
        self.norm = norm

        ones = np.ones([2, 2])
        image_params = {
            "extent": [1, 2, 1, 2],
            "origin": "lower",
            "aspect": aspect,
            "interpolation": "nearest"
        }

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

        self.cbar = None
        if cbar:
            self.cbar = plt.colorbar(self.image_values,
                                     ax=self.ax,
                                     cax=self.cax,
                                     extend=extend)
            self._disable_colorbar_offset()
        if self.cax is None:
            self.cbar.ax.yaxis.set_label_coords(-1.1, 0.5)
        self.mask_image = {}

    def _disable_colorbar_offset(self):
        if not isinstance(self.norm, LogNorm):
            self.cbar.formatter.set_useOffset(False)

    def _make_limits(self, vmin, vmax):
        if math.isclose(vmin, vmax):
            offset = 0.001 * max(abs(vmin), abs(vmax))
            vmin -= offset
            vmax += offset
        return vmin, vmax

    def rescale_to_data(self, vmin, vmax):
        """
        Rescale the colorbar limits according to the supplied values.
        """
        vmin, vmax = self._make_limits(vmin, vmax)
        self.norm.vmin = vmin
        self.norm.vmax = vmax
        self.image_values.set_clim(vmin, vmax)
        self.opacify_colorbar()
        self.draw()

    def opacify_colorbar(self):
        self.cbar.set_alpha(1.0)
        self.cbar.draw_all()

    def toggle_mask(self, *args, **kwargs):
        """
        Show or hide a given mask.
        """
        return

    def update_axes(self, scale, unit):
        """
        Update axes labels, scales, tick locations and labels, as well as axes
        limits.
        """
        self.cbar.set_label(unit)
        self.ax.set_xlabel(
            self._formatters['x']["label"] if self.xlabel is None else self.xlabel)
        self.ax.set_ylabel(
            self._formatters['y']["label"] if self.ylabel is None else self.ylabel)
        self.ax.set_xscale(scale['x'])
        self.ax.set_yscale(scale['y'])

        self.ax.xaxis.set_major_formatter(self.axformatter['x'][scale['x']])
        self.ax.xaxis.set_major_locator(self.axlocator['x'][scale['x']])
        self.ax.yaxis.set_major_formatter(self.axformatter['y'][scale['y']])
        self.ax.yaxis.set_major_locator(self.axlocator['y'][scale['y']])

        self._limits_set = False

    def update_data(self, new_values):
        """
        Update image array with new values.
        """
        rgba = self.cmap(self.norm(new_values["values"]))
        if "masks" in new_values:
            indices = np.where(new_values["masks"])
            rgba[indices] = self._mask_cmap(self.norm(new_values["values"][indices]))

        self.image_colors.set_data(rgba)
        self.image_values.set_data(new_values["values"])
        self.image_colors.set_extent(new_values["extent"])
        self.image_values.set_extent(new_values["extent"])
        if not self._limits_set:
            self._limits_set = True
            with warnings.catch_warnings():
                warnings.filterwarnings("ignore", category=UserWarning)
                self.ax.set_xlim(new_values["extent"][:2])
                self.ax.set_ylim(new_values["extent"][2:])
        self.draw()

    def toggle_norm(self, norm=None, vmin=None, vmax=None):
        vmin, vmax = self._make_limits(vmin, vmax)
        self.norm = LogNorm(vmin=vmin, vmax=vmax) if norm == "log" else Normalize(
            vmin=vmin, vmax=vmax)
        self.image_values.set_norm(self.norm)
        self._disable_colorbar_offset()
        self.opacify_colorbar()

    def rescale_on_zoom(self, *args, **kwargs):
        if self.toolbar is not None:
            return self.toolbar.rescale_on_zoom(*args, **kwargs)
        else:
            return False
