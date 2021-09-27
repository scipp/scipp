# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
from .._scipp import core
import numpy as np


def _slice_params(array, dim, loc):
    coord = array.meta[dim]
    # Use first entry. Note that this is problematic if ranges overlap only
    # partially.
    if not isinstance(array, core.DataArray):
        array = next(iter(array.values()))
    if array.sizes[dim] + 1 == coord.sizes[dim]:
        _, i = core.get_slice_params(array.data, coord, loc * coord.unit)
        if i < 0 or i + 1 >= coord.sizes[dim]:
            return None
        return coord[dim, i], coord[dim, i + 1]
    else:
        # get_slice_params only handles *exact* matches
        return int(np.argmin(np.abs(coord.values - loc)))


class PlotView:
    """
    Base class for a plot view.
    It holds a `figure`, which can be either a Matplotlib based figure (1d and
    2d) or a pythreejs scene (3d).

    The difference between a `PlotView` and a `PlotFigure` is that the
    `PlotView` also handles the communications with the `PlotController` that
    are to do with the `PlotProfile` plot displayed below the `PlotFigure`.
    """
    def __init__(self, figure, formatters):
        self._dims = None
        self._scale = None
        self.figure = figure
        self.formatters = formatters
        self.controller = {}
        self._pick_lock = False
        self._data = None

    @property
    def axes(self):
        return self._axes

    @property
    def dims(self):
        return self._dims

    @property
    def data(self):
        return self._data

    def set_scale(self, scale):
        """
        Set new scales for dims. Takes effect after update_data() is called.
        """
        self._dims = None  # flag for axis change
        self._scale = scale

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

    def close(self):
        """
        Close the figure.
        """
        self.figure.close()

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

    def connect(self, controller=None):
        """
        Connect the view interface to the callbacks provided by the
        `controller`.
        """
        self.controller = controller
        self.figure.connect(controller=controller)

    def _slices_from_event(self, event):
        slices = {}
        if event.inaxes == self.figure.ax:
            loc = {'x': event.xdata, 'y': event.ydata}
            for dim, axis in zip(self.dims, self.axes):
                # Find limits of hovered *display* pixel
                params = _slice_params(self._data, dim, loc[axis])
                if params is None:
                    return {}
                slices[dim] = params
        return slices

    def toggle_mouse_events(self, active):
        self.figure.toggle_mouse_events(active=active, event_handler=self)

    def handle_motion_notify(self, event):
        self.controller.hover(self._slices_from_event(event))

    def handle_button_press(self, event):
        if self._pick_lock:
            self._pick_lock = False
            return
        if event.button == 1 and not self.figure.toolbar.tool_active:
            self.controller.click(self._slices_from_event(event))

    def handle_pick(self, event):
        if event.mouseevent.button == 1:
            result = self._do_handle_pick(event)
            if result is not None:
                self._pick_lock = True
                self.controller.pick(index=result)

    def rescale_to_data(self, vmin, vmax):
        """
        Forward rescaling to the `figure`.
        """
        self.figure.rescale_to_data(vmin.value, vmax.value)

    def toggle_mask(self, change=None):
        """
        Dummy toggle_mask function.
        """
        return

    def toggle_norm(self, norm, vmin, vmax):
        """
        Forward norm change to the `figure`.
        """
        self.figure.toggle_norm(norm, vmin.value, vmax.value)

    def _update_axes(self):
        """
        Forward axes update to the `figure`.
        """
        self.figure.initialize(
            {axis: self.formatters[dim]
             for axis, dim in zip(self._axes, self._dims)})
        scale = {axis: self._scale[dim] for axis, dim in zip(self._axes, self._dims)}
        self.figure.update_axes(scale=scale, unit=f'[{self._data.unit}]')

    def _make_data(self, new_values, mask_info):
        return new_values

    def refresh(self, mask_info):
        self.figure.update_data(self._make_data(self._data, mask_info))

    def update_data(self, new_values, mask_info=None):
        """
        Forward data update to the `figure`.
        """
        self._data = new_values
        if self._dims != new_values.dims:
            self._dims = new_values.dims
            self._update_axes()
        if self.figure.toolbar is not None:
            self.figure.toolbar.dims = self._dims
        self.refresh(mask_info)

    def set_draw_no_delay(self, *args, **kwargs):
        """
        Forward set_draw_no_delay to the `figure`.
        """
        self.figure.set_draw_no_delay(*args, **kwargs)

    def mark(self, index, color, slices):
        """
        Add a marker (colored scatter point).
        """
        loc = {}
        for ax, (dim, params) in zip(self.axes, slices.items()):
            if isinstance(params, int):
                loc[ax] = self._data.meta[dim][dim, params].value
            else:  # bin edges
                loc[ax] = 0.5 * (params[0].value + params[1].value)
        self._do_mark(index, color, **loc)
