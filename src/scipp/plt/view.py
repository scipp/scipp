# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .. import config
from .tools import fig_to_pngbytes
import ipywidgets as ipw
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker


class PlotView:
    """
    Base class for 1d and 2d figures, that holds matplotlib axes.
    """
    def __init__(self,
                 ax=None,
                 figsize=None,
                 title=None,
                 bounding_box=None,
                 xlabel=None,
                 ylabel=None,
                 toolbar=None,
                 grid=False):
        self.fig = None
        self.closed = False
        self.ax = ax
        self.own_axes = True
        self.toolbar = None
        self.bounding_box = bounding_box
        cfg = config['plot']
        if self.ax is None:
            if figsize is None:
                figsize = (cfg['width'] / cfg['dpi'], cfg['height'] / cfg['dpi'])
            self.fig, self.ax = plt.subplots(1, 1, figsize=figsize, dpi=cfg['dpi'])
            if self.bounding_box is None:
                self.bounding_box = cfg['bounding_box']
            self.fig.tight_layout(rect=self.bounding_box)
            if self.is_widget():
                self.toolbar = toolbar(external_toolbar=self.fig.canvas.toolbar)
                self.fig.canvas.toolbar_visible = False
        else:
            self.own_axes = False
            self.fig = self.ax.get_figure()

        self.ax.set_title(title)
        if grid:
            self.ax.grid()

        self.axformatter = {}
        self.axlocator = {}
        self.xlabel = xlabel
        self.ylabel = ylabel
        self.draw_no_delay = False
        self.event_connections = {}

        self.toolbar.connect(view=self)

    def initialize_toolbar(self, **kwargs):
        if self.toolbar is not None:
            self.toolbar.initialize(**kwargs)

    def is_widget(self):
        """
        Check whether we are using the Matplotlib widget backend or not.
        "on_widget_constructed" is an attribute specific to `ipywidgets`.
        """
        return hasattr(self.fig.canvas, "on_widget_constructed")

    def savefig(self, filename=None):
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

    def _to_widget(self):
        """
        Convert the Matplotlib figure to a widget. If the ipympl (widget)
        backend is in use, return the custom toolbar and the figure canvas.
        If not, convert the plot to a png image and place inside an ipywidgets
        Image container.
        """
        if self.is_widget():
            return ipw.HBox([
                self.toolbar._to_widget(),
                self._to_image() if self.closed else self.fig.canvas
            ])
        else:
            return self._to_image()

    def _to_image(self):
        """
        Convert the Matplotlib figure to a static image.
        """
        width, height = self.fig.get_size_inches()
        dpi = self.fig.get_dpi()
        return ipw.Image(value=fig_to_pngbytes(self.fig),
                         width=width * dpi,
                         height=height * dpi)

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

    def toggle_xaxis_scale(self, change):
        self.ax.set_xscale("log" if change['new'] else "linear")
        self.draw()

    def toggle_yaxis_scale(self, change):
        self.ax.set_yscale("log" if change['new'] else "linear")
        self.draw()

    def draw(self):
        """
        Manually update the figure.
        We control update manually since we have better control on how many
        draws are performed on the canvas. Matplotlib's automatic drawing
        (which we have disabled by using `plt.ioff()`) can degrade performance
        significantly.
        Matplotlib's `draw()` is slightly more expensive than `draw_idle()`
        but won't update inside a loop (only when the loop has finished
        executing).
        If `draw_no_delay` has been set to True (via `set_draw_no_delay`,
        then we use `draw()` instead of `draw_idle()`.
        """
        self.fig.canvas.draw_idle()

    def get_axis_bounds(self, axis):
        return getattr(self.ax, "get_{}lim".format(axis))()

    def set_axis_label(self, axis, string):
        getattr(self.ax, "set_{}label".format(axis))(string)
