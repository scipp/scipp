# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .controller import PlotController
from ..units import one
import numpy as np


class PlotController1d(PlotController):
    """
    Controller class for 1d plots.
    """
    def render(self, *args, **kwargs):
        """
        Update axes (and data) to render the figure once all components
        have been created.
        """
        super().render(*args, **kwargs)
        self.view.rescale_to_data()
