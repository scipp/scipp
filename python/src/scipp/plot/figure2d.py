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


class PlotFigure2d:
    def __init__(self,
                 ax=None,
                 cax=None,
                 figsize=None,
                 aspect=None,
                 cmap=None,
                 norm=None,
                 title=None,
                 cbar=None,
                 unit=None,
                 log=None,
                 vmin=None,
                 vmax=None,
                 color=None,
                 # logx=False,
                 # logy=False,
                 mask_cmap=None,
                 masks=None,
                 resolution=None):

        # Get matplotlib figure and axes
        self.fig, self.ax, self.cax, self.own_axes = get_mpl_axes(
            ax=ax, cax=cax, figsize=figsize)

        # Save aspect ratio setting
        if aspect is None:
            aspect = config.plot.aspect

        self.image = self.make_default_imshow(cmap=cmap,
                                              norm=norm,
                                              aspect=aspect,
                                              picker=5)
        self.ax.set_title(title)
        self.cbar = None
        if cbar:
            self.cbar = plt.colorbar(self.image, ax=self.ax, cax=self.cax)
            self.cbar.set_label(unit)
        if self.cax is None:
            self.cbar.ax.yaxis.set_label_coords(-1.1, 0.5)
        self.mask_image = {}
        for m in masks:
            self.mask_image[m] = self.make_default_imshow(cmap=mask_cmap,
                                                          norm=norm,
                                                          aspect=aspect)
        # self.logx = logx
        # self.logy = logy
        # if self.logx:
        #     self.ax.set_xscale("log")
        # if self.logy:
        #     self.ax.set_yscale("log")

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

    def savefig(self, filename=None):
        self.fig.savefig(filename, bbox_inches="tight")

    def make_default_imshow(self, cmap, norm, aspect=None, picker=None):
        return self.ax.imshow([[1.0, 1.0], [1.0, 1.0]],
                              norm=norm,
                              extent=[1, 2, 1, 2],
                              origin="lower",
                              aspect=aspect,
                              interpolation="nearest",
                              cmap=cmap,
                              picker=picker)

    def rescale_to_data(self, vmin, vmax):
        self.image.set_clim([vmin, vmax])
        for m, im in self.mask_image.items():
            im.set_clim([vmin, vmax])
        self.fig.canvas.draw_idle()

    def toggle_mask(self, mask_name, visible):
        im = self.mask_image[mask_name]
        if im.get_url() != "hide":
            im.set_visible(visible)
        self.fig.canvas.draw_idle()

    def reset_home_button(self, axparams):
        # Some annoying house-keeping when using X/Y buttons: we need to update
        # the deeply embedded limits set by the Home button in the matplotlib
        # toolbar. The home button actually brings the first element in the
        # navigation stack to the top, so we need to modify the first element
        # in the navigation stack in-place.
        if self.fig is not None:
            if self.fig.canvas.toolbar is not None:
                if len(self.fig.canvas.toolbar._nav_stack._elements) > 0:
                    # Get the first key in the navigation stack
                    key = list(self.fig.canvas.toolbar._nav_stack._elements[0].
                               keys())[0]
                    # Construct a new tuple for replacement
                    alist = []
                    for x in self.fig.canvas.toolbar._nav_stack._elements[0][
                            key]:
                        alist.append(x)
                    alist[0] = (*axparams["x"]["lims"], *axparams["y"]["lims"])
                    # Insert the new tuple
                    self.fig.canvas.toolbar._nav_stack._elements[0][
                        key] = tuple(alist)

    def update_axes(self, axparams, axformatter, axlocator):
      # , logx, logy):

        # is_log = {"x": self.logx, "y": self.logy}

        # Set axes labels
        self.ax.set_xlabel(axparams["x"]["label"])
        self.ax.set_ylabel(axparams["y"]["label"])
        self.ax.set_xscale("log" is axparams["x"]["log"] else "linear")
        self.ax.set_yscale("log" is axparams["y"]["log"] else "linear")

        for xy, param in axparams.items():
            axis = getattr(self.ax, "{}axis".format(xy))
            axis.set_major_formatter(axformatter[param["dim"]][param["log"]])
            axis.set_major_locator(axlocator[param["dim"]][param["log"]])

        # Set axes limits and ticks
        extent_array = np.array([axparams["x"]["lims"],
                                 axparams["y"]["lims"]]).flatten()
        with warnings.catch_warnings():
            warnings.filterwarnings("ignore", category=UserWarning)
            self.image.set_extent(extent_array)
            for m, im in self.mask_image.items():
                im.set_extent(extent_array)
            self.ax.set_xlim(axparams["x"]["lims"])
            self.ax.set_ylim(axparams["y"]["lims"])

        # self.reset_profile()
        self.reset_home_button(axparams)

    def update_data(self, new_values, info=None):
        self.image.set_data(new_values["values"])
        if new_values["extent"] is not None:
            self.image.set_extent(new_values["extent"])
        for m in self.mask_image:
            if new_values["masks"][m] is not None:
                self.mask_image[m].set_data(new_values["masks"][m])
            else:
                self.mask_image[m].set_visible(False)
                self.mask_image[m].set_url("hide")
            if new_values["extent"] is not None:
                self.mask_image[m].set_extent(new_values["extent"])
        self.fig.canvas.draw_idle()
