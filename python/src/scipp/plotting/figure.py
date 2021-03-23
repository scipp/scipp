# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .. import config
import ipywidgets as ipw
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import matplotlib.transforms as transf
import numpy as np
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
                 ndim=1,
                 xlabel=None,
                 ylabel=None,
                 toolbar=None):
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
                self.toolbar = toolbar(canvas=self.fig.canvas)
                self.fig.canvas.toolbar_visible = False
        else:
            self.own_axes = False
            self.fig = self.ax.get_figure()

        self.ax.set_title(title)

        self.axformatter = {}
        self.axlocator = {}
        self.xlabel = xlabel
        self.ylabel = ylabel
        # self.axis_offsets = {"y_placement": -0.07, "x_placement": 0.0}
        # self.axis_boundaries = {}

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

    def _to_widget(self, as_static=False):
        """
        Convert the Matplotlib figure to a widget. If the ipympl (widget)
        backend is in use, return the custom toolbar and the figure canvas.
        If not, convert the plot to a png image and place inside an ipywidgets
        Image container.
        """
        if self.is_widget():
            if as_static:
                return ipw.HBox([self.toolbar._to_widget(), self._to_image()])
            else:
                return ipw.HBox([self.toolbar._to_widget(), self.fig.canvas])
        else:
            return self._to_image()

    def _to_image(self):
        """
        Convert the Matplotlib figure to a static image.
        """
        buf = io.BytesIO()
        self.fig.savefig(buf, format='png')
        # Here we close the figure to prevent it from showing up again in
        # cells further down the notebook.
        plt.close(self.fig)
        buf.seek(0)
        return ipw.Image(value=buf.getvalue(),
                         width=config.plot.width,
                         height=config.plot.height)

    def show(self):
        """
        Show the matplotlib figure.
        """
        self.fig.show()

    def initialise(self, axformatters):
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
                "linear":
                ticker.MaxNLocator(integer=True) if
                axformatters[dim]["custom_locator"] else ticker.AutoLocator(),
                "log":
                ticker.LogLocator()
            }

    def connect(self, callbacks):
        """
        Connect the toolbar to callback from the controller. This includes
        rescaling the data norm, and change the scale (log or linear) on the
        axes.
        """
        if self.toolbar is not None:
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
        """
        Update the state (value and color) of toolbar log axes buttons when
        axes or dimensions are swapped.
        """
        if self.toolbar is not None:
            self.toolbar.update_log_axes_buttons(*args, **kwargs)

    def update_norm_button(self, *args, **kwargs):
        """
        Change state of norm button in toolbar.
        """
        if self.toolbar is not None:
            self.toolbar.update_norm_button(*args, **kwargs)

    def home_view(self, *args, **kwargs):
        self.toolbar.home_view()

    def pan_view(self, *args, **kwargs):
        self.toolbar.pan_view()

    def zoom_view(self, *args, **kwargs):
        self.toolbar.zoom_view()

    def save_view(self, *args, **kwargs):
        self.toolbar.save_view()

    def get_axis_bounds(self, axis):
        return getattr(self.ax, "get_{}lim".format(axis))()

    # def set_axis_offset(self, axis, string):
    #     # import os
    #     # os.write(1, "set_axis_offset 1\n".encode())
    #     self.axis_offsets[axis].set_text(string)
    #     # os.write(1, "set_axis_offset 1.1\n".encode())
    #     # t = self.ax.transData.inverted()
    #     # os.write(1, "set_axis_offset 1.2\n".encode())
    #     # bb = self.axis_offsets[axis].get_window_extent(
    #     #     renderer=self.fig.canvas.get_renderer())
    #     # os.write(1, "set_axis_offset 1.3\n".encode())
    #     # bb = np.ravel(bb.transformed(t))

    #     # os.write(1, "set_axis_offset 2\n".encode())
    #     # if position is not None:
    #     #     os.write(1, "set_axis_offset 3\n".encode())
    #     #     # getattr(self.axis_offsets[axis], "set_{}".format(axis))(position)
    #     #     if axis == "x":
    #     #        self.axis_offsets[axis].xy = [position, self.axis_offsets[axis].xy[1]]
    #     #        self.axis_offsets[axis].xyann = [position, self.axis_offsets[axis].xyann[1]]
    #     #     else:
    #     #        self.axis_offsets[axis].xy = [self.axis_offsets[axis].xy[0], position]
    #     #        self.axis_offsets[axis].xyann = [self.axis_offsets[axis].xyann[0], position]

    #     #     os.write(1, "set_axis_offset 4\n".encode())
    #     # else:
    #     #     os.write(1, "set_axis_offset 5\n".encode())
    #     #     lims = getattr(self.ax, "get_{}lim".format(axis))
    #     #     # getattr(self.ax,
    #     #     #         "set_{}".format(axis))(0.7 * (lims[1] - lims[0]) + lims[0])
    #     #     if axis == "x":
    #     #         os.write(1, (str(lims[1]-(bb[2]-bb[0]))+"\n").encode())
    #     #         self.axis_offsets[axis].xy = [lims[1]-(bb[2]-bb[0]),
    #     #             self.axis_offsets[f"{axis}_placement"]]
    #     #     else:
    #     #         self.axis_offsets[axis].xy = [self.axis_offsets[f"{axis}_placement"],
    #     #             lims[1]]
    #     #     os.write(1, "set_axis_offset 6\n".encode())
    #     # os.write(1, "set_axis_offset 7\n".encode())

    def set_axis_label(self, axis, string):
        getattr(self.ax, "set_{}label".format(axis))(string)

    # def create_axis_offsets(self):
    #     """
    #     The hard-coded numbers represent x/y locations of the text, in
    #     normalized axis coordinates (i.e. the lower and upper figure axes are
    #     drawn at 0 and 1, respectively).
    #     """
    #     self.axis_offsets.update({
    #         # "x":
    #         # self.ax.annotate('', xy=[0.0, 0.0], xytext=[0.0, -0.07],
    #         #     arrowprops=dict(facecolor='black', arrowstyle="-"),
    #         #                              ha='left',
    #         #              va='top',
    #         #              transform=transf.blended_transform_factory(
    #         #                  self.ax.transData, self.ax.transAxes)),
    #         #             "y":
    #         # self.ax.annotate('', xy=[0.0, 0.0], xytext=[0.0, 0.0],
    #         #     arrowprops=dict(facecolor='black', arrowstyle="-"),
    #         #                              ha='left',
    #         #              va='bottom',
    #         #              transform=transf.blended_transform_factory(
    #         #                  self.ax.transAxes, self.ax.transData))
    #         # })

    #         "x": self.ax.text(0.70,
    #                      -0.07,
    #                      "",
    #                      ha='left',
    #                      va='top',
    #                      transform=self.ax.transAxes),
    #         "y":
    #         self.ax.text(0.0,
    #                      1.0,
    #                      "",
    #                      ha='left',
    #                      va='bottom',
    #                      transform=self.ax.transAxes),
    #     })

    #     # self.axis_boundaries.update({
    #     #     "x":
    #     #     self.ax.text(0.0, 0.0, "", ha='left', va='top'),
    #     #     "y":
    #     #     self.ax.text(0.0, 0.0, "", ha='left', va='bottom'),
    #     #     "line":
    #     #     self.ax.plot([0, 0])
    #     # })

    # def reset_axis_offsets(self):
    #     for key in "xy":
    #         self.axis_offsets[key].set_text("")

    # # def set_axis_boundary(self, axis, boundary):
    # #     self.axis_boundaries[axis].set_text(boundary["text"])
    # #     self.axis_boundaries[axis].set_x(boundary["loc"])
    # #     # self.ax.plotis_boundaries[axis].set_position([boundary["loc"], -0.05])
