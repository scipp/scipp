# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet
from matplotlib.lines import Line2D

from .view import PlotView
from .tools import vars_to_err
from ..core import ones, units, Variable


def _make_label(array):
    # TODO use formatter
    labels = []
    for dim, coord in array.meta.items():
        unit = '' if coord.unit == units.dimensionless else f' {coord.unit}'
        if dim not in array.dims:
            labels.append(f'{dim}={coord.values.round(decimals=2)}{unit}')
    return ', '.join(labels)


class PlotView1d(PlotView):
    """
    View object for 1 dimensional plots. Contains a `PlotFigure1d`.

    The difference between `PlotView1d` and `PlotFigure1d` is that `PlotView1d`
    also handles the communications with the `PlotController` that are to do
    with the `PlotProfile` plot displayed below the `PlotFigure1d`.

    """
    def __init__(self, figure, formatters):
        super().__init__(figure=figure, formatters=formatters)
        self._axes = ['x']

    def _make_masks(self, array, mask_info):
        if not mask_info:
            return {}
        masks = {}
        data = array.data
        base_mask = ones(sizes=data.sizes, dtype='int32')
        for m in mask_info:
            if m in array.masks:
                msk = base_mask * Variable(dims=array.masks[m].dims,
                                           values=array.masks[m].values)
                masks[m] = msk.values
            else:
                masks[m] = None
        return masks

    def _make_data(self, new_values, mask_info):
        out = {}
        for name, array in new_values.items():
            self._dim = array.dims[0]  # should be same for all items
            values = {"values": {}, "variances": {}, "masks": {}}
            values['label'] = _make_label(array)
            values["values"]["x"] = array.meta[self._dim].values.ravel()
            values["values"]["y"] = array.values.ravel()
            if array.variances is not None:
                values["variances"]["e"] = vars_to_err(array.variances.ravel())
            values["masks"] = self._make_masks(array, mask_info=mask_info[name])
            out[name] = values
        return out

    def toggle_mask(self, change):
        """
        Forward mask toggling to the `figure`.
        """
        self.figure.toggle_mask(change["owner"].mask_group, change["owner"].mask_name,
                                change["new"])

    def keep_line(self, *args, **kwargs):
        """
        Forward keep line event to the `figure`.
        """
        self.figure.keep_line(*args, **kwargs)

    def remove_line(self, *args, **kwargs):
        """
        Forward remove line event to the `figure`.
        """
        self.figure.remove_line(*args, **kwargs)

    def update_line_color(self, *args, **kwargs):
        """
        Forward line color update to the `figure`.
        """
        self.figure.update_line_color(*args, **kwargs)

    def clear_marks(self):
        """
        Remove all markers (axvline).
        """
        self.figure.ax.lines = [
            line for line in self.figure.ax.lines if isinstance(line.get_gid(), int)
        ]
        self.figure.draw()

    def _do_handle_pick(self, event):
        """
        Return the index of the picked line, None if something else is picked.
        """
        if isinstance(event.artist, Line2D):
            # Avoid matching data or mask lines
            if isinstance(event.artist.get_gid(), int):
                return event.artist.get_gid()

    def _do_mark(self, index, color, x):
        """
        Add a marker (axvline).
        """
        line = self.figure.ax.axvline(x, color=color, picker=True)
        line.set_pickradius(5.0)
        line.set_gid(index)
        self.figure.draw()

    def remove_mark(self, index):
        """
        Remove a marker (axvline).
        """
        self.figure.ax.lines = [
            line for line in self.figure.ax.lines if line.get_gid() != index
        ]
        self.figure.draw()
