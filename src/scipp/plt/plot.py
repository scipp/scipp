# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .. import config, units
# from .formatters import make_formatter
from .params import make_params
from ..core import DimensionError
# from .model1d import PlotModel1d
from .widgets import WidgetCollection
# from .resampling_model import ResamplingMode
# from .preprocessors import make_default_preprocessors
from .slicing_step import SlicingStep
from .mask_step import MaskStep

# def _make_errorbar_params(arrays, errorbars):
#     """
#     Determine whether error bars should be plotted or not.
#     """
#     if errorbars is None:
#         params = {}
#     else:
#         if isinstance(errorbars, bool):
#             params = {name: errorbars for name in arrays}
#         elif isinstance(errorbars, dict):
#             params = errorbars
#         else:
#             raise TypeError("Unsupported type for argument "
#                             "'errorbars': {}".format(type(errorbars)))
#     for name, array in arrays.items():
#         has_variances = array.variances is not None
#         if name in params:
#             params[name] &= has_variances
#         else:
#             params[name] = has_variances
#     return params

# def _make_formatters(*, dims, arrays, labels):
#     array = next(iter(arrays.values()))
#     labs = {dim: dim for dim in dims}
#     if labels is not None:
#         labs.update(labels)
#     # formatters = {dim: make_formatter(array, labs[dim], dim) for dim in dims}
#     formatters = {}
#     return labs, formatters

# def make_profile(ax, mask_color):
#     from .profile import PlotProfile
#     cfg = config['plot']
#     bbox = list(cfg['bounding_box'])
#     bbox[2] = 0.77
#     return PlotProfile(ax=ax,
#                        mask_color=mask_color,
#                        figsize=(1.3 * cfg['width'] / cfg['dpi'],
#                                 0.6 * cfg['height'] / cfg['dpi']),
#                        bounding_box=bbox,
#                        legend={
#                            "show": True,
#                            "loc": (1.02, 0.0)
#                        })


class PlotDict():
    """
    The Plot object is used as output for the plot command.
    It is a small wrapper around python dict, with an `_ipython_display_`
    representation.
    The dict will contain one entry for each entry in the input supplied to
    the plot function.
    More functionalities can be added in the future.
    """
    def __init__(self, *args, **kwargs):
        self._items = dict(*args, **kwargs)

    def __getitem__(self, key):
        return self._items[key]

    def __len__(self):
        return len(self._items)

    def keys(self):
        return self._items.keys()

    def values(self):
        return self._items.values()

    def items(self):
        return self._items.items()

    def _ipython_display_(self):
        """
        IPython display representation for Jupyter notebooks.
        """
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        """
        Return plot contents into a single VBocx container
        """
        import ipywidgets as ipw
        contents = []
        for item in self.values():
            if item is not None:
                contents.append(item._to_widget())
        return ipw.VBox(contents)

    def show(self):
        """
        """
        for item in self.values():
            item.show()

    def hide_widgets(self):
        for item in self.values():
            item.hide_widgets()

    def close(self):
        """
        Close all plots in dict, making them static.
        """
        for item in self.values():
            item.close()

    def redraw(self):
        """
        Redraw/update  all plots in dict.
        """
        for item in self.values():
            item.redraw()

    def set_draw_no_delay(self, value):
        """
        When set to True, try to update plots as soon as possible.
        This is useful in the case where one wishes to update the plot inside
        a loop (e.g. when listening to a data stream).
        The plot update is then slightly more expensive than when it is set to
        False.
        """
        for item in self.values():
            item.set_draw_no_delay(value)


class Plot:
    """
    Base class for plot objects. It uses the Model-View-Controller pattern to
    separate displayed figures and user-interaction via widgets from the
    operations performed on the data.

    It contains:
      - a `PlotModel`: contains the input data and performs all the heavy
          calculations.
      - a `PlotView`: contains a `PlotFigure` which is displayed and handles
          communications between `PlotController` and `PlotFigure`, as well as
          updating the `PlotProfile` depending on signals captured by the
          `PlotFigure`.
      - some `PlotWidgets`: a base collection of sliders and buttons which
          provide interactivity to the user.
      - a `PlotPanel` (optional): an extra set of widgets which is not part of
          the base `PlotWidgets`.
      - a `PlotProfile` (optional): used to display a profile plot under the
          `PlotFigure` to show one of the slider dimensions as a 1 dimensional
          line plot.
      - a `PlotController`: handles all the communication between all the
          pieces above.
    """
    def __init__(self,
                 models,
                 controller,
                 profile_figure=None,
                 errorbars=None,
                 panel=None,
                 labels=None,
                 resolution=None,
                 dims=None,
                 view=None,
                 vmin=None,
                 vmax=None,
                 axes=None,
                 scale=None,
                 view_ndims=None):

        self._view = view
        self._models = models
        self.view_ndims = view_ndims

        # Shortcut access to the underlying figure for easier modification
        self.fig = None
        self.ax = None

        self._widgets = WidgetCollection()
        self._controller = controller(models=self._models, view=self._view)

        self._add_default_pipeline_steps()
        self._render()

    def _add_default_pipeline_steps(self):
        # Add step for slicing dimensions out with sliders
        array = next(iter(self._models.values()))
        slicing_step = SlicingStep(dims=array.dims, sizes=array.sizes, ndim=view_ndims)
        self._controller.add_pipeline_step(slicing_step)
        self._widgets.append(slicing_step)

        for key, model in self._models.items():
            mask_step = MaskStep(masks=model.masks, name=key)
            self._controller.add_pipeline_step(key=key, step=mask_step)
            self._widgets.append(mask_step)

    def _ipython_display_(self):
        """
        IPython display representation for Jupyter notebooks.
        """
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        """
        Get the SciPlot object as an `ipywidget`.
        """
        import ipywidgets as ipw
        return ipw.VBox([self._view._to_widget(), self._widgets._to_widget()])

    def close(self):
        """
        Send close signal to the view.
        """
        self._view.close()

    def show(self):
        """
        Call the show() method of a matplotlib figure.
        """
        self._view.show()

    def _render(self):
        """
        Perform some initial calls to render the figure once all components
        have been created.
        """
        self._controller.render()
        self.fig = getattr(self._view, "fig", None)
        self.ax = getattr(self._view, "ax", None)

    def savefig(self, filename=None):
        """
        Save plot to file.
        Possible file extensions are `.jpg`, `.png` and `.pdf`.
        The default directory for writing the file is the same as the
        directory where the script or notebook is running.
        """
        self.view.savefig(filename=filename)
