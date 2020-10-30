# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .. import config
from .toolbar import PlotToolbar
import ipywidgets as ipw
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import io


class PlotFigure:
    """
    Base class for 1d and 2d figures, that holds matplotlib axes.
    """
    def __init__(self,
                 ax=None,
                 cax=None,
                 figsize=None,
                 title=None,
                 padding=None,
                 ndim=1):
        self.fig = None
        self.ax = ax
        self.cax = cax
        self.own_axes = True
        self.toolbar = None
        if self.ax is None:
            if figsize is None:
                figsize = (config.plot.width / config.plot.dpi,
                           config.plot.height / config.plot.dpi)
            self.fig, self.ax = plt.subplots(1,
                                             1,
                                             figsize=figsize,
                                             dpi=config.plot.dpi)
            if padding is None:
                padding = config.plot.padding
            self.fig.tight_layout(rect=padding)
            if self.is_widget():
                # We create a custom toolbar
                self.toolbar = PlotToolbar(canvas=self.fig.canvas, ndim=ndim)
                self.fig.canvas.toolbar_visible = False
        else:
            self.own_axes = False
            self.fig = self.ax.get_figure()

        self.ax.set_title(title)

        self.axformatter = {}
        self.axlocator = {}

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
            return ipw.HBox([self.toolbar._to_widget(), self.fig.canvas])
        else:
            buf = io.BytesIO()
            self.fig.savefig(buf, format='png')
            buf.seek(0)
            return ipw.Image(value=buf.getvalue(),
                             width=config.plot.width,
                             height=config.plot.height)

    def initialise(self, axformatters=None):
        """
        Initialise figure parameters once the model has been created, since
        the axes formatters are defined by the model.
        """
        for dim in axformatters:
            self.axformatter[dim] = {}
            for key in ["linear", "log"]:
                if axformatters[dim][key] is None:
                    self.axformatter[dim][key] = ticker.ScalarFormatter()
                else:
                    self.axformatter[dim][key] = ticker.FuncFormatter(
                        axformatters[dim][key])
            self.axlocator[dim] = {
                "linear": ticker.AutoLocator(),
                "log": ticker.LogLocator()
            }
            if axformatters[dim]["custom_locator"]:
                self.axlocator[dim]["linear"] = ticker.MaxNLocator(
                    integer=True)

    def connect(self, callbacks):
        self.toolbar.connect(callbacks)

    def draw(self):
        """
        Manually update the figure.
        We control update manually since we have better control on how many
        draws are performed on the canvas. Matplotlib's automatic drawing
        (which we have disabled by using `plt.ioff()`) can degrade performance
        significantly.
        """
        self.fig.canvas.draw_idle()

    def connect_profile(self, pick_callback=None, hover_callback=None):
        """
        Connect the figure to provided callbacks to handle profile picking and
        hovering updates.
        """
        pick_connection = self.fig.canvas.mpl_connect('pick_event',
                                                      pick_callback)
        hover_connection = self.fig.canvas.mpl_connect('motion_notify_event',
                                                       hover_callback)
        return pick_connection, hover_connection

    def disconnect_profile(self, pick_connection=None, hover_connection=None):
        """
        Disconnect profile events when the profile is hidden.
        """
        if pick_connection is not None:
            self.fig.canvas.mpl_disconnect(pick_connection)
        if hover_connection is not None:
            self.fig.canvas.mpl_disconnect(hover_connection)

    def update_log_axes_buttons(self, *args, **kwargs):
        self.toolbar.update_log_axes_buttons(*args, **kwargs)
