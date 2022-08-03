# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from ... import config
from ... import broadcast, DataArray
from .limits import find_limits, fix_empty_range
from .tools import get_cmap
from ...utils import name_with_unit

from functools import reduce
from matplotlib.colors import Normalize, LogNorm
import matplotlib.pyplot as plt
import numpy as np
from typing import Any


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
                 masks_cmap: str = "gray",
                 norm: str = "linear",
                 vmin=None,
                 vmax=None,
                 cbar=True):

        self._ax = ax
        self._data = data
        self._dims = {'x': self._data.dims[1], 'y': self._data.dims[0]}

        self._xlabel = None
        self._ylabel = None
        self._title = None
        self._user_vmin = vmin
        self._user_vmax = vmax
        self._vmin = np.inf
        self._vmax = np.NINF

        self._cmap = get_cmap(cmap)
        self._cax = cax
        self._mask_cmap = get_cmap(masks_cmap)
        self._norm_flag = norm
        self._norm_func = None

        self._mesh = None
        self._cbar = cbar
        self._aspect = aspect if aspect is not None else config['plot']['aspect']

        self._extend = "neither"
        if (vmin is not None) and (vmax is not None):
            self._extend = "both"
        elif vmin is not None:
            self._extend = "min"
        elif vmax is not None:
            self._extend = "max"

        self._make_mesh()

    def _make_mesh(self):
        self._mesh = self._ax.pcolormesh(self._data.meta[self._dims['x']].values,
                                         self._data.meta[self._dims['y']].values,
                                         self._data.data.values,
                                         cmap=self._cmap,
                                         shading='auto')
        if self._cbar:
            self._cbar = plt.colorbar(self._mesh,
                                      ax=self._ax,
                                      cax=self._cax,
                                      extend=self._extend,
                                      label=name_with_unit(var=self._data.data,
                                                           name=""))

            # Add event that toggles the norm of the colorbar when clicked on
            # TODO: change this to a double-click event once this is supported in
            # jupyterlab, see https://github.com/matplotlib/ipympl/pull/446
            self._cbar.ax.set_picker(5)
            self._ax.figure.canvas.mpl_connect('pick_event', self.toggle_norm)

            if self._cax is None:
                self._cbar.ax.yaxis.set_label_coords(-1.1, 0.5)
            # When we transpose, remove the mesh and make a new one with _make_mesh().
            # To ensure this does not add a new colorbar every time we hit transpose,
            # we save and re-use the colorbar axis.
            self._cax = self._cbar.ax
        self._mesh.set_array(None)
        self._set_norm()
        self._set_mesh_colors()

    def _rescale_colormap(self):
        """
        """
        vmin, vmax = fix_empty_range(find_limits(self._data.data,
                                                 scale=self._norm_flag))
        if self._user_vmin is not None:
            assert self._user_vmin.unit == self._data.unit
            self._vmin = self._user_vmin.value
        elif vmin.value < self._vmin:
            self._vmin = vmin.value
        if self._user_vmax is not None:
            assert self._user_vmax.unit == self._data.unit
            self._vmax = self._user_vmax.value
        elif vmax.value > self._vmax:
            self._vmax = vmax.value

        self._norm_func.vmin = self._vmin
        self._norm_func.vmax = self._vmax

    def _set_clim(self):
        self._mesh.set_clim(self._vmin, self._vmax)

    def _set_mesh_colors(self):
        flat_values = self._data.values.flatten()
        rgba = self._cmap(self._norm_func(flat_values))
        if len(self._data.masks) > 0:
            one_mask = broadcast(reduce(lambda a, b: a | b, self._data.masks.values()),
                                 dims=self._data.dims,
                                 shape=self._data.shape).values.flatten()
            rgba[one_mask] = self._mask_cmap(self._norm_func(flat_values[one_mask]))
        self._mesh.set_facecolors(rgba)

    def update(self, new_values: DataArray):
        """
        Update image array with new values.
        """
        self._data = new_values.transpose([self._dims['y'], self._dims['x']])
        self._rescale_colormap()
        self._set_clim()
        self._set_mesh_colors()

    def _set_norm(self):
        func = LogNorm if self._norm_flag == "log" else Normalize
        self._norm_func = func()
        self._rescale_colormap()
        self._mesh.set_norm(self._norm_func)
        self._set_clim()

    def toggle_norm(self, event):
        if event.artist is not self._cbar.ax:
            return
        self._norm_flag = "log" if self._norm_flag == "linear" else "linear"
        self._vmin = np.inf
        self._vmax = np.NINF
        self._set_norm()
        self._set_mesh_colors()
        self._ax.figure.canvas.draw_idle()

    def transpose(self):
        self._dims['x'], self._dims['y'] = self._dims['y'], self._dims['x']
        self._data = self._data.transpose([self._dims['y'], self._dims['x']])
        self._mesh.remove()
        self._make_mesh()

    def get_limits(self, xscale, yscale):
        xmin, xmax = fix_empty_range(
            find_limits(self._data.meta[self._dims['x']], scale=xscale))
        ymin, ymax = fix_empty_range(
            find_limits(self._data.meta[self._dims['y']], scale=yscale))
        return xmin, xmax, ymin, ymax
