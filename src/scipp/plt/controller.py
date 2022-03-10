# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from ..units import one
import numpy as np


class PlotController:
    """
    Controller class plots.
    """
    def __init__(self,
                 dims,
                 vmin=None,
                 vmax=None,
                 norm=None,
                 scale=None,
                 widgets=None,
                 model=None,
                 view=None):
        self._dims = dims
        self.widgets = widgets
        self.model = model
        self.view = view

        self.vmin = vmin
        self.vmax = vmax
        self.norm = norm if norm is not None else "linear"

        self.scale = {dim: "linear" for dim in self._dims}
        if scale is not None:
            for dim, item in scale.items():
                self.scale[dim] = item

    def render(self):
        """
        Update axes (and data) to render the figure once all components
        have been created.
        """
        self.widgets.connect(controller=self)
        # self.view.connect(controller=self)
        # if self.panel is not None:
        #     self.panel.controller = self
        self.update_data()

    # def update(self, *, slices=None):
    def update(self):
        """
        This function is called when the data in the displayed 1D plot or 2D
        image is to be updated. This happens for instance when we move a slider
        which is navigating an additional dimension. It is also always
        called when update_axes is called since the displayed data needs to be
        updated when the axes have changed.
        """
        # if slices is None:
        #     slices = self.widgets.slices
        # else:
        #     slices.update(self.widgets.slices)

        data_processors = []

        slices = self.widgets.slices
        new_values = self.model.update_data(slices)
        # change to: new_values = self.model[slices]
        # Model could just be a data array

        # INSERT additional post-processing here
        # - a generic function to do, for example, some custom resampling

        # self.widgets.update_slider_readout(new_values.meta)

        self.view.update_data(new_values)  #, mask_info=self.get_masks_info())
        # if self.panel is not None:
        #     self.panel.update_data(new_values)
        # if self.profile is not None:
        #     self._update_slice_area()
