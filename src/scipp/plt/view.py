# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .. import config
from .tools import fig_to_pngbytes
from .toolbar import Toolbar

import ipywidgets as ipw
import matplotlib.pyplot as plt
from typing import Any, Tuple


class SideBar:
    def __init__(self, children=None):
        self._children = children if children is not None else []

    def _ipython_display_(self):
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        return ipw.VBox([child._to_widget() for child in self._children])


class View:
    """
    Base class for 1d and 2d figures, that holds matplotlib axes.
    """
    def __init__(self, figure, **kwargs):
        # ax: Any = None,
        # figsize: Tuple[float, ...] = None,
        # title: str = None,
        # bounding_box: Tuple[float, ...] = None,
        # xlabel: str = None,
        # ylabel: str = None,
        # toolbar: Toolbar = None,
        # grid: bool = False):

        self.figure = figure(**kwargs)

        self.toolbar = toolbar(external_controls=self.fig.canvas.toolbar)

        # self.fig = None
        # self.closed = False
        # self.ax = ax
        # self.own_axes = True
        # self.toolbar = None
        # self.bounding_box = bounding_box
        # cfg = config['plot']
        # if self.ax is None:
        #     if figsize is None:
        #         figsize = (cfg['width'] / cfg['dpi'], cfg['height'] / cfg['dpi'])
        #     self.fig, self.ax = plt.subplots(1, 1, figsize=figsize, dpi=cfg['dpi'])
        #     if self.bounding_box is None:
        #         self.bounding_box = cfg['bounding_box']
        #     self.fig.tight_layout(rect=self.bounding_box)
        #     if self.is_widget():
        #         self.toolbar = toolbar(external_controls=self.fig.canvas.toolbar)
        #         self.fig.canvas.toolbar_visible = False
        # else:
        #     self.own_axes = False
        #     self.fig = self.ax.get_figure()

        # self.ax.set_title(title)
        # if grid:
        #     self.ax.grid()

        self.left_bar = SideBar([self.toolbar])
        self.right_bar = SideBar()
        self.bottom_bar = SideBar()
        self.top_bar = SideBar()

        # self.axformatter = {}
        # self.axlocator = {}
        # self.xlabel = xlabel
        # self.ylabel = ylabel
        # self.draw_no_delay = False
        # self.event_connections = {}

        self.toolbar.connect(view=self)

    # def is_widget(self) -> bool:
    #     """
    #     Check whether we are using the Matplotlib widget backend or not.
    #     "on_widget_constructed" is an attribute specific to `ipywidgets`.
    #     """
    #     return hasattr(self.fig.canvas, "on_widget_constructed")

    def home_view(self, button: ipw.Widget):
        self.external_controls.home()

    def pan_view(self, change: dict):
        if change["new"]:
            # In case the zoom button is selected, we need to de-select it
            if self.members["zoom_view"].value:
                self.members["zoom_view"].value = False
            self.external_controls.pan()

    def zoom_view(self, change: dict):
        if change["new"]:
            # In case the pan button is selected, we need to de-select it
            if self.members["pan_view"].value:
                self.members["pan_view"].value = False
            self.external_controls.zoom()

    def save_view(self, button: ipw.Widget):
        self.external_controls.save_figure()

    def savefig(self, filename: str = None):
        """
        Save plot to file.
        Possible file extensions are `.jpg`, `.png` and `.pdf`.
        The default directory for writing the file is the same as the
        directory where the script or notebook is running.
        """
        self.fig.savefig(filename, bbox_inches="tight")

    def _ipython_display_(self):
        """
        IPython display representation for Jupyter notebooks.
        """
        return self._to_widget()._ipython_display_()

    def _to_widget(self) -> ipw.Widget:
        """
        Convert the Matplotlib figure to a widget. If the ipympl (widget)
        backend is in use, return the custom toolbar and the figure canvas.
        If not, convert the plot to a png image and place inside an ipywidgets
        Image container.
        """
        return ipw.VBox([
            self.top_bar._to_widget(),
            ipw.HBox([
                self.left_bar._to_widget(),
                self.figure._to_widget(),
                self.right_bar._to_widget()
            ]),
            self.bottom_bar._to_widget()
        ])

    def close(self):
        """
        Set the closed flag to True to output static images.
        """
        self.closed = True

    def show(self):
        """
        Show the matplotlib figure.
        """
        self.fig.show()

    def toggle_xaxis_scale(self, change: dict):
        self.ax.set_xscale("log" if change['new'] else "linear")
        self.draw()

    def toggle_yaxis_scale(self, change: dict):
        self.ax.set_yscale("log" if change['new'] else "linear")
        self.draw()

    # def draw(self):
    #     """
    #     Manually update the figure.
    #     We control update manually since we have better control on how many
    #     draws are performed on the canvas. Matplotlib's automatic drawing
    #     (which we have disabled by using `plt.ioff()`) can degrade performance
    #     significantly.
    #     """
    #     self.fig.canvas.draw_idle()

    def render(self):
        return
