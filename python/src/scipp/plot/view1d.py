# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .figure1d import PlotFigure1d
from .view import PlotView
from .._utils import make_random_color


class PlotView1d(PlotView):
    def __init__(self, *args, **kwargs):
        super().__init__(figure=PlotFigure1d(*args, **kwargs))

    def toggle_mask(self, change):
        self.figure.toggle_mask(change["owner"].mask_group,
                                change["owner"].mask_name, change["new"])

    def update_axes(self, *args, **kwargs):
        self.figure.update_axes(*args, **kwargs)

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
        self.figure.draw()

    def update_profile(self, event):
        if event.inaxes == self.figure.ax:
            self.interface["update_profile"](xdata=event.xdata)
            self.interface["toggle_hover_visibility"](True)
        else:
            self.interface["toggle_hover_visibility"](False)

    def keep_or_remove_profile(self, event):
        line_url = event.artist.get_url()
        if line_url == "axvline":
            self.remove_profile(event)
        else:
            self.keep_profile(event, line_url)
        self.figure.draw()

    def keep_profile(self, event, line_name):
        # The names of the data variables are stored in the masks information
        if line_name in self.figure.masks:
            xdata = event.mouseevent.xdata
            col = make_random_color(fmt='hex')
            self.profile_counter += 1
            line_id = self.profile_counter
            line = self.figure.ax.axvline(xdata, color=col, picker=True)
            line.set_pickradius(5.0)
            line.set_url("axvline")
            line.set_gid(line_id)
            self.interface["keep_line"](target="profile",
                                        name=line_name,
                                        color=col,
                                        line_id=line_id)

    def remove_profile(self, event):
        new_lines = []
        gid = event.artist.get_gid()
        url = event.artist.get_url()
        for line in self.figure.ax.lines:
            if not ((line.get_gid() == gid) and (line.get_url() == url)):
                new_lines.append(line)
        self.figure.ax.lines = new_lines

        # Also remove the line from the 1d plot
        self.interface["remove_line"](target="profile", line_id=gid)
