# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .figure import Figure
from .toolbar import Toolbar

import ipywidgets as ipw


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
    def __init__(self, **kwargs):

        self.figure = Figure(**kwargs)
        self.toolbar = Toolbar()

        self.toolbar.add_button(name="home_view",
                                callback=self.figure.home_view,
                                icon="home",
                                tooltip="Autoscale view")
        self.toolbar.add_togglebutton(name="pan_view",
                                      callback=self.figure.pan_view,
                                      icon="arrows",
                                      tooltip="Pan")
        self.toolbar.add_togglebutton(name="zoom_view",
                                      callback=self.figure.zoom_view,
                                      icon="search-plus",
                                      tooltip="Zoom")
        self.toolbar.add_togglebutton(name='toggle_xaxis_scale',
                                      callback=self.figure.toggle_xaxis_scale,
                                      description="logx")
        self.toolbar.add_togglebutton(name="toggle_yaxis_scale",
                                      callback=self.figure.toggle_yaxis_scale,
                                      description="logy")
        self.toolbar.add_button(name="save_view",
                                callback=self.figure.save_view,
                                icon="save",
                                tooltip="Save")

        self.left_bar = SideBar([self.toolbar])
        self.right_bar = SideBar()
        self.bottom_bar = SideBar()
        self.top_bar = SideBar()

    def savefig(self, filename: str = None):
        """
        Save plot to file.
        Possible file extensions are `.jpg`, `.png` and `.pdf`.
        The default directory for writing the file is the same as the
        directory where the script or notebook is running.
        """
        self.figure.savefig(filename)

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

    # def close(self):
    #     """
    #     Set the closed flag to True to output static images.
    #     """
    #     self.closed = True

    # def show(self):
    #     """
    #     Show the matplotlib figure.
    #     """
    #     self.fig.show()

    def draw(self):
        """
        """
        self.figure.draw()

    def render(self):
        return self.figure.render()

    def update(self, *args, **kwargs):
        return self.figure.update(*args, **kwargs)
