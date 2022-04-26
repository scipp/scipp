# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .. import config, DataArray
from .tools import fig_to_pngbytes
from .mesh import Mesh
from .line import Line
from ..utils import name_with_unit

import ipywidgets as ipw
import matplotlib.pyplot as plt
import numpy as np
from typing import Any, Tuple


class Figure:
    def __init__(self,
                 ax: Any = None,
                 figsize: Tuple[float, ...] = None,
                 title: str = "",
                 xlabel: str = None,
                 ylabel: str = None,
                 grid: bool = False,
                 bounding_box: Tuple[float, ...] = None,
                 vmin=None,
                 vmax=None,
                 **kwargs):

        self._fig = None
        self._closed = False
        self._title = title
        self._ax = ax
        self._bounding_box = bounding_box
        self._xlabel = xlabel
        self._ylabel = ylabel
        self._user_vmin = vmin
        self._user_vmax = vmax
        self._kwargs = kwargs

        self._children = {}

        cfg = config['plot']
        if self._ax is None:
            if figsize is None:
                figsize = (cfg['width'] / cfg['dpi'], cfg['height'] / cfg['dpi'])
            self._fig, self._ax = plt.subplots(1, 1, figsize=figsize, dpi=cfg['dpi'])
            if self._bounding_box is None:
                self._bounding_box = cfg['bounding_box']
            self._fig.tight_layout(rect=self._bounding_box)
            if self.is_widget():
                self._fig.canvas.toolbar_visible = False
        else:
            self._fig = self._ax.get_figure()

        self._ax.set_title(self._title)
        if grid:
            self._ax.grid()

        self._legend = False
        self._new_artist = False

    def is_widget(self) -> bool:
        """
        Check whether we are using the Matplotlib widget backend or not.
        "on_widget_constructed" is an attribute specific to `ipywidgets`.
        """
        return hasattr(self._fig.canvas, "on_widget_constructed")

    def _to_widget(self) -> ipw.Widget:
        """
        Convert the Matplotlib figure to a widget. If the ipympl (widget)
        backend is in use, return the custom toolbar and the figure canvas.
        If not, convert the plot to a png image and place inside an ipywidgets
        Image container.
        """
        if self.is_widget() and not self._closed:
            return self._fig.canvas
        else:
            return self._to_image()

    def _to_image(self) -> ipw.Image:
        """
        Convert the Matplotlib figure to a static image.
        """
        width, height = self._fig.get_size_inches()
        dpi = self._fig.get_dpi()
        return ipw.Image(value=fig_to_pngbytes(self._fig),
                         width=width * dpi,
                         height=height * dpi)

    def _autoscale(self):
        global_xmin = np.inf
        global_xmax = np.NINF
        global_ymin = np.inf
        global_ymax = np.NINF
        xscale = self._ax.get_xscale()
        yscale = self._ax.get_yscale()
        for key, child in self._children.items():
            xmin, xmax, ymin, ymax = child.get_limits(xscale=xscale, yscale=yscale)
            if isinstance(child, Line):
                if self._user_vmin is not None:
                    ymin = self._user_vmin
                if self._user_vmax is not None:
                    ymax = self._user_vmax
            if xmin.value < global_xmin:
                global_xmin = xmin.value
            if xmax.value > global_xmax:
                global_xmax = xmax.value
            if ymin.value < global_ymin:
                global_ymin = ymin.value
            if ymax.value > global_ymax:
                global_ymax = ymax.value
        self._ax.set_xlim(global_xmin, global_xmax)
        self._ax.set_ylim(global_ymin, global_ymax)

    def draw(self):
        """
        """
        if self._new_artist:
            self._ax.set_xlabel(self._xlabel)
            self._ax.set_ylabel(self._ylabel)
            if self._legend:
                self._ax.legend()
            self._new_artist = False
        self._draw_canvas()

    def _draw_canvas(self):
        self._fig.canvas.draw_idle()

    def home_view(self, *_):
        self._autoscale()
        self._draw_canvas()

    def pan_view(self, *_):
        self._fig.canvas.toolbar.pan()

    def zoom_view(self, *_):
        self._fig.canvas.toolbar.zoom()

    def save_view(self, *_):
        self._fig.canvas.toolbar.save_figure()

    def toggle_xaxis_scale(self, *_):
        swap_scales = {"linear": "log", "log": "linear"}
        self._ax.set_xscale(swap_scales[self._ax.get_xscale()])
        self._autoscale()
        self._draw_canvas()

    def toggle_yaxis_scale(self, *_):
        swap_scales = {"linear": "log", "log": "linear"}
        self._ax.set_yscale(swap_scales[self._ax.get_yscale()])
        self._autoscale()
        self._draw_canvas()

    def transpose(self, *_):
        for child in self._children.values():
            if isinstance(child, Mesh):
                child.transpose()
        self._autoscale()
        self._draw_canvas()

    def savefig(self, filename: str = None):
        """
        Save plot to file.
        Possible file extensions are `.jpg`, `.png` and `.pdf`.
        The default directory for writing the file is the same as the
        directory where the script or notebook is running.
        """
        self._fig.savefig(filename, bbox_inches="tight")

    def update(self, new_values: DataArray = None, key: str = None, draw: bool = True):
        """
        Update image array with new values.
        """

        if key not in self._children:
            self._new_artist = True
            if new_values.ndim == 1:
                self._children[key] = Line(ax=self._ax,
                                           data=new_values,
                                           number=len(self._children),
                                           **self._kwargs)
                self._legend = True

                if self._xlabel is None:
                    self._xlabel = name_with_unit(var=new_values.meta[new_values.dim])
                if self._ylabel is None:
                    self._ylabel = name_with_unit(var=new_values.data, name="")

            elif new_values.ndim == 2:
                self._children[key] = Mesh(ax=self._ax,
                                           data=new_values,
                                           vmin=self._user_vmin,
                                           vmax=self._user_vmax,
                                           **self._kwargs)
                if self._xlabel is None:
                    self._xlabel = name_with_unit(
                        var=new_values.meta[new_values.dims[1]])
                if self._ylabel is None:
                    self._ylabel = name_with_unit(
                        var=new_values.meta[new_values.dims[0]])

        else:
            self._children[key].update(new_values=new_values)

        if draw:
            self.draw()
