# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .. import config
from .. import broadcast, DataArray
from .tools import find_limits, fix_empty_range
from ..utils import name_with_unit
# from .figure import Figure

from functools import reduce
from matplotlib.colors import Normalize, LogNorm
import matplotlib.pyplot as plt
import numpy as np
from typing import Any, Tuple


class Mesh:
    """
    Class for 2 dimensional plots.
    """
    def __init__(self,
                 ax,
                 data,
                 cax: Any = None,
                 aspect: str = None,
                 cmap: str = None,
                 masks: dict = None,
                 norm: str = None,
                 extend: bool = None):

        # super().__init__(**kwargs)
        self._ax = ax
        self._data = data

        self._xlabel = None
        self._ylabel = None
        self._title = None
        self._user_vmin = None
        self._user_vmax = None
        self._vmin = np.inf
        self._vmax = np.NINF

        self._cmap = cmap
        self._cax = cax
        self._mask_cmap = 'gray'
        self._norm_flag = 'linear'
        self._norm_func = None
        self._extend = extend
        self._mesh = None
        self._cbar = None
        # self._data = None
        self._aspect = aspect if aspect is not None else config['plot']['aspect']

        self._make_mesh()

    def _make_mesh(self):
        dims = self._data.dims
        self._mesh = self._ax.pcolormesh(self._data.meta[dims[1]].values,
                                         self._data.meta[dims[0]].values,
                                         self._data.data.values,
                                         shading='auto')
        self._cbar = plt.colorbar(self._mesh,
                                  ax=self._ax,
                                  cax=self._cax,
                                  extend=self._extend)
        if self._cax is None:
            self._cbar.ax.yaxis.set_label_coords(-1.1, 0.5)
        self._mesh.set_array(None)
        self._set_norm()
        self._ax.set_xlabel(
            self._xlabel if self._xlabel is not None else name_with_unit(
                var=self._data.meta[dims[1]]))
        self._ax.set_ylabel(
            self._ylabel if self._ylabel is not None else name_with_unit(
                var=self._data.meta[dims[0]]))
        if self._title is None:
            self._ax.set_title(self._data.name)

        self._set_mesh_colors()

    def _make_limits(self) -> Tuple[float, ...]:
        vmin, vmax = fix_empty_range(
            find_limits(self._data.data, scale=self._norm_flag)[self._norm_flag])
        return vmin.value, vmax.value

    def _rescale_to_data(self):
        """
        Rescale the colorbar limits according to the supplied values.
        """
        vmin, vmax = self._make_limits()
        if self._user_vmin is not None:
            assert self._user_vmin.unit == self._data.unit
            self._vmin = self._user_vmin.value
        elif vmin < self._vmin:
            self._vmin = vmin
        if self._user_vmax is not None:
            assert self._user_vmax.unit == self._data.unit
            self._vmax = self._user_vmax.value
        elif vmax > self._vmax:
            self._vmax = vmax

        self._norm_func.vmin = self._vmin
        self._norm_func.vmax = self._vmax
        self._mesh.set_clim(self._vmin, self._vmax)

    def _set_mesh_colors(self):
        # if self._mesh is None:
        #     self._make_mesh()
        self._rescale_to_data()

        flat_values = self._data.values.flatten()
        rgba = self._cmap(self._norm_func(flat_values))
        if len(self._data.masks) > 0:
            one_mask = broadcast(reduce(lambda a, b: a | b, self._data.masks.values()),
                                 dims=self._data.dims,
                                 shape=self._data.shape).values.flatten()
            rgba[one_mask] = self._mask_cmap(self._norm_func(flat_values[one_mask]))

        self._mesh.set_facecolors(rgba)
        # if draw:
        #     self.draw()

    def update(self, new_values: DataArray):
        """
        Update image array with new values.
        """
        # if new_values is not None:
        self._data = new_values
        self._set_mesh_colors()

    def _set_norm(self):
        vmin, vmax = self._make_limits()
        func = LogNorm if self._norm_flag == "log" else Normalize
        self._norm_func = func(vmin=vmin, vmax=vmax)
        self._mesh.set_norm(self._norm_func)

    def toggle_norm(self, change: dict = None):
        self._norm_flag = "log" if change["new"] else "linear"
        self._set_norm()
        self.update()

    def transpose(self):
        pass
