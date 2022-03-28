# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .. import config, Variable
from .tools import fig_to_pngbytes
from .toolbar import Toolbar

import ipywidgets as ipw
import matplotlib.pyplot as plt
import numpy as np
from typing import Any, Tuple


class Figure:
    def __init__(self,
                 ax: Any = None,
                 figsize: Tuple[float, ...] = None,
                 title: str = None,
                 xlabel: str = None,
                 ylabel: str = None,
                 vmin: Variable = None,
                 vmax: Variable = None,
                 grid: bool = False,
                 bounding_box: Tuple[float, ...] = None):
        self._fig = None
        self._closed = False
        self._title = title
        self._ax = ax
        self._bounding_box = bounding_box
        self._xlabel = xlabel
        self._ylabel = ylabel
        self._user_vmin = vmin
        self._user_vmax = vmax
        self._vmin = np.inf
        self._vmax = np.NINF

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

    def draw(self):
        """
        Manually update the figure.
        We control update manually since we have better control on how many
        draws are performed on the canvas. Matplotlib's automatic drawing
        (which we have disabled by using `plt.ioff()`) can degrade performance
        significantly.
        """
        self._fig.canvas.draw_idle()

    def home_view(self, *_):
        self._fig.canvas.toolbar.home()

    def pan_view(self, *_):
        # if change["new"]:
        # In case the zoom button is selected, we need to de-select it
        # if self.members["zoom_view"].value:
        #     self.members["zoom_view"].value = False
        self._fig.canvas.toolbar.pan()

    def zoom_view(self, *_):
        # if change["new"]:
        # In case the pan button is selected, we need to de-select it
        # if self.members["pan_view"].value:
        #     self.members["pan_view"].value = False
        self._fig.canvas.toolbar.zoom()

    def save_view(self, *_):
        self._fig.canvas.toolbar.save_figure()

    def toggle_xaxis_scale(self, *_):
        swap_scales = {"linear": "log", "log": "linear"}
        self._ax.set_xscale(swap_scales[self._ax.get_xscale()])
        self.draw()

    def toggle_yaxis_scale(self, *_):
        swap_scales = {"linear": "log", "log": "linear"}
        self._ax.set_yscale(swap_scales[self._ax.get_yscale()])
        self.draw()

    def render(self):
        return

    def savefig(self, filename: str = None):
        """
        Save plot to file.
        Possible file extensions are `.jpg`, `.png` and `.pdf`.
        The default directory for writing the file is the same as the
        directory where the script or notebook is running.
        """
        self._fig.savefig(filename, bbox_inches="tight")
