# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import config
from .figure import get_mpl_axes
from .._utils import make_random_color

# Other imports
import numpy as np
import ipywidgets as ipw
import matplotlib.pyplot as plt
from matplotlib.collections import PathCollection
import warnings
import io


class PlotView2d:
    def __init__(self,
                 controller=None,
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
                 logx=False,
                 logy=False,
                 mask_cmap=None,
                 masks=None,
                 resolution=None):

        self.controller = controller

        self.xlim_updated = False
        self.ylim_updated = False
        self.current_lims = {"x": np.zeros(2), "y": np.zeros(2)}
        self.global_lims = {"x": np.zeros(2), "y": np.zeros(2)}

        self.profile_hover_connection = None
        self.profile_pick_connection = None
        self.profile_update_lock = False
        self.profile_scatter = None
        self.profile_counter = -1
        self.profile_ids = []

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
        if logx:
            self.ax.set_xscale("log")
        if logy:
            self.ax.set_yscale("log")
        if self.own_axes:
            self.fig.tight_layout(rect=config.plot.padding)

        # Connect changes in axes limits to resampling function
        self.ax.callbacks.connect('xlim_changed', self.check_for_xlim_update)
        self.ax.callbacks.connect('ylim_changed', self.check_for_ylim_update)

        return

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

    def toggle_mask(self, change):
        im = self.mask_image[change["owner"].mask_name]
        if im.get_url() != "hide":
            im.set_visible(change["new"])
        self.fig.canvas.draw_idle()

    def check_for_xlim_update(self, event_ax):
        self.xlim_updated = True
        if self.ylim_updated:
            self.update_bins_from_axes_limits()

    def check_for_ylim_update(self, event_ax):
        self.ylim_updated = True
        if self.xlim_updated:
            self.update_bins_from_axes_limits()

    def update_bins_from_axes_limits(self):
        """
        Update the axis limits and resample the image according to new viewport
        """
        self.xlim_updated = False
        self.ylim_updated = False
        xylims = {}

        # Make sure we don't overrun the original array bounds
        xylims["x"] = np.clip(self.ax.get_xlim(),
                              *sorted(self.global_lims["x"]))
        xylims["y"] = np.clip(self.ax.get_ylim(),
                              *sorted(self.global_lims["y"]))

        dx = np.abs(self.current_lims["x"][1] - self.current_lims["x"][0])
        dy = np.abs(self.current_lims["y"][1] - self.current_lims["y"][0])
        diffx = np.abs(self.current_lims["x"] - xylims["x"]) / dx
        diffy = np.abs(self.current_lims["y"] - xylims["y"]) / dy
        diff = diffx.sum() + diffy.sum()

        # Only resample image if the changes in axes limits are large enough to
        # avoid too many updates while panning.
        if diff > 0.1:
            self.current_lims = xylims
            self.controller.update_viewport(xylims)

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

    def update_axes(self, axparams, axformatter, axlocator, logx, logy):

        self.current_lims['x'] = axparams["x"]["lims"]
        self.current_lims['y'] = axparams["y"]["lims"]
        self.global_lims["x"] = axparams["x"]["lims"]
        self.global_lims["y"] = axparams["y"]["lims"]

        is_log = {"x": logx, "y": logy}

        # Set axes labels
        self.ax.set_xlabel(axparams["x"]["label"])
        self.ax.set_ylabel(axparams["y"]["label"])

        for xy, param in axparams.items():
            axis = getattr(self.ax, "{}axis".format(xy))
            axis.set_major_formatter(axformatter[param["dim"]][is_log[xy]])
            axis.set_major_locator(axlocator[param["dim"]][is_log[xy]])

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

        self.reset_profile()
        self.reset_home_button(axparams)

    def update_data(self, new_values):
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

    def reset_profile(self):
        if self.profile_scatter is not None:
            self.profile_scatter = None
            self.ax.collections = []
            self.fig.canvas.draw_idle()

    def update_profile(self, event):
        if event.inaxes == self.ax:
            xdata = event.xdata - self.current_lims["x"][0]
            ydata = event.ydata - self.current_lims["y"][0]
            self.controller.update_profile(xdata, ydata)
            self.controller.toggle_hover_visibility(True)
        else:
            self.controller.toggle_hover_visibility(False)

    def keep_or_remove_profile(self, event):
        if isinstance(event.artist, PathCollection):
            self.remove_profile(event)
            # We need a profile lock to catch the second time the function is
            # called because the pick event is registed by both the scatter
            # points and the image
            self.profile_update_lock = True
        elif self.profile_update_lock:
            self.profile_update_lock = False
        else:
            self.keep_profile(event)
        self.fig.canvas.draw_idle()

    def update_profile_connection(self, visible):
        # Connect picking events
        if visible:
            self.profile_pick_connection = self.fig.canvas.mpl_connect(
                'pick_event', self.keep_or_remove_profile)
            self.profile_hover_connection = self.fig.canvas.mpl_connect(
                'motion_notify_event', self.update_profile)
        else:
            if self.profile_pick_connection is not None:
                self.fig.canvas.mpl_disconnect(self.profile_pick_connection)
            if self.profile_hover_connection is not None:
                self.fig.canvas.mpl_disconnect(self.profile_hover_connection)

    def keep_profile(self, event):
        xdata = event.mouseevent.xdata
        ydata = event.mouseevent.ydata
        col = make_random_color(fmt='rgba')
        self.profile_counter += 1
        line_id = self.profile_counter
        self.profile_ids.append(line_id)
        if self.profile_scatter is None:
            self.profile_scatter = self.ax.scatter([xdata], [ydata],
                                                   c=[col],
                                                   picker=5)
        else:
            new_offsets = np.concatenate(
                (self.profile_scatter.get_offsets(), [[xdata, ydata]]), axis=0)
            new_colors = np.concatenate(
                (self.profile_scatter.get_facecolors(), [col]), axis=0)
            self.profile_scatter.set_offsets(new_offsets)
            self.profile_scatter.set_facecolors(new_colors)

        self.controller.keep_line(target="profile", color=col, line_id=line_id)

    def remove_profile(self, event):
        ind = event.ind[0]
        xy = np.delete(self.profile_scatter.get_offsets(), ind, axis=0)
        c = np.delete(self.profile_scatter.get_facecolors(), ind, axis=0)
        self.profile_scatter.set_offsets(xy)
        self.profile_scatter.set_facecolors(c)
        # Also remove the line from the 1d plot
        self.controller.remove_line(target="profile",
                                    line_id=self.profile_ids[ind])
        self.profile_ids.pop(ind)
