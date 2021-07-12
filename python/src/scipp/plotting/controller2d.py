# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .controller import PlotController


class PlotController2d(PlotController):
    """
    Controller class for 2d plots.
    """
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def redraw(self):
        """
        Update the model data dicts and re-draw the figure.
        """
        self.model.reset_resampling_model()
        super().redraw()

    def home_view(self, button):
        # TODO There appears to be an issue with the mechanism of how the
        # matplotlib "home" button works. While I am not sure I understand
        # what is happening, "home" appears to remember the *initial* xlim
        # and ylim values. However, when we switch dims or transpose we
        # change these limits. See in particular PlotFigure2d.update_data.
        # I could not figure out where matplotlib stores the "home" limits.
        # We use limits stored in the view to handle this ourselves. Note
        # current hack with figure._limits_set due to shortcomings of how
        # axis lim changes are detected.
        self.view.figure._limits_set = False
        self.update_axes(slices=self.view.global_limits)
