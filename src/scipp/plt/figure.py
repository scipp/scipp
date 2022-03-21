# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .. import config
from .tools import fig_to_pngbytes
from .toolbar import Toolbar

import ipywidgets as ipw
import matplotlib.pyplot as plt


class Figure:
    def __init__(self,
                 ax: Any = None,
                 figsize: Tuple[float, ...] = None,
                 title: str = None,
                 xlabel: str = None,
                 ylabel: str = None,
                 grid: bool = False,
                 bounding_box: Tuple[float, ...] = None):
        self._fig = None
        self._closed = False
        self._ax = ax
        # self._own_axes = True
        self._bounding_box = bounding_box
        cfg = config['plot']
        if self._ax is None:
            if figsize is None:
                figsize = (cfg['width'] / cfg['dpi'], cfg['height'] / cfg['dpi'])
            self._fig, self._ax = plt.subplots(1, 1, figsize=figsize, dpi=cfg['dpi'])
            if self._bounding_box is None:
                self._bounding_box = cfg['bounding_box']
            self._fig.tight_layout(rect=self._bounding_box)
            if self.is_widget():
                # self.toolbar = toolbar(external_controls=self.fig.canvas.toolbar)
                self._fig.canvas.toolbar_visible = False
        else:
            # self.own_axes = False
            self._fig = self._ax.get_figure()

        self._ax.set_title(title)
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
        self.fig.canvas.draw_idle()
