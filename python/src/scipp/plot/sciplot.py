# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

import ipywidgets as ipw


class SciPlot:

    def __init__(self):

        self.controller = None
        self.model = None
        self.panel = None
        self.profile = None
        self.view = None

    def _ipython_display_(self):
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        widget_list = []
        for item in [self.view, self.profile, self.controller, self.panel]:
            if item is not None:
                widget_list.append(item._to_widget())
        return ipw.VBox(widget_list)

    def savefig(self, filename=None):
        self.view.savefig(filename=filename)


    def _connect_controller_members(self):
        self.controller.model = self.model
        self.controller.panel = self.panel
        self.controller.profile = self.profile
        self.controller.view = self.view
