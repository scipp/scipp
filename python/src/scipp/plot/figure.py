# Scipp imports
from .. import config
from .tools import get_mpl_axes

# Other imports
import numpy as np
import ipywidgets as ipw
import matplotlib.pyplot as plt
import warnings
import io
# from copy import deepcopy


class PlotFigure:
    def __init__(self,
                 ax=None,
                 cax=None,
                 figsize=None,
                 title=None,
                 padding=None):

        """
            Return matplotlib figure and axes
        """
        self.fig = None
        self.ax = ax
        self.cax = cax
        self.own_axes = True
        if self.ax is None:
            if figsize is None:
                figsize = (config.plot.width / config.plot.dpi,
                           config.plot.height / config.plot.dpi)
            self.fig, self.ax = plt.subplots(1, 1, figsize=figsize, dpi=config.plot.dpi)
            if padding is None:
                padding = config.plot.padding
            self.fig.tight_layout(rect=padding)
        else:
            self.own_axes = False
            self.fig = self.ax.get_figure()

        self.ax.set_title(title)

        self.axformatter = {}
        self.axlocator = {}

    def _ipython_display_(self):
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        if hasattr(self.fig.canvas, "widgets"):
            return self.fig.canvas
        else:
            buf = io.BytesIO()
            self.fig.savefig(buf, format='png')
            buf.seek(0)
            return ipw.Image(value=buf.getvalue(),
                             width=config.plot.width,
                             height=config.plot.height)

    def initialise(self, axformatters=None):
        for dim in axformatters:
            self.axformatter[dim] = {}
            for key in [False, True]:
                if axformatters[dim][key] is None:
                    self.axformatter[dim][key] = ticker.ScalarFormatter()
                else:
                    self.axformatter[dim][key] = ticker.FuncFormatter(axformatters[dim][key])
            self.axlocator[dim] = {False: ticker.AutoLocator(), True: ticker.LogLocator()}
            if axformatters[dim]["custom_locator"]:
                self.axlocator[dim][False] = ticker.MaxNLocator(integer=True)

