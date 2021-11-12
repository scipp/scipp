# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from ..core import zeros
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

    def _make_data(self, new_values, mask_info):
        # TODO could handle multiple clouds here?
        array = next(iter(new_values.values()))
        new_values = {'data': array}
        mask_info = next(iter(mask_info.values()))
        if len(mask_info) > 0:
            # Use automatic broadcasting in Scipp variables
            msk = zeros(sizes=array.sizes, dtype='int32')
            for m, val in mask_info.items():
                if val:
                    msk += array.masks[m].astype(msk.dtype)
            new_values['mask'] = msk
        return new_values

    def update_data(self, new_values, mask_info=None):
        """
        Forward data update to the `figure`.
        """
        self._data = new_values
        # TODO In principle we should use actual dimension here, usually x,y,z
        self.figure.toolbar.dims = []
        self.refresh(mask_info)

    def set_position_params(self, params):
        self.figure.set_position_params(params)

    def remove_cut_surface(self):
        self.figure.remove_cut_surface()

    def add_cut_surface(self, *args, **kwargs):
        self.figure.add_cut_surface(*args, **kwargs)
