# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .... import DataArray
from ....utils import value_to_string
from ..view import View
from ..model import node

import ipywidgets as ipw
from typing import Callable


class SliceView(View):

    def __init__(self, dims, *nodes):
        super().__init__(*nodes)
        self._labels = {dim: ipw.Label() for dim in dims}

    def _to_widget(self) -> ipw.Widget:
        self.render()
        return ipw.VBox(list(self._labels.values()))

    def _update(self, new_coords):
        for dim, lab in self._labels.items():
            if dim in new_coords:
                lab.value = value_to_string(new_coords[dim].values) + str(
                    new_coords[dim].unit)

    def notify_view(self, message):
        node_id = message["node_id"]
        new_values = self._graph_nodes[node_id].request_data()
        self._update(new_values.meta)

    def render(self):
        for n in self._graph_nodes.values():
            new_values = n.request_data()
            self._update(new_coords=new_values.meta)


class SliceWidget:
    """
    Widgets containing a slider for each of the input's dimensions, as well as
    buttons to modify the currently displayed axes.
    """

    def __init__(self, data_array, dims: list):

        self._container = []
        self._slider_dims = dims
        self.controls = {}
        self.view = None

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

            self.controls[dim] = {'continuous': continuous_update, 'slider': slider}

        for index, dim in enumerate(self._slider_dims):
            row = list(self.controls[dim].values())
            self._container.append(ipw.HBox(row))

    def _ipython_display_(self):
        """
        IPython display representation for Jupyter notebooks.
        """
        return self._to_widget()._ipython_display_()

    def _to_widget(self) -> ipw.Widget:
        """
        Gather all widgets in a single container box.
        """
        out = ipw.VBox(self._container)
        if self.view is not None:
            out = ipw.HBox([out, self.view._to_widget()])
        return out

    def observe(self, callback: Callable, **kwargs):
        for dim in self.controls:
            self.controls[dim]['slider'].observe(callback, **kwargs)

    @property
    def value(self) -> dict:
        return {dim: self.controls[dim]['slider'].value for dim in self._slider_dims}

    def make_view(self, *nodes):
        self.view = SliceView(self._slider_dims, *nodes)


@node
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
