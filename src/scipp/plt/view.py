# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

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
    def __init__(self, figure, **kwargs):

        self.figure = figure(**kwargs)
        self.toolbar = Toolbar()

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
