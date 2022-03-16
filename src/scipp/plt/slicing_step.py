# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from functools import partial
from html import escape
from ..utils import value_to_string
from .step import WidgetStep


class SlicingWidget:
    """
    Widgets containing a slider for each of the input's dimensions, as well as
    buttons to modify the currently displayed axes.
    """
    def __init__(self, dims, sizes, ndim):

        import ipywidgets as ipw
        # dims = model.dims
        # sizes = model.sizes
        # self._labels = dim_label_map
        # self._controller = None
        # self._formatters = formatters
        # Dict of controls for each dim, one entry per dim of data
        self._controls = {}
        self._callback = None

        # The container list to hold all widgets
        self.container = []
        # dim_buttons: buttons to control which dimension the slider controls
        self.dim_buttons = {}

        self._slider_dims = dims[:len(dims) - ndim]

        for dim in dims:
            slider = ipw.IntSlider(step=1,
                                   description=dim,
                                   min=0,
                                   max=sizes[dim],
                                   continuous_update=True,
                                   readout=True,
                                   layout={"width": "400px"})
            # slider.observe(self._changed, names="value")

            continuous_update = ipw.Checkbox(value=True,
                                             description="Continuous update",
                                             indent=False,
                                             layout={"width": "20px"})
            ipw.jslink((continuous_update, 'value'), (slider, 'continuous_update'))

            slider_readout = ipw.Label()

            # unit = ipw.Label(value=self._formatters[dim]['unit'],
            #                  layout={"width": "60px"})
            unit = ipw.Label(value='unit', layout={"width": "60px"})

            self._controls[dim] = {
                'continuous': continuous_update,
                'slider': slider,
                'value': slider_readout,
                'unit': unit
            }

        first = True
        for index, dim in enumerate(self._slider_dims):
            row = list(self._controls[dim].values())
            self.container.append(ipw.HBox(row))

    def _ipython_display_(self):
        """
        IPython display representation for Jupyter notebooks.
        """
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        """
        Gather all widgets in a single container box.
        """
        import ipywidgets as ipw
        return ipw.VBox(self.container)

    # def set_callback(self, callback):
    #     self._callback = callback

    # def _slider_moved(self, _):
    #     self._callback()

    def observe(self, callback, **kwargs):
        """
        Get the current range covered by the thick slice.
        """
        for dim in self._controls:
            self._controls[dim]['slider'].observe(callback, **kwargs)

    @property
    def value(self):
        return {dim: self._controls[dim]['slider'].value for dim in self._slider_dims}

    def _changed(self, change):
        change = {"new": self.value}
        super()._changed(change)

        # return {dim: self._controls[dim]['slider'].value for dim in self._slider_dims}

    # def update_slider_readout(self, bounds):
    #     """
    #     Update the slider readout with new slider bounds.
    #     """
    #     for dim in self._slider_dims:

    #         def format(val):
    #             form = self._formatters[dim]['linear']
    #             if form is None:
    #                 return value_to_string(val)
    #             # pos=None causes datetime formatter to return full string
    #             # rather than attempting to set a separate label and returning
    #             # offset
    #             return form(val, pos=None)

    #         if bounds[dim].values.ndim == 0:
    #             bound = f'{format(bounds[dim].value)}'
    #         else:
    #             low, high = bounds[dim].values
    #             bound = f'[{format(low)} {format(high)}]'
    #         self._controls[dim]['value'].value = bound


def _slicing_func(model, slices):
    """
    Slice the data along dimension sliders that are not disabled for all
    entries in the dict of data arrays, and return a dict of 1d value
    arrays for data values, variances, and masks.
    """
    out = model
    for dim, sl in slices.items():
        out = out[dim, sl]
    return out


class SlicingStep(WidgetStep):
    def __init__(self, **kwargs):
        super().__init__(func=_slicing_func, widget=SlicingWidget(**kwargs))


########################################################################################


def smooth(da, radius):
    return da.smoothed(radius)


class SmoothingStep(WidgetStep):
    def __init__(self, **kwargs):
        super().__init__(func=smooth, widget=ipw.FloatSlider(min=0, max=10))


def add_noise(da):
    out = da.copy()
    data = out.values
    noise = np.random.random(data.shape)
    data += noise
    return out


class NoiseStep(Step):
    def __init__(self, **kwargs):
        super().__init__(func=smooth)
