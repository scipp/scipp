# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .figure3d import PlotFigure3d
from .view import PlotView

class PlotView3d(PlotView):
    def __init__(self, *args, **kwargs):

        super().__init__(
            figure=PlotFigure3d(*args, **kwargs))

    def update_opacity(self, *args, **kwargs):
        self.figure.update_opacity(*args, **kwargs)

    def update_depth_test(self, *args, **kwargs):
        self.figure.update_depth_test(*args, **kwargs)

    def update_cut_surface(self, *args, **kwargs):
        self.figure.update_cut_surface(*args, **kwargs)
