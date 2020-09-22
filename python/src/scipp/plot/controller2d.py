# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .controller import PlotController


class PlotController2d(PlotController):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def update_viewport(self, xylims):
        new_values = self.model.update_viewport(
            xylims, mask_info=self._get_mask_info())
        self.view.update_data(new_values)
