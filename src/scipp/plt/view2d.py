# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .. import config
from .view import PlotView
from .toolbar import PlotToolbar2d
from .tools import find_limits, fix_empty_range
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.colors import Normalize, LogNorm
import math
import warnings


class PlotView2d(PlotView):
    """
    Class for 2 dimensional plots, based on Matplotlib's `imshow`.
    """
    def __init__(
            self,
            ax=None,
            cax=None,
            figsize=None,
            aspect=None,
            cmap=None,
            mask_cmap=None,
            norm=None,
            name=None,
            # cbar=None,
            resolution=None,
            extend=None,
            title=None,
            xlabel=None,
            ylabel=None,
            grid=False):

        super().__init__(ax=ax,
                         figsize=figsize,
                         title=name if title is None else title,
                         ndim=2,
                         xlabel=xlabel,
                         ylabel=ylabel,
                         toolbar=PlotToolbar2d,
                         grid=grid)

        if aspect is None:
            aspect = config['plot']['aspect']

        self.cmap = cmap
        self.cax = cax

        # print("cmap2", cmap)
        self._mask_cmap = mask_cmap
        self.norm_flag = norm
        print("NORM", norm)
        self.norm_func = None
        self.extend = extend
        # print("norm", self.norm)

        # ones = np.ones([2, 2])
        # image_params = {
        #     "extent": [1, 2, 1, 2],
        #     "origin": "lower",
        #     "aspect": aspect,
        #     "interpolation": "nearest"
        # }

        # self.image_colors = self.ax.imshow(self.cmap(self.norm(ones)),
        #                                    zorder=1,
        #                                    **image_params)

        # self.image_values = self.ax.imshow(ones,
        #                                    norm=self.norm,
        #                                    cmap=self.cmap,
        #                                    picker=5,
        #                                    zorder=2,
        #                                    alpha=0.0,
        #                                    **image_params)
        self.image = None

        # self.image_values = None

        self.cbar = None

        self.data = None

        # self.mask_image = {}

    # def _disable_colorbar_offset(self):
    #     if not isinstance(self.norm, LogNorm):
    #         self.cbar.formatter.set_useOffset(False)

    def _make_limits(self):
        vmin, vmax = fix_empty_range(
            find_limits(self.data.data, scale=self.norm_flag)[self.norm_flag])
        return vmin.value, vmax.value

    def rescale_to_data(self, _):
        """
        Rescale the colorbar limits according to the supplied values.
        """
        vmin, vmax = self._make_limits()
        self.norm_func.vmin = vmin
        self.norm_func.vmax = vmax
        self.image.set_clim(vmin, vmax)
        # self.opacify_colorbar()
        # self.draw()
        self.update_data()

    # def opacify_colorbar(self):
    #     self.cbar.set_alpha(1.0)
    #     self.cbar.draw_all()

    def toggle_mask(self, *args, **kwargs):
        """
        Show or hide a given mask.
        """
        return

    # def update_axes(self, scale, unit):
    #     """
    #     Update axes labels, scales, tick locations and labels, as well as axes
    #     limits.
    #     """
    #     self.cbar.set_label(unit)
    #     self.ax.set_xlabel(
    #         self._formatters['x']["label"] if self.xlabel is None else self.xlabel)
    #     self.ax.set_ylabel(
    #         self._formatters['y']["label"] if self.ylabel is None else self.ylabel)
    #     self.ax.set_xscale(scale['x'])
    #     self.ax.set_yscale(scale['y'])

    #     self.ax.xaxis.set_major_formatter(self.axformatter['x'][scale['x']])
    #     self.ax.xaxis.set_major_locator(self.axlocator['x'][scale['x']])
    #     self.ax.yaxis.set_major_formatter(self.axformatter['y'][scale['y']])
    #     self.ax.yaxis.set_major_locator(self.axlocator['y'][scale['y']])

    #     self._limits_set = False

    # def _make_data(self, new_values, mask_info=None):
    #     dims = new_values.dims
    #     # for dim in dims:
    #     #     xmin = new_values.coords[dim].values[0]
    #     #     xmax = new_values.coords[dim].values[-1]
    #     #     if dim not in self.global_lims:
    #     #         self.global_lims[dim] = [xmin, xmax]
    #     #     self.current_lims[dim] = [xmin, xmax]
    #     values = new_values.data.values
    #     slice_values = {
    #         "values": values,
    #         # "extent":
    #         # np.array([self.current_lims[dims[1]],
    #         #           self.current_lims[dims[0]]]).flatten()
    #     }
    #     slice_values["x"] = new_values.meta[dims[1]].values
    #     slice_values["y"] = new_values.meta[dims[0]].values
    #     # mask_info = next(iter(mask_info.values()))
    #     # if len(mask_info) > 0:
    #     #     # Use automatic broadcasting in Scipp variables
    #     #     msk = zeros(sizes=new_values.sizes, dtype='int32', unit=None)
    #     #     for m, val in mask_info.items():
    #     #         if val:
    #     #             msk += new_values.masks[m].astype(msk.dtype)
    #     #     slice_values["masks"] = msk.values
    #     return slice_values

    def update_data(self, new_values=None):
        """
        Update image array with new values.
        """
        # self.data = self._make_data(new_values)
        if new_values is not None:
            self.data = new_values
        # print(self.cmap)
        # print(self.norm)
        # print(self.cmap(np.arange(10.)))
        # print(self.norm(np.arange(10.)))
        # print(new_values["values"])
        dims = self.data.dims

        # if "masks" in new_values:
        #     indices = np.where(new_values["masks"].flatten())
        #     rgba[indices] = self._mask_cmap(self.norm(new_values["values"][indices]))

        if self.image is None:
            self.image = self.ax.pcolormesh(self.data.meta[dims[1]].values,
                                            self.data.meta[dims[0]].values,
                                            self.data.data.values,
                                            shading='auto')
            self.cbar = plt.colorbar(self.image,
                                     ax=self.ax,
                                     cax=self.cax,
                                     extend=self.extend)
            # self._disable_colorbar_offset()
            if self.cax is None:
                self.cbar.ax.yaxis.set_label_coords(-1.1, 0.5)
            self.image.set_array(None)
            self._set_norm()

        rgba = self.cmap(self.norm_func(self.data.data.values.flatten()))
        self.image.set_facecolors(rgba)
        # self.image_values.set_data(new_values["values"])
        # self.image_colors.set_extent(new_values["extent"])
        # self.image_values.set_extent(new_values["extent"])
        # if not self._limits_set:
        #     self._limits_set = True
        #     with warnings.catch_warnings():
        #         warnings.filterwarnings("ignore", category=UserWarning)
        #         self.ax.set_xlim(new_values["extent"][:2])
        #         self.ax.set_ylim(new_values["extent"][2:])
        self.draw()

    def _set_norm(self):
        vmin, vmax = self._make_limits()
        func = LogNorm if self.norm_flag == "log" else Normalize
        self.norm_func = func(vmin=vmin, vmax=vmax)
        self.image.set_norm(self.norm_func)

    def toggle_norm(self, change=None):
        self.norm_flag = "log" if change["new"] else "linear"
        # # lims = find_limits(self.data.data, scale=self.norm)
        # vmin, vmax = self._make_limits(norm=self.norm_flag)
        # func = LogNorm if self.norm_flag == "log" else Normalize
        # self.norm_func = func(vmin=vmin, vmax=vmax)
        # # self.image.set_norm(self.norm_func)
        # # self._disable_colorbar_offset()
        # # self.opacify_colorbar()
        self._set_norm()
        self.update_data()

    def transpose(self):
        pass

    # def rescale_on_zoom(self, *args, **kwargs):
    #     if self.toolbar is not None:
    #         return self.toolbar.rescale_on_zoom(*args, **kwargs)
    #     else:
    #         return False
