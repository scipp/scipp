# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# from .controller import PlotController
from ..units import one
import numpy as np


class PlotController1d:
    """
    Controller class for 1d plots.
    """
    def __init__(
            self,
            dims,
            vmin=None,
            vmax=None,
            norm=None,
            # resampling_mode=None,
            scale=None,
            widgets=None,
            model=None,
            # profile_model=None,
            # panel=None,
            # profile=None,
            view=None):
        self._dims = dims
        self.widgets = widgets
        self.model = model
        # TODO calling copy here may not be enough to avoid interdependencies
        # self._profile_model = profile_model
        # self._profile_markers = MarkerModel()
        # self.panel = panel
        # # self.model.mode = resampling_mode
        # if view.figure.toolbar is not None:
        #     view.figure.toolbar.set_resampling_mode_display(self.model.is_resampling)
        # self.profile = profile
        # if profile is not None:
        #     self._profile_view = PlotView1d(figure=profile, formatters=view.formatters)
        #     self._profile_model.dims = self._dims[:-len(self.model.dims)]
        #     self._profile_model.mode = resampling_mode
        self.view = view

        self.vmin = vmin
        self.vmax = vmax
        self.norm = norm if norm is not None else "linear"

        self.scale = {dim: "linear" for dim in self._dims}
        if scale is not None:
            for dim, item in scale.items():
                self.scale[dim] = item
        # self.view.set_scale(scale=self.scale)

    def render(self):
        """
        Update axes (and data) to render the figure once all components
        have been created.
        """
        # if self.profile is not None:
        #     self.profile.connect()

        # self.widgets.connect(controller=self)
        # self.view.connect(controller=self)
        # if self.panel is not None:
        #     self.panel.controller = self
        self.update_data()

    def update_data(self, *, slices=None):
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
        new_values = self.model.update_data(slices)
        # self.widgets.update_slider_readout(new_values.meta)

        self.view.update_data(new_values)  #, mask_info=self.get_masks_info())
        # if self.panel is not None:
        #     self.panel.update_data(new_values)
        # if self.profile is not None:
        #     self._update_slice_area()

    # def update_line_color(self, line_id=None, color=None):
    #     """
    #     Get a message from the panel to change the color of a given line.
    #     Forward the message to the view.
    #     """
    #     self.view.update_line_color(line_id=line_id, color=color)

    # def refresh(self):
    #     """
    #     Do nothing for refresh in case of 1d plot.
    #     """
    #     return

    def rescale_to_data(self, button=None):
        """
        A small delta is used to add padding around the plotted points.
        """
        with_min_padding = self.vmin is None
        with_max_padding = self.vmax is None
        vmin, vmax = self.find_vmin_vmax(button=button)
        if vmin.unit is None:
            vmin.unit = one
            vmax.unit = one
        if self.norm == "log":
            delta = 10**(0.05 * np.log10(vmax.value / vmin.value))
            if with_min_padding or (button is not None):
                vmin /= delta
            if with_max_padding or (button is not None):
                vmax *= delta
        else:
            delta = 0.05 * (vmax - vmin)
            if with_min_padding or (button is not None):
                vmin -= delta
            if with_max_padding or (button is not None):
                vmax += delta
        self.view.rescale_to_data(vmin, vmax)

    # def toggle_norm(self, owner):
    #     """
    #     Toggle data normalization from toolbar button signal.
    #     """
    #     self.norm = "log" if owner.value else "linear"
    #     vmin, vmax = self.find_vmin_vmax()
    #     if self.norm == "log":
    #         self.rescale_to_data()
    #         self.view.toggle_norm(self.norm, vmin, vmax)
    #     else:
    #         self.view.toggle_norm(self.norm, vmin, vmax)
    #         self.rescale_to_data()
