# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .. import config
from .. import broadcast, DataArray
from .toolbar import Toolbar2d
from .tools import find_limits, fix_empty_range
from ..utils import name_with_unit
from .figure import Figure

from functools import reduce
from matplotlib.colors import Normalize, LogNorm
import matplotlib.pyplot as plt
from typing import Any, Tuple


class Figure2d(Figure):
    """
    Class for 2 dimensional plots.
    """
    def __init__(
        self,
        # ax: Any = None,
        cax: Any = None,
        # figsize: Tuple[float, ...] = None,
        aspect: str = None,
        cmap: str = None,
        masks: dict = None,
        norm: str = None,
        extend: bool = None,
        # title: str = None,
        # xlabel: str = None,
        # ylabel: str = None,
        # grid: bool = False
        **kwargs):

        super().__init__(**kwargs)

        # if aspect is None:
        #     aspect = config['plot']['aspect']

        self._cmap = cmap
        self._cax = cax
        self._mask_cmap = masks["cmap"]
        self._norm_flag = norm
        self._norm_func = None
        self._extend = extend
        self._image = None
        self._cbar = None
        self._data = None
        self._title = title
        self._aspect = aspect if aspect is not None else config['plot']['aspect']

    def _make_limits(self) -> Tuple[float, ...]:
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

    def _make_image(self):
        dims = self._data.dims
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
        self.ax.set_xlabel(self.xlabel if self.xlabel is not None else name_with_unit(
            var=self._data.meta[dims[1]]))
        self.ax.set_ylabel(self.ylabel if self.ylabel is not None else name_with_unit(
            var=self._data.meta[dims[0]]))
        if self._title is None:
            self.ax.set_title(self._data.name)

    def update(self, new_values: DataArray = None, key: str = None, draw: bool = True):
        """
        Update image array with new values.
        """
        if new_values is not None:
            self._data = new_values

        if self._image is None:
            self._make_image()

        flat_values = self._data.values.flatten()
        rgba = self._cmap(self._norm_func(flat_values))
        if len(self._data.masks) > 0:
            one_mask = broadcast(reduce(lambda a, b: a | b, self._data.masks.values()),
                                 dims=self._data.dims,
                                 shape=self._data.shape).values.flatten()
            rgba[one_mask] = self._mask_cmap(self._norm_func(flat_values[one_mask]))

        self._image.set_facecolors(rgba)
        if draw:
            self.draw()

    def _set_norm(self):
        vmin, vmax = self._make_limits()
        func = LogNorm if self._norm_flag == "log" else Normalize
        self._norm_func = func(vmin=vmin, vmax=vmax)
        self._image.set_norm(self._norm_func)

    def toggle_norm(self, change: dict = None):
        self._norm_flag = "log" if change["new"] else "linear"
        self._set_norm()
        self.update()

    def transpose(self):
        pass
