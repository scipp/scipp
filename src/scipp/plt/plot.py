# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .controller import Controller
from .. import DataArray, Variable
from .mask_step import MaskStep
from .slicing_step import SlicingStep
from .view import View
from .widgets import WidgetCollection

from typing import Dict


class PlotDict():
    """
    The Plot object is used as output for the plot command.
    It is a small wrapper around python dict, with an `_ipython_display_`
    representation.
    The dict will contain one entry for each entry in the input supplied to
    the plot function.
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

    def close(self):
        """
        Close all plots in dict, making them static.
        """
        for item in self.values():
            item.close()


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
                 models: Dict[str, DataArray],
                 view: View = None,
                 vmin: Variable = None,
                 vmax: Variable = None,
                 view_ndims: int = None):

        self._view = view
        self._models = models
        self._view_ndims = view_ndims

        # Shortcut access to the underlying figure for easier modification
        self.fig = None
        self.ax = None

        self._widgets = WidgetCollection()
        self._controller = Controller(models=self._models, view=self._view)

        self._add_default_pipeline_steps()
        self._render()

    def _add_default_pipeline_steps(self):
        # Add step for slicing dimensions out with sliders
        array = next(iter(self._models.values()))
        slicing_step = SlicingStep(dims=array.dims,
                                   sizes=array.sizes,
                                   ndim=self._view_ndims)
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

    def savefig(self, filename: str = None):
        """
        Save plot to file.
        Possible file extensions are `.jpg`, `.png` and `.pdf`.
        The default directory for writing the file is the same as the
        directory where the script or notebook is running.
        """
        self._view.savefig(filename=filename)
