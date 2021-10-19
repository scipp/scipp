# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .. import config
from .tools import fig_to_pngbytes
import ipywidgets as ipw
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker


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
                 ndim=1,
                 xlabel=None,
                 ylabel=None,
                 toolbar=None,
                 grid=False):
        self.fig = None
        self.closed = False
        self.ax = ax
        self.cax = cax
        self.own_axes = True
        self.toolbar = None
        self.padding = padding
        if self.ax is None:
            if figsize is None:
                figsize = (config.plot.width / config.plot.dpi,
                           config.plot.height / config.plot.dpi)
            self.fig, self.ax = plt.subplots(1, 1, figsize=figsize, dpi=config.plot.dpi)
            if self.padding is None:
                self.padding = config.plot.padding
            self.fig.tight_layout(rect=self.padding)
            if self.is_widget():
                self.toolbar = toolbar(mpl_toolbar=self.fig.canvas.toolbar)
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

    def initialize(self, axformatters):
        """
        Initialize figure parameters once the model has been created, since
        the axes formatters are defined by the model.
        """
        self._formatters = axformatters
        for axis, formatter in self._formatters.items():
            self.axformatter[axis] = {}
            for key in ["linear", "log"]:
                if formatter[key] is None:
                    self.axformatter[axis][key] = ticker.ScalarFormatter()
                else:
                    form = formatter[key]
                    if "need_callbacks" in formatter:
                        from functools import partial
                        form = partial(form,
                                       axis=axis,
                                       get_axis_bounds=self.get_axis_bounds,
                                       set_axis_label=self.set_axis_label)
                    self.axformatter[axis][key] = ticker.FuncFormatter(form)
            self.axlocator[axis] = {
                "linear":
                ticker.MaxNLocator(integer=True)
                if axformatters[axis]["custom_locator"] else ticker.AutoLocator(),
                "log":
                ticker.LogLocator()
            }

    def connect(self, controller):
        """
        Connect the toolbar to callback from the controller. This includes
        rescaling the data norm, and change the scale (log or linear) on the
        axes.
        """
        if self.toolbar is not None:
            self.toolbar.connect(controller=controller)

    def toggle_mouse_events(self, active, event_handler):
        if active:
            self.event_connections['button_press_event'] = self.fig.canvas.mpl_connect(
                'button_press_event', event_handler.handle_button_press)
            self.event_connections['pick_event'] = self.fig.canvas.mpl_connect(
                'pick_event', event_handler.handle_pick)
            self.event_connections['motion_notify_event'] = self.fig.canvas.mpl_connect(
                'motion_notify_event', event_handler.handle_motion_notify)
        else:
            for cid in self.event_connections.values():
                self.fig.canvas.mpl_disconnect(cid)
            self.event_connections.clear()

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
        if self.draw_no_delay:
            self.fig.canvas.draw()
        else:
            self.fig.canvas.draw_idle()

    def get_axis_bounds(self, axis):
        return getattr(self.ax, "get_{}lim".format(axis))()

    def set_axis_label(self, axis, string):
        getattr(self.ax, "set_{}label".format(axis))(string)

    def set_draw_no_delay(self, value):
        self.draw_no_delay = value
