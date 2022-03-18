# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .controller import Controller


class Controller1d(Controller):
    """
    Controller class for 1d plots.
    """
    def render(self, *args, **kwargs):
        """
        """
        super().render(*args, **kwargs)
        self._view.rescale_to_data()
        self._view.set_axes_labels()
