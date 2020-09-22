# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import config
# from .profiler import Profiler
from .lineplot import LinePlot
from .tools import to_bin_edges, parse_params
# from .widgets import PlotWidgets
from .._utils import name_with_unit, make_random_color
from .._scipp import core as sc
from .. import detail

# Other imports
import numpy as np
import ipywidgets as ipw
import matplotlib.pyplot as plt
from matplotlib.axes import Subplot
import warnings
import io
import os


class PlotView1d:
    def __init__(self,
                 controller=None,
                 ax=None,
                 errorbars=None,
                 title=None,
                 unit=None,
                 logx=False,
                 logy=False,
                 mask_params=None,
                 mask_names=None,
                 mpl_line_params=None,
                 grid=False,
                 picker=None):

        self.controller = controller
        self.profile_hover_connection = None
        self.profile_pick_connection = None
        self.profile_update_lock = False
        self.profile_scatter = None
        self.profile_counter = -1

        self.figure = LinePlot(errorbars=errorbars,
                               ax=ax,
                               mpl_line_params=mpl_line_params,
                               title=title,
                               unit=unit,
                               logx=logx,
                               logy=logy,
                               grid=grid,
                               mask_params=mask_params,
                               mask_names=mask_names,
                               picker=picker)

        return

    def _ipython_display_(self):
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        return self.figure._to_widget()

    def savefig(self, filename=None):
        self.figure.savefig(filename=filename)

    def toggle_mask(self, change):
        self.figure.toggle_mask(change["owner"].mask_group,
                                change["owner"].mask_name, change["new"])

    def rescale_to_data(self, vmin=None, vmax=None):
        self.figure.rescale_to_data()

    def update_axes(self, axparams, axformatter, axlocator, logx, logy):
        self.figure.update_axes(axparams, axformatter, axlocator, logx, logy)

    def update_data(self, new_values):
        self.figure.update_data(new_values)

    def keep_line(self, name=None, color=None, line_id=None):
        self.figure.keep_line(name=name, color=color, line_id=line_id)

    def remove_line(self, line_id):
        self.figure.remove_line(line_id)

    def update_line_color(self, line_id, color):
        self.figure.update_line_color(line_id, color)

    def reset_profile(self):
        if self.profile_scatter is not None:
            self.profile_scatter = None
            self.ax.collections = []
            self.fig.canvas.draw_idle()

    def update_profile(self, event):
        if event.inaxes == self.figure.ax:
            self.controller.update_profile(xdata=event.xdata)
            self.controller.toggle_hover_visibility(True)
        else:
            self.controller.toggle_hover_visibility(False)

    def keep_or_remove_profile(self, event):
        if event.artist.get_url() == "axvline":
            self.remove_profile(event)
        else:
            self.keep_profile(event)
        self.figure.fig.canvas.draw_idle()

    def update_profile_connection(self, visible):
        # Connect picking events
        if visible:
            self.profile_pick_connection = self.figure.fig.canvas.mpl_connect(
                'pick_event', self.keep_or_remove_profile)
            self.profile_hover_connection = self.figure.fig.canvas.mpl_connect(
                'motion_notify_event', self.update_profile)
        else:
            if self.profile_pick_connection is not None:
                self.figure.fig.canvas.mpl_disconnect(
                    self.profile_pick_connection)
            if self.profile_hover_connection is not None:
                self.figure.fig.canvas.mpl_disconnect(
                    self.profile_hover_connection)

    def keep_profile(self, event):
        xdata = event.mouseevent.xdata
        col = make_random_color(fmt='hex')
        self.profile_counter += 1
        line_id = self.profile_counter
        line = self.figure.ax.axvline(xdata, color=col, picker=5)
        line.set_url("axvline")
        line.set_gid(line_id)
        self.controller.keep_line(target="profile", color=col, line_id=line_id)

    def remove_profile(self, event):
        new_lines = []
        gid = event.artist.get_gid()
        url = event.artist.get_url()
        for line in self.figure.ax.lines:
            if not ((line.get_gid() == gid) and (line.get_url() == url)):
                new_lines.append(line)
        self.figure.ax.lines = new_lines

        # Also remove the line from the 1d plot
        self.controller.remove_line(target="profile", line_id=gid)
