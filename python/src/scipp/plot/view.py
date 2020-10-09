# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet


class PlotView:
    def __init__(self, figure=None):

        self.figure = figure

        self.interface = {}
        self.profile_hover_connection = None
        self.profile_pick_connection = None
        self.profile_update_lock = False
        self.profile_scatter = None
        self.profile_counter = -1
        self.profile_ids = []

    def _ipython_display_(self):
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        return self.figure._to_widget()

    def savefig(self, *args, **kwargs):
        self.figure.savefig(*args, **kwargs)

    def initialise(self, *args, **kwargs):
        self.figure.initialise(*args, **kwargs)

    def connect(self, callbacks):
        for key, func in callbacks.items():
            self.interface[key] = func

    def rescale_to_data(self, vmin=None, vmax=None):
        self.figure.rescale_to_data(vmin=vmin, vmax=vmax)

    def update_data(self, *args, **kwargs):
        self.figure.update_data(*args, **kwargs)
