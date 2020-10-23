# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet


class PlotView:
    """
    Base class for a plot view.
    It holds a `figure`, which can be either a Matplotlib based figure (1d and
    2d) or a pythreejs scene (3d).

    The difference between a `view` and a `figure` is that the `view` also
    handles the communications with the `controller` that are to do with the
    `profile` plot displayed below the `figure`.
    """
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
        """
        IPython display representation for Jupyter notebooks.
        """
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        """
        The `view` as a widget is just the `figure` as a widget.
        """
        return self.figure._to_widget()

    def savefig(self, *args, **kwargs):
        """
        Forward figure saving to the `figure`.
        """
        self.figure.savefig(*args, **kwargs)

    def initialise(self, *args, **kwargs):
        """
        Forward figure initialization.
        """
        self.figure.initialise(*args, **kwargs)

    def connect(self, callbacks):
        """
        Connect the view interface to the callbacks provided by the
        `controller`.
        """
        for key, func in callbacks.items():
            self.interface[key] = func

    def rescale_to_data(self, vmin=None, vmax=None):
        """
        Forward rescaling to the `figure`.
        """
        self.figure.rescale_to_data(vmin=vmin, vmax=vmax)

    def update_axes(self, *args, **kwargs):
        """
        Forward axes update to the `figure`.
        """
        self.figure.update_axes(*args, **kwargs)

    def update_data(self, *args, **kwargs):
        """
        Forward data update to the `figure`.
        """
        self.figure.update_data(*args, **kwargs)

    def update_profile_connection(self, visible):
        """
        Connect or disconnect profile pick and hover events.
        """
        if visible:
            self.profile_pick_connection, self.profile_hover_connection = \
                self.figure.connect_profile(
                    self.keep_or_remove_profile, self.update_profile)
        else:
            self.figure.disconnect_profile(self.profile_pick_connection,
                                           self.profile_hover_connection)
            self.profile_pick_connection = None
            self.profile_hover_connection = None
