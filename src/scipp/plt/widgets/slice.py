# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from ... import DataArray
from ...utils import value_to_string
# from .widget import WidgetView
from ..view import View

import ipywidgets as ipw
from typing import Callable


class SliceWidget(View):
    """
    Widgets containing a slider for each of the input's dimensions, as well as
    buttons to modify the currently displayed axes.
    """

    def __init__(self, data_array, dims: list):
        super().__init__()

        self._controls = {}
        self._callback = None
        # The container list to hold all widgets
        self.container = []
        # dim_buttons: buttons to control which dimension the slider controls
        self.dim_buttons = {}

        self._slider_dims = dims

        for dim in dims:
            slider = ipw.IntSlider(step=1,
                                   description=dim,
                                   min=0,
                                   max=data_array.sizes[dim],
                                   continuous_update=True,
                                   readout=True,
                                   layout={"width": "400px"})
            continuous_update = ipw.Checkbox(value=True,
                                             description="Continuous update",
                                             indent=False,
                                             layout={"width": "20px"})
            ipw.jslink((continuous_update, 'value'), (slider, 'continuous_update'))

            slider_readout = ipw.Label()

            self._controls[dim] = {
                'continuous': continuous_update,
                'slider': slider,
                'value': slider_readout
            }

        for index, dim in enumerate(self._slider_dims):
            row = list(self._controls[dim].values())
            self.container.append(ipw.HBox(row))

    def _ipython_display_(self):
        """
        IPython display representation for Jupyter notebooks.
        """
        return self._to_widget()._ipython_display_()

    def _to_widget(self) -> ipw.Widget:
        """
        Gather all widgets in a single container box.
        """
        return ipw.VBox(self.container)

    def observe(self, callback: Callable, **kwargs):
        for dim in self._controls:
            self._controls[dim]['slider'].observe(callback, **kwargs)

    @property
    def value(self) -> dict:
        return {dim: self._controls[dim]['slider'].value for dim in self._slider_dims}

    def update(self, new_coords):
        for dim, c in self._controls.items():
            if dim in new_coords:
                c["value"].value = value_to_string(new_coords[dim].values) + str(
                    new_coords[dim].unit)

    def notify_view(self, message):
        node_id = message["node_id"]
        # node_name = message["node_name"]
        # graph_name = message["graph_name"]
        new_values = self._graph_nodes[node_id].request_data()
        self.update(new_values.meta)


def slice_dims(data_array: DataArray, slices: dict) -> DataArray:
    """
    Slice the data along dimension sliders that are not disabled for all
    entries in the dict of data arrays, and return a dict of 1d value
    arrays for data values, variances, and masks.
    """
    out = data_array
    for dim, sl in slices.items():
        out = out[dim, sl]
    return out


# class SliceView(WidgetView):

#     def __init__(self, *args, **kwargs):
#         super().__init__(widgets={"slices": SliceWidget(*args, **kwargs)})

#     def notify(self, message):
#         node_name = message["node_name"]
#         graph_name = message["graph_name"]
#         new_values = self._graph_nodes[graph_name][node_name].request_data()
#         self._widgets["slices"].update(new_values.meta)
