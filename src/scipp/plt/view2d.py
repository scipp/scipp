# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .. import config
from .view import View
from .toolbar import Toolbar2d
from .tools import find_limits, fix_empty_range
from ..utils import name_with_unit
from ..core import broadcast
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.colors import Normalize, LogNorm
import math
import warnings
from functools import reduce


class View2d(View):
    """
    Class for 2 dimensional plots.
    """
    def __init__(self,
                 ax=None,
                 cax=None,
                 figsize=None,
                 aspect=None,
                 cmap=None,
                 masks=None,
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
                         toolbar=Toolbar2d,
                         grid=grid)

        if aspect is None:
            aspect = config['plot']['aspect']

        self._cmap = cmap
        self._cax = cax
        self._mask_cmap = masks["cmap"]
        self._norm_flag = norm
        self._norm_func = None
        self._extend = extend
        self._image = None
        self._cbar = None
        self._data = None

    def _make_limits(self):
        vmin, vmax = fix_empty_range(
            find_limits(self._data.data, scale=self._norm_flag)[self._norm_flag])
        return vmin.value, vmax.value

    def rescale_to_data(self, _):
        """
        Rescale the colorbar limits according to the supplied values.
        """
        vmin, vmax = self._make_limits()
        self._norm_func.vmin = vmin
        self._norm_func.vmax = vmax
        self._image.set_clim(vmin, vmax)
        self.update()

    def toggle_mask(self, *args, **kwargs):
        """
        Show or hide a given mask.
        """
        return

    def update(self, new_values=None, draw=True):
        """
        Update image array with new values.
        """
        if new_values is not None:
            self._data = new_values
        dims = self._data.dims

        if self._image is None:
            self._image = self.ax.pcolormesh(self._data.meta[dims[1]].values,
                                             self._data.meta[dims[0]].values,
                                             self._data.data.values,
                                             shading='auto')
            self._cbar = plt.colorbar(self._image,
                                      ax=self.ax,
                                      cax=self._cax,
                                      extend=self._extend)
            if self._cax is None:
                self._cbar.ax.yaxis.set_label_coords(-1.1, 0.5)
            self._image.set_array(None)
            self._set_norm()
            self.ax.set_xlabel(
                self.xlabel if self.xlabel is not None else name_with_unit(
                    var=self._data.meta[dims[1]]))
            self.ax.set_ylabel(
                self.ylabel if self.ylabel is not None else name_with_unit(
                    var=self._data.meta[dims[0]]))

        flat_values = self._data.values.flatten()
        rgba = self._cmap(self._norm_func(flat_values))
        if len(self._data.masks) > 0:
            # Combine all masks into one
            one_mask = broadcast(reduce(lambda a, b: a | b, self._data.masks.values()),
                                 dims=self._data.dims,
                                 shape=self._data.shape).values.flatten()
            # indices = np.where(new_values["masks"])
            rgba[one_mask] = self._mask_cmap(self._norm_func(flat_values[one_mask]))

        self._image.set_facecolors(rgba)
        if draw:
            self.draw()

    def _set_norm(self):
        vmin, vmax = self._make_limits()
        func = LogNorm if self._norm_flag == "log" else Normalize
        self._norm_func = func(vmin=vmin, vmax=vmax)
        self._image.set_norm(self._norm_func)

    def toggle_norm(self, change=None):
        self._norm_flag = "log" if change["new"] else "linear"
        self._set_norm()
        self.update()

    def transpose(self):
        pass
