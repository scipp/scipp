# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .. import config
from .view import PlotView
from .toolbar import PlotToolbar2d
from .tools import find_limits, fix_empty_range
from ..utils import name_with_unit
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.colors import Normalize, LogNorm
import math
import warnings


class PlotView2d(PlotView):
    """
    Class for 2 dimensional plots.
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
                 resolution=None,
                 extend=None,
                 title=None,
                 xlabel=None,
                 ylabel=None,
                 grid=False):

        super().__init__(ax=ax,
                         figsize=figsize,
                         title=name if title is None else title,
                         xlabel=xlabel,
                         ylabel=ylabel,
                         toolbar=PlotToolbar2d,
                         grid=grid)

        if aspect is None:
            aspect = config['plot']['aspect']

        self.cmap = cmap
        self.cax = cax

        self._mask_cmap = mask_cmap
        self.norm_flag = norm
        self.norm_func = None
        self.extend = extend
        self.image = None
        self.cbar = None
        self._data = None

    def _make_limits(self):
        vmin, vmax = fix_empty_range(
            find_limits(self._data.data, scale=self.norm_flag)[self.norm_flag])
        return vmin.value, vmax.value

    def rescale_to_data(self, _):
        """
        Rescale the colorbar limits according to the supplied values.
        """
        vmin, vmax = self._make_limits()
        self.norm_func.vmin = vmin
        self.norm_func.vmax = vmax
        self.image.set_clim(vmin, vmax)
        self.update()

    def toggle_mask(self, *args, **kwargs):
        """
        Show or hide a given mask.
        """
        return

    def update(self, new_values=None):
        """
        Update image array with new values.
        """
        if new_values is not None:
            self._data = new_values
        dims = self._data.dims

        if self.image is None:
            self.image = self.ax.pcolormesh(self._data.meta[dims[1]].values,
                                            self._data.meta[dims[0]].values,
                                            self._data.data.values,
                                            shading='auto')
            self.cbar = plt.colorbar(self.image,
                                     ax=self.ax,
                                     cax=self.cax,
                                     extend=self.extend)
            if self.cax is None:
                self.cbar.ax.yaxis.set_label_coords(-1.1, 0.5)
            self.image.set_array(None)
            self._set_norm()
            self.ax.set_xlabel(
                self.xlabel if self.xlabel is not None else name_with_unit(
                    var=self._data.meta[dims[1]]))
            self.ax.set_ylabel(
                self.ylabel if self.ylabel is not None else name_with_unit(
                    var=self._data.meta[dims[0]]))

        rgba = self.cmap(self.norm_func(self._data.data.values.flatten()))
        self.image.set_facecolors(rgba)
        self.draw()

    def _set_norm(self):
        vmin, vmax = self._make_limits()
        func = LogNorm if self.norm_flag == "log" else Normalize
        self.norm_func = func(vmin=vmin, vmax=vmax)
        self.image.set_norm(self.norm_func)

    def toggle_norm(self, change=None):
        self.norm_flag = "log" if change["new"] else "linear"
        self._set_norm()
        self.update()

    def transpose(self):
        pass
