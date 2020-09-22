# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

import numpy as np


class ProfileView(LinePlot):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        return

    def toggle_hover_visibility(self, value):
        # If the mouse moves off the image, we hide the profile. If it moves
        # back onto the image, we show the profile
        for name in self.data_lines:
            self.data_lines[name].set_visible(value)
        for name in self.error_lines:
            # Need to get the 3rd element of the errorbar container, which
            # contains the vertical errorbars, and then the first element of
            # that because it is a tuple itself.
            self.error_lines[name][2][0].set_visible(value)
        for name, mlines in self.mask_lines.items():
            for ml in mlines.values():
                ml.set_visible(value)

    def update_slice_area(self, profile_slice):

        xstart = profile_slice["location"] - 0.5*profile_slice["thickness"]
        xend = profile_slice["location"] + 0.5*profile_slice["thickness"]

        new_xy = np.array([[xstart, 0.0],
                           [xstart, 1.0],
                           [xend, 1.0],
                           [xend, 0.0],
                           [xstart, 0.0]])

        self.slice_area.set_xy(new_xy)
        self.fig.canvas.draw_idle()

