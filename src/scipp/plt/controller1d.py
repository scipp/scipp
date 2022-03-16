# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .controller import Controller


class Controller1d(Controller):
    """
    Controller class for 1d plots.
    """
    def render(self, *args, **kwargs):
        """
        Update axes (and data) to render the figure once all components
        have been created.
        """
        super().render(*args, **kwargs)
        self._view.rescale_to_data()
        self._view.set_axes_labels()

    # def update(self):
    #     """
    #     This function is called when the data in the displayed 1D plot or 2D
    #     image is to be updated. This happens for instance when we move a slider
    #     which is navigating an additional dimension. It is also always
    #     called when update_axes is called since the displayed data needs to be
    #     updated when the axes have changed.
    #     """
    #     # if slices is None:
    #     #     slices = self.widgets.slices
    #     # else:
    #     #     slices.update(self.widgets.slices)

    #     data_processors = self._make_data_processors()

    #     # slices = self.widgets.slices

    #     for key in self.model.keys():
    #         # print(type(model))
    #         new_values = self.model.update(key=key, data_processors=data_processors)
    #         new_values.name = key
    #         self.view.update(new_values)

    #     self.view.draw()