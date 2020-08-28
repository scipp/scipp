# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import config
from .plot import plot
from .render import render_plot
from .plot_2d import Slicer2d
from ..show import _hex_to_rgb
from .tools import to_bin_edges, parse_params
from .._utils import name_with_unit
from .._scipp import core as sc
from .. import detail

# Other imports
import numpy as np
import ipywidgets as widgets
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle
from matplotlib.collections import PathCollection
import warnings


def profiler(scipp_obj_dict=None,
             axes=None,
             masks=None,
             filename=None,
             figsize=None,
             ax=None,
             cax=None,
             aspect=None,
             cmap=None,
             log=False,
             vmin=None,
             vmax=None,
             color=None,
             logx=False,
             logy=False,
             logxy=False,
             resolution=None):
    """
    Plot a 2D slice through a N dimensional dataset. For every dimension above
    2, a slider is created to adjust the position of the slice in that
    particular dimension.
    """

    sv = Profiler(scipp_obj_dict=scipp_obj_dict,
                  axes=axes,
                  masks=masks,
                  ax=ax,
                  cax=cax,
                  aspect=aspect,
                  cmap=cmap,
                  log=log,
                  vmin=vmin,
                  vmax=vmax,
                  color=color,
                  logx=logx or logxy,
                  logy=logy or logxy,
                  resolution=resolution)

    if ax is None:
        render_plot(figure=sv.fig, widgets=sv.vbox, filename=filename)

    return sv


class Profiler(Slicer2d):
    def __init__(self,
                 scipp_obj_dict=None,
                 axes=None,
                 masks=None,
                 ax=None,
                 cax=None,
                 aspect=None,
                 cmap=None,
                 log=None,
                 vmin=None,
                 vmax=None,
                 color=None,
                 logx=False,
                 logy=False,
                 resolution=None):

        # Variables for profile plotting from pick need to be set before
        # calling the init from Slicer2d as it calls update_axes() and
        # update_slice(), which require these variables to be defined.
        self.profile_viewer = None
        self.profile_key = None
        self.slice_pos_rectangle = None
        self.profile_scatter = None
        self.profile_update_lock = False
        self.ax_pick = None

        super().__init__(scipp_obj_dict=scipp_obj_dict,
                         axes=axes,
                         masks=masks,
                         ax=ax,
                         cax=cax,
                         aspect=aspect,
                         cmap=cmap,
                         log=log,
                         vmin=vmin,
                         vmax=vmax,
                         color=color,
                         logx=logx,
                         logy=logy,
                         resolution=resolution)

        # Initial checks
        if self.ndim != 3:
            raise RuntimeError("Picking on a 2d plot is only supported for 3D "
                               "data.")

        # Save a copy of the data array with bin-edge coords
        self.da_with_edges = self.make_data_array_with_bin_edges()

        # Double the figure height
        self.fig.set_figheight(2 * self.fig.get_figheight())
        # Change the ax geometry so it becomes a subplot
        self.ax.change_geometry(2, 1, 1)
        # Add lower panel
        self.ax_pick = self.fig.add_subplot(212)

        # Also need to move the colorbar to the top panel.
        # Easiest way to do this is to remove it and create it again.
        if self.params["values"][self.name]["cbar"]:
            self.cbar.remove()
            del self.cbar
            self.cbar = plt.colorbar(self.image, ax=self.ax, cax=self.cax)
            self.cbar.set_label(name_with_unit(var=self.data_array, name=""))
        if self.cax is None:
            self.cbar.ax.yaxis.set_label_coords(-1.1, 0.5)
        self.members["colorbar"] = self.cbar

        self.ax_pick.set_ylim([
            self.params["values"][self.name]["vmin"],
            self.params["values"][self.name]["vmax"]
        ])

        # Connect picking events
        self.fig.canvas.mpl_connect('pick_event', self.keep_or_delete_profile)
        self.fig.canvas.mpl_connect('motion_notify_event', self.update_profile)

        return

    def make_data_array_with_bin_edges(self):
        da_with_edges = detail.move_to_data_array(
            data=sc.Variable(dims=self.data_array.dims,
                             unit=sc.units.counts,
                             values=self.data_array.values,
                             variances=self.data_array.variances,
                             dtype=sc.dtype.float32))
        for dim, coord in self.slider_coord[self.name].items():
            if self.histograms[self.name][dim][dim]:
                da_with_edges.coords[dim] = coord
            else:
                da_with_edges.coords[dim] = to_bin_edges(coord, dim)
        if len(self.masks[self.name]) > 0:
            for m in self.masks[self.name]:
                da_with_edges.masks[m] = self.data_array.masks[m]
        return da_with_edges

    def update_axes(self):

        # Run Slicer2d update_axes
        super().update_axes()

        # Clear profile axes if present and reset to None
        del self.profile_viewer
        if self.ax_pick is not None:
            self.ax_pick.clear()
        self.profile_viewer = None
        if self.profile_scatter is not None:
            self.ax.collections = []
            self.fig.canvas.draw_idle()
            del self.profile_scatter
            self.profile_scatter = None

    def slice_data(self):

        # Run Slicer2d slice_data
        super().slice_data()

        # Update the position of the slice position indicator
        for dim, val in self.slider.items():
            if not val.disabled:
                if self.slice_pos_rectangle is not None:
                    new_pos = self.slider_coord[self.name][dim][
                        dim, val.value].value
                    self.slice_pos_rectangle.set_x(new_pos)
                    if self.histograms[self.name][dim][dim]:
                        self.slice_pos_rectangle.set_width(self.slider_coord[
                            self.name][dim][dim, val.value + 1].value -
                                                           new_pos)
                    else:
                        new_pos -= 0.5 * self.slice_pos_rectangle.get_width()
                    self.slice_pos_rectangle.set_x(new_pos)

    def toggle_mask(self, change):
        super().toggle_mask(change)
        if self.profile_viewer is not None:
            self.profile_viewer[self.profile_key].masks[self.name][
                change["owner"].masks_name].value = change["new"]
        return

    def compute_profile(self, event):
        # Find indices of pixel where cursor lies
        dimx = self.xyrebin["x"].dims[0]
        dimy = self.xyrebin["y"].dims[0]
        ix = int((event.xdata - self.current_lims["x"][0]) /
                 self.image_pixel_size[dimx])
        iy = int((event.ydata - self.current_lims["y"][0]) /
                 self.image_pixel_size[dimy])
        # Resample the 3d cube down to a 1d profile
        return self.resample_image(self.da_with_edges,
                                   coord_edges={
                                       dimy: self.da_with_edges.coords[dimy],
                                       dimx: self.da_with_edges.coords[dimx]
                                   },
                                   rebin_edges={
                                       dimy: self.xyrebin["y"][dimy,
                                                               iy:iy + 2],
                                       dimx: self.xyrebin["x"][dimx, ix:ix + 2]
                                   })[dimy, 0][dimx, 0]

    def create_profile_viewer(self, prof):
        # We need to extract the data again and replace with the original
        # coordinates, because coordinates have been forced to be bin-edges
        # so that rebin could be used. Also reset original unit.
        to_plot = sc.DataArray(data=sc.Variable(dims=prof.dims,
                                                unit=self.data_array.unit,
                                                values=prof.values,
                                                variances=prof.variances))
        for dim in prof.dims:
            to_plot.coords[dim] = self.slider_coord[self.name][dim]
        if len(prof.masks) > 0:
            for m in prof.masks:
                to_plot.masks[m] = prof.masks[m]
        self.profile_viewer = plot({self.name: to_plot},
                                   ax=self.ax_pick,
                                   logy=self.log)
        self.profile_key = list(self.profile_viewer.keys())[0]
        return to_plot

    def update_profile(self, event):
        if event.inaxes == self.ax:
            prof = self.compute_profile(event)
            if self.profile_viewer is None:
                to_plot = self.create_profile_viewer(prof)

                # Add indicator of range covered by current slice
                dim = to_plot.dims[0]
                xlims = self.ax_pick.get_xlim()
                ylims = self.ax_pick.get_ylim()
                left = to_plot.coords[dim][dim, self.slider[dim].value].value
                if self.histograms[self.name][dim][dim]:
                    width = (
                        to_plot.coords[dim][dim, self.slider[dim].value + 1] -
                        to_plot.coords[dim][dim, self.slider[dim].value]).value
                else:
                    width = 0.01 * (xlims[1] - xlims[0])
                    left -= 0.5 * width
                self.slice_pos_rectangle = Rectangle((left, ylims[0]),
                                                     width,
                                                     ylims[1] - ylims[0],
                                                     facecolor="lightgray",
                                                     zorder=-10)
                self.ax_pick.add_patch(self.slice_pos_rectangle)

            else:
                self.profile_viewer[self.profile_key].update_slice(
                    {"vslice": {
                        self.name: prof
                    }})
            self.toggle_visibility_of_hover_plot(True)
        elif self.profile_viewer is not None:
            self.toggle_visibility_of_hover_plot(False)

    def toggle_visibility_of_hover_plot(self, value):
        # If the mouse moves off the image, we hide the profile. If it moves
        # back onto the image, we show the profile
        self.profile_viewer[self.profile_key].members["lines"][
            self.name].set_visible(value)
        if self.profile_viewer[self.profile_key].errorbars[self.name]:
            for item in self.profile_viewer[
                    self.profile_key].members["error_y"][self.name]:
                if item is not None:
                    for it in item:
                        it.set_visible(value)
        mask_dict = self.profile_viewer[self.profile_key].members["masks"][
            self.name]
        if len(mask_dict) > 0:
            for m in mask_dict:
                mask_dict[m].set_visible(value if self.profile_viewer[
                    self.profile_key].masks[self.name][m].value else False)
                mask_dict[m].set_gid("onaxes" if value else "offaxes")

    def keep_or_delete_profile(self, event):
        if isinstance(event.artist, PathCollection):
            self.delete_profile(event)
            self.profile_update_lock = True
        elif self.profile_update_lock:
            self.profile_update_lock = False
        else:
            self.keep_profile(event)

    def keep_profile(self, event):
        trace = list(
            self.profile_viewer[self.profile_key].keep_buttons.values())[-1]
        xdata = event.mouseevent.xdata
        ydata = event.mouseevent.ydata
        if self.profile_scatter is None:
            self.profile_scatter = self.ax.scatter([xdata], [ydata],
                                                   c=[trace[2].value],
                                                   picker=5)
        else:
            new_offsets = np.concatenate(
                (self.profile_scatter.get_offsets(), [[xdata, ydata]]), axis=0)
            col = np.array(_hex_to_rgb(trace[2].value) + [255],
                           dtype=np.float) / 255.0
            new_colors = np.concatenate(
                (self.profile_scatter.get_facecolors(), [col]), axis=0)
            self.profile_scatter.set_offsets(new_offsets)
            self.profile_scatter.set_facecolors(new_colors)
        self.profile_viewer[self.profile_key].keep_trace(trace[1])

    def delete_profile(self, event):
        ind = event.ind[0]
        xy = np.delete(self.profile_scatter.get_offsets(), ind, axis=0)
        c = np.delete(self.profile_scatter.get_facecolors(), ind, axis=0)
        self.profile_scatter.set_offsets(xy)
        self.profile_scatter.set_facecolors(c)
        self.fig.canvas.draw_idle()
        # Also remove the line from the 1d plot
        trace = list(
            self.profile_viewer[self.profile_key].keep_buttons.values())[ind]
        self.profile_viewer[self.profile_key].remove_trace(trace[1])
