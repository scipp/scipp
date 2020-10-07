# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .figure1d import PlotFigure1d
from .._utils import make_random_color


class PlotView1d:
    def __init__(self,
                 # controller=None,
                 ax=None,
                 figsize=None,
                 errorbars=None,
                 title=None,
                 unit=None,
                 norm=None,
                 # logx=False,
                 # logy=False,
                 # mask_params=None,
                 masks=None,
                 mpl_line_params=None,
                 grid=False,
                 picker=False):

        # self.controller = controller
        self.profile_hover_connection = None
        self.profile_pick_connection = None
        self.profile_update_lock = False
        self.profile_counter = -1

        self.figure = PlotFigure1d(errorbars=errorbars,
                                   ax=ax,
                                   mpl_line_params=mpl_line_params,
                                   title=title,
                                   unit=unit,
                                   norm=norm,
                                   # logx=logx,
                                   # logy=logy,
                                   grid=grid,
                                   # mask_params=mask_params,
                                   masks=masks,
                                   picker=picker)

    def _ipython_display_(self):
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        return self.figure._to_widget()

    def savefig(self, *args, **kwargs):
        self.figure.savefig(*args, **kwargs)

    def toggle_mask(self, change):
        self.figure.toggle_mask(change["owner"].mask_group,
                                change["owner"].mask_name, change["new"])

    def rescale_to_data(self, vmin=None, vmax=None):
        self.figure.rescale_to_data()

    def update_axes(self, *args, **kwargs):
        self.figure.update_axes(*args, **kwargs)

    def update_data(self, *args, **kwargs):
        self.figure.update_data(*args, **kwargs)

    def keep_line(self, *args, **kwargs):
        self.figure.keep_line(*args, **kwargs)

    def remove_line(self, *args, **kwargs):
        self.figure.remove_line(*args, **kwargs)

    def update_line_color(self, *args, **kwargs):
        self.figure.update_line_color(*args, **kwargs)

    def reset_profile(self):
        new_lines = []
        for line in self.figure.ax.lines:
            if not (line.get_url() == "axvline"):
                new_lines.append(line)
        self.figure.ax.lines = new_lines
        self.figure.fig.canvas.draw_idle()

    def update_profile(self, event):
        if event.inaxes == self.figure.ax:
            self.controller.update_profile(xdata=event.xdata)
            self.controller.toggle_hover_visibility(True)
        else:
            self.controller.toggle_hover_visibility(False)

    def keep_or_remove_profile(self, event):
        line_url = event.artist.get_url()
        if line_url == "axvline":
            self.remove_profile(event)
        else:
            self.keep_profile(event, line_url)
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

    def keep_profile(self, event, line_name):
        xdata = event.mouseevent.xdata
        col = make_random_color(fmt='hex')
        self.profile_counter += 1
        line_id = self.profile_counter
        line = self.figure.ax.axvline(xdata, color=col, picker=True)
        line.set_pickradius(5.0)
        line.set_url("axvline")
        line.set_gid(line_id)
        self.controller.keep_line(target="profile",
                                  name=line_name,
                                  color=col,
                                  line_id=line_id)
        # self.rescale_to_data()

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
