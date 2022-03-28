# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .figure1d import Figure1d
from .view import View


class View1d(View):
    """
    Class for 1 dimensional plots.
    """
    def __init__(self, **kwargs):

        super().__init__(figure=Figure1d, **kwargs)

        self.toolbar.add_button(name="home_view",
                                callback=self.figure.home_view,
                                icon="home",
                                tooltip="Reset original view")
        self.toolbar.add_togglebutton(name="pan_view",
                                      callback=self.figure.pan_view,
                                      icon="arrows",
                                      tooltip="Pan")
        self.toolbar.add_togglebutton(name="zoom_view",
                                      callback=self.figure.zoom_view,
                                      icon="square-o",
                                      tooltip="Zoom")
        self.toolbar.add_togglebutton(name='toggle_xaxis_scale',
                                      callback=self.figure.toggle_xaxis_scale,
                                      description="logx")
        self.toolbar.add_togglebutton(name="toggle_norm",
                                      callback=self.figure.toggle_norm,
                                      description="logy",
                                      tooltip="log(data)")
        self.toolbar.add_button(name="save_view",
                                callback=self.figure.save_view,
                                icon="save",
                                tooltip="Save")
