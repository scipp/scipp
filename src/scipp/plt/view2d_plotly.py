# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .. import config
from .view import PlotView
from .toolbar import PlotToolbar2d
from .tools import find_limits, fix_empty_range
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.colors import Normalize, LogNorm
import math
import warnings
import plotly.graph_objects as go


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

        self.fig = go.FigureWidget(layout={
            "autosize": False,
            "width": 900,
            "height": 700
        })

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

    def _ipython_display_(self):
        """
        IPython display representation for Jupyter notebooks.
        """
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        """
        Convert the Matplotlib figure to a widget. If the ipympl (widget)
        backend is in use, return the custom toolbar and the figure canvas.
        If not, convert the plot to a png image and place inside an ipywidgets
        Image container.
        """
        return self.fig

    def _make_limits(self):
        vmin, vmax = fix_empty_range(
            find_limits(self._data.data, scale=self.norm_flag)[self.norm_flag])
        return vmin.value, vmax.value

    def rescale_to_data(self, _):
        """
        Rescale the colorbar limits according to the supplied values.
        """
        return
        vmin, vmax = self._make_limits()
        self.norm_func.vmin = vmin
        self.norm_func.vmax = vmax
        self.image.set_clim(vmin, vmax)
        self.update_data()

    def toggle_mask(self, *args, **kwargs):
        """
        Show or hide a given mask.
        """
        return

    def update_data(self, new_values=None):
        """
        Update image array with new values.
        """
        if new_values is not None:
            self._data = new_values
        dims = self._data.dims

        if self.image is None:
            self.image = go.Heatmap(x=self._data.meta[dims[1]].values,
                                    y=self._data.meta[dims[0]].values,
                                    z=self._data.data.values)

            # self.image = self.ax.pcolormesh(self._data.meta[dims[1]].values,
            #                                 self._data.meta[dims[0]].values,
            #                                 self._data.data.values,
            #                                 shading='auto')
            # self.cbar = plt.colorbar(self.image,
            #                          ax=self.ax,
            #                          cax=self.cax,
            #                          extend=self.extend)
            # if self.cax is None:
            #     self.cbar.ax.yaxis.set_label_coords(-1.1, 0.5)
            # self.image.set_array(None)
            # self._set_norm()
            self.fig.add_trace(self.image)
        else:
            self.fig.data[0].z = self._data.data.values

        # rgba = self.cmap(self.norm_func(self._data.data.values.flatten()))
        # self.image.set_facecolors(rgba)
        # self.draw()

    def _set_norm(self):
        vmin, vmax = self._make_limits()
        func = LogNorm if self.norm_flag == "log" else Normalize
        self.norm_func = func(vmin=vmin, vmax=vmax)
        self.image.set_norm(self.norm_func)

    def toggle_norm(self, change=None):
        self.norm_flag = "log" if change["new"] else "linear"
        self._set_norm()
        self.update_data()

    def transpose(self):
        pass
