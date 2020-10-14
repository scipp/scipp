# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .controller import PlotController


class PlotController2d(PlotController):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def update_viewport(self, xylims):
        new_values = self.model.update_viewport(
            xylims, mask_info=self.get_masks_info())
        self.view.update_data(new_values)

    def connect_view(self):
        super().connect_view()
        self.view.connect({"update_viewport": self.update_viewport})
