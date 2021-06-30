# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .view import PlotView


class PlotView3d(PlotView):
    """
    View object for 3 dimensional plots. Contains a `PlotFigure3d`.

    The view also handles events to do with updating opacities of the cut
    surface.

    This will also be handling profile picking events in the future.
    """
    def __init__(self, figure, formatters):
        super().__init__(figure=figure, formatters=formatters)
        self._axes = ['z', 'y', 'x']

    def update_opacity(self, *args, **kwargs):
        self.figure.update_opacity(*args, **kwargs)

    def update_depth_test(self, *args, **kwargs):
        self.figure.update_depth_test(*args, **kwargs)

    def close(self):
        """
        Dummy close function because 3d plots cannot be closed like mpl
        figures.
        """
        return

    def update_data(self, new_values, mask_info=None):
        """
        Forward data update to the `figure`.
        """
        self._data = new_values
        # self._dims = []
        self.refresh(mask_info)
