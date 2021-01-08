# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet


class PlotView:
    """
    Base class for a plot view.
    It holds a `figure`, which can be either a Matplotlib based figure (1d and
    2d) or a pythreejs scene (3d).

    The difference between a `PlotView` and a `PlotFigure` is that the
    `PlotView` also handles the communications with the `PlotController` that
    are to do with the `PlotProfile` plot displayed below the `PlotFigure`.
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

    def show(self):
        """
        Forward the call to show() to the figure.
        """
        self.figure.show()

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

    def connect(self, view_callbacks=None, figure_callbacks=None):
        """
        Connect the view interface to the callbacks provided by the
        `controller`.
        """
        if view_callbacks is not None:
            for key, func in view_callbacks.items():
                self.interface[key] = func
        if figure_callbacks is not None:
            self.figure.connect(figure_callbacks)

    def home_view(self, *args, **kwargs):
        self.figure.home_view(*args, **kwargs)

    def pan_view(self, *args, **kwargs):
        self.figure.pan_view(*args, **kwargs)

    def zoom_view(self, *args, **kwargs):
        self.figure.zoom_view(*args, **kwargs)

    def save_view(self, *args, **kwargs):
        self.figure.save_view(*args, **kwargs)

    def rescale_to_data(self, *args, **kwargs):
        """
        Forward rescaling to the `figure`.
        """
        self.figure.rescale_to_data(*args, **kwargs)

    def toggle_mask(self, change=None):
        """
        Dummy toggle_mask function.
        """
        return

    def toggle_norm(self, *args, **kwargs):
        """
        Forward norm change to the `figure`.
        """
        self.figure.toggle_norm(*args, **kwargs)

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

    def update_log_axes_buttons(self, *args, **kwargs):
        """
        Forward log buttons update to the `figure`.
        """
        self.figure.update_log_axes_buttons(*args, **kwargs)

    def update_norm_button(self, *args, **kwargs):
        """
        Forward norm button update to the `figure`.
        """
        self.figure.update_norm_button(*args, **kwargs)
