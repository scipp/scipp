# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .controller import PlotController


class PlotController2d(PlotController):
    """
    Controller class for 2d plots.
    """
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def update_viewport(self, xylims):
        """
        When the view requests a viewport udpate, we get new image data from
        the model and send them back to the view which then updates the figure.
        """
        new_values = self.model.update_viewport(
            xylims, mask_info=self.get_masks_info())
        if new_values is not None:
            self.view.update_data(new_values)

    def connect_view(self):
        """
        Connect the view interface to callbacks.
        """
        super().connect_view()
        self.view.connect(
            view_callbacks={"update_viewport": self.update_viewport})

    def rescale_to_data(self, button=None):
        """
        When we rescale the colorbar limits, we also have to manually update
        the colors in the figure, as opposed to Matplotlib's `imshow` which
        does the update automatically.
        """
        super().rescale_to_data()
        new_values = self.model.get_slice_values(
            mask_info=self.get_masks_info())
        self.view.update_data(new_values)

    def toggle_mask(self, change=None):
        """
        Show/hide masks
        """
        new_values = self.model.get_slice_values(
            mask_info=self.get_masks_info())
        self.view.update_data(new_values)

    def toggle_norm(self, change):
        """
        When toggling the color normalization, we need to get values from
        the model to update the colors in 3d plots.
        """
        super().toggle_norm(change)
        new_values = self.model.get_slice_values(
            mask_info=self.get_masks_info())
        self.view.update_data(new_values)