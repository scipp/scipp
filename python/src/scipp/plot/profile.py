# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .. import config
# from .slicer import Slicer
# from .slicer_1d import Slicer1d
from .lineplot import LinePlot
from .tools import parse_params, make_fake_coord, to_bin_edges, to_bin_centers
from .._utils import name_with_unit, value_to_string
from .._scipp import core as sc

# Other imports
import numpy as np
import matplotlib.ticker as ticker
import matplotlib.pyplot as plt
import ipywidgets as widgets
import os


class ProfileView:
    def __init__(self,
                 # controller=None,
                 ax=None,
                 errorbars=None,
                 title=None,
                 unit=None,
                 logx=False,
                 logy=False,
                 mask_params=None,
                 mask_names=None,
                 mpl_line_params=None,
                 grid=False):

        # self.controller = controller

        self.figure = LinePlot(errorbars=errorbars,
                 # masks=masks,
                 ax=ax,
                 mpl_line_params=mpl_line_params,
                 title=title,
                 unit=unit,
                 logx=logx,
                 logy=logy,
                 grid=grid,
                 mask_params=mask_params,
                 mask_names=mask_names,
                 figsize=(config.plot.width / config.plot.dpi,
                         0.6 * config.plot.height / config.plot.dpi))

        self.toggle_view(visible=False)


    def _ipython_display_(self):
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        return self.figure._to_widget()


    def toggle_view(self, visible=True):
        # self.profile_dim = change["owner"].dim
        self.figure.toggle_view(visible=visible)
        self.toggle_hover_visibility(False)

        # if visible:
        #     self.fig.canvas.mpl_connect('motion_notify_event', self.update_profile)
        # else:
        #     i = 1


        # self.layout.display = 'none'
        # if change["new"]:
        #     self.show_profile_view()
        # else:
        #     self.hide_profile_view()
        return



    # def show_profile_view(self):

    #     # Double the figure height
    #     self.fig.set_figheight(2 * self.fig.get_figheight())
    #     # Change the ax geometry so it becomes a subplot
    #     self.ax.change_geometry(2, 1, 1)
    #     # Add lower panel
    #     self.profile_ax = self.fig.add_subplot(212)

    #     # Also need to move the colorbar to the top panel.
    #     # Easiest way to do this is to remove it and create it again.
    #     if self.params["values"][self.name]["cbar"]:
    #         self.cbar.remove()
    #         del self.cbar
    #         self.cbar = plt.colorbar(self.image, ax=self.ax, cax=self.cax)
    #         self.cbar.set_label(name_with_unit(var=self.data_arrays[self.name], name=""))
    #         if self.cax is None:
    #             self.cbar.ax.yaxis.set_label_coords(-1.1, 0.5)
    #         self.members["colorbar"] = self.cbar

    #     # # self.ax_pick.set_ylim([
    #     # #     self.params["values"][self.name]["vmin"],
    #     # #     self.params["values"][self.name]["vmax"]
    #     # # ])
    #     # self.ax_pick.set_ylim(get_ylim(
    #     #         var=self.data_array, errorbars=(self.data_array.variances is not None)))

    #     # Connect picking events
    #     # self.fig.canvas.mpl_connect('pick_event', self.keep_or_delete_profile)
    #     self.fig.canvas.mpl_connect('motion_notify_event', self.update_profile)

    #     return

    def update_axes(self, axparams=None, axformatter=None, axlocator=None, logx=False, logy=False):

        self.figure.update_axes(axparams=axparams, axformatter=axformatter, axlocator=axlocator, logx=logx, logy=logy)
        # # Clear profile axes if present and reset to None
        # del self.profile_viewer
        # if self.profile_ax is not None:
        #     self.profile_ax.clear()
        #     # # ylim = get_ylim(
        #     # #     var=self.data_array, errorbars=(self.data_array.variances is not None))
        #     # self.ax_pick.set_ylim(get_ylim(
        #     #     var=self.data_array, errorbars=(self.data_array.variances is not None)))
        # self.profile_viewer = None
        # if self.profile_scatter is not None:
        #     # self.ax.collections = []
        #     self.fig.canvas.draw_idle()
        #     del self.profile_scatter
        #     self.profile_scatter = None


    def update_data(self, new_values):
        self.figure.update_data(new_values)





    # def compute_profile(self, event):
    #     # Find indices of pixel where cursor lies
    #     # os.write(1, "compute_profile 1\n".encode())
    #     dimx = self.xyrebin["x"].dims[0]
    #     # os.write(1, "compute_profile 1.1\n".encode())
    #     dimy = self.xyrebin["y"].dims[0]
    #     # os.write(1, "compute_profile 1.2\n".encode())
    #     ix = int((event.xdata - self.current_lims["x"][0]) /
    #              (self.xyrebin["x"].values[1] - self.xyrebin["x"].values[0]))
    #     # os.write(1, "compute_profile 1.3\n".encode())
    #     iy = int((event.ydata - self.current_lims["y"][0]) /
    #              (self.xyrebin["y"].values[1] - self.xyrebin["y"].values[0]))
    #     # os.write(1, "compute_profile 2\n".encode())

    #     data_slice = self.data_arrays[self.name]
    #     os.write(1, "compute_profile 3\n".encode())

    #     # Slice along dimensions with active sliders
    #     for dim, val in self.slider.items():
    #         os.write(1, "compute_profile 4\n".encode())
    #         if dim != self.profile_dim:
    #             os.write(1, "compute_profile 5\n".encode())
    #             if dim == dimx:
    #                 os.write(1, "compute_profile 6\n".encode())
    #                 data_slice = self.resample_image(data_slice,
    #                     rebin_edges={dimx: self.xyrebin["x"][dimx, ix:ix + 2]})[dimx, 0]
    #             elif dim == dimy:
    #                 os.write(1, "compute_profile 7\n".encode())
    #                 data_slice = self.resample_image(data_slice,
    #                     rebin_edges={dimy: self.xyrebin["y"][dimy, iy:iy + 2]})[dimy, 0]
    #             else:
    #                 os.write(1, "compute_profile 8\n".encode())
    #                 deltax = self.thickness_slider[dim].value
    #                 data_slice = self.resample_image(data_slice,
    #                     rebin_edges={dim: sc.Variable([dim], values=[val.value - 0.5 * deltax,
    #                                                                  val.value + 0.5 * deltax],
    #                                                         unit=data_slice.coords[dim].unit)})[dim, 0]
    #     os.write(1, "compute_profile 9\n".encode())

    #                 # depth = self.slider_xlims[self.name][dim][dim, 1] - self.slider_xlims[self.name][dim][dim, 0]
    #                 # depth.unit = sc.units.one
    #             # data_slice *= (deltax * sc.units.one)


    #     # # Resample the 3d cube down to a 1d profile
    #     # return self.resample_image(self.da_with_edges,
    #     #                            coord_edges={
    #     #                                dimy: self.da_with_edges.coords[dimy],
    #     #                                dimx: self.da_with_edges.coords[dimx]
    #     #                            },
    #     #                            rebin_edges={
    #     #                                dimy: self.xyrebin["y"][dimy,
    #     #                                                        iy:iy + 2],
    #     #                                dimx: self.xyrebin["x"][dimx, ix:ix + 2]
    #     #                            })[dimy, 0][dimx, 0]
    #     return data_slice

    # def create_profile_plot(self, prof):
    #     # We need to extract the data again and replace with the original
    #     # coordinates, because coordinates have been forced to be bin-edges
    #     # so that rebin could be used. Also reset original unit.
    #     os.write(1, "create_profile_viewer 1\n".encode())
    #     # to_plot = sc.DataArray(data=sc.Variable(dims=prof.dims,
    #     #                                         unit=self.data_arrays[self.name].unit,
    #     #                                         values=prof.values,
    #     #                                         variances=prof.variances))

    #     prof.unit = self.data_arrays[self.name].unit
    #     os.write(1, "create_profile_viewer 1.1\n".encode())
    #     dim = prof.dims[0]
    #     os.write(1, "create_profile_viewer 1.2\n".encode())
    #     if not self.histograms[self.name][dim][dim]:
    #         os.write(1, "create_profile_viewer 1.3\n".encode())
    #         os.write(1, (str(to_bin_centers(prof.coords[dim], dim)) + "\n").encode())
    #         # TODO: there is an issue here when we have non-bin edges
    #         prof.coords[dim] = to_bin_centers(prof.coords[dim], dim)

    #     os.write(1, "create_profile_viewer 2\n".encode())
    #     # for dim in prof.dims:
    #     #     to_plot.coords[dim] = self.slider_coord[self.name][dim]
    #     # if len(prof.masks) > 0:
    #     #     for m in prof.masks:
    #     #         to_plot.masks[m] = prof.masks[m]
    #     # os.write(1, "create_profile_viewer 3\n".encode())
    #     self.profile_viewer = Slicer1d({self.name: prof},
    #                                ax=self.profile_ax,
    #                                logy=self.log,
    #                                mpl_line_params={
    #     "color": {self.name: config.plot.color[0]},
    #     "marker": {self.name: config.plot.marker[0]},
    #     "linestyle": {self.name: config.plot.linestyle[0]},
    #     "linewidth": {self.name: config.plot.linewidth[0]}
    # })
    #     os.write(1, "create_profile_viewer 3\n".encode())
    #     self.profile_key = list(self.profile_viewer.keys())[0]
    #     os.write(1, "create_profile_viewer 4\n".encode())
    #     # if self.flatten_as != "slice":
    #     #     self.ax_pick.set_ylim(self.ylim)
    #     # os.write(1, "create_profile_viewer 5\n".encode())
    #     return prof



    # def check_inaxes(self, event):
    #     inaxes = self.figure.check_inaxes(event)

    # def update_profile(self, event):
    #     os.write(1, "update_profile 1\n".encode())
    #     if event.inaxes == self.ax:
    #         os.write(1, "update_profile 1.5\n".encode())
    #         prof = self.compute_profile(event)
    #         os.write(1, "update_profile 2\n".encode())
    #         if self.profile_viewer is None:
    #             to_plot = self.create_profile_plot(prof)
    #             os.write(1, "update_profile 3\n".encode())

    #             # if self.flatten_as == "slice":

    #             # # Add indicator of range covered by current slice
    #             # dim = to_plot.dims[0]
    #             # xlims = self.ax_pick.get_xlim()
    #             # ylims = self.ax_pick.get_ylim()
    #             # left = to_plot.coords[dim][dim, self.slider[dim].value].value
    #             # if self.histograms[self.name][dim][dim]:
    #             #     width = (
    #             #         to_plot.coords[dim][dim, self.slider[dim].value + 1] -
    #             #         to_plot.coords[dim][dim, self.slider[dim].value]).value
    #             # else:
    #             #     width = 0.01 * (xlims[1] - xlims[0])
    #             #     left -= 0.5 * width
    #             # self.slice_pos_rectangle = Rectangle((left, ylims[0]),
    #             #                                      width,
    #             #                                      ylims[1] - ylims[0],
    #             #                                      facecolor="lightgray",
    #             #                                      zorder=-10)
    #             # self.ax_pick.add_patch(self.slice_pos_rectangle)




    #         # else:
    #         #     # os.write(1, "update_profile 5\n".encode())
    #         #     self.profile_viewer[self.profile_key].update_slice(
    #         #         {"vslice": {
    #         #             self.name: prof
    #         #         }})



    #             # os.write(1, "update_profile 6\n".encode())
    #         # os.write(1, "update_profile 7\n".encode())




    #         self.toggle_visibility_of_hover_plot(True)
    #     elif self.profile_viewer is not None:
    #         self.toggle_visibility_of_hover_plot(False)




        # self.fig.canvas.draw_idle()
        # os.write(1, "update_profile 8\n".encode())

    def toggle_hover_visibility(self, value):
        self.figure.toggle_hover_visibility(value)

    def toggle_visibility_of_hover_plot(self, value):
        return
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
            self.profile_scatter = self.ax.scatter(
                [xdata], [ydata], c=[trace["colorpicker"].value], picker=5)
        else:
            new_offsets = np.concatenate(
                (self.profile_scatter.get_offsets(), [[xdata, ydata]]), axis=0)
            col = np.array(_hex_to_rgb(trace["colorpicker"].value) + [255],
                           dtype=np.float) / 255.0
            new_colors = np.concatenate(
                (self.profile_scatter.get_facecolors(), [col]), axis=0)
            self.profile_scatter.set_offsets(new_offsets)
            self.profile_scatter.set_facecolors(new_colors)
        self.profile_viewer[self.profile_key].keep_trace(trace["button"])

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
        self.profile_viewer[self.profile_key].remove_trace(trace["button"])































# # SPDX-License-Identifier: GPL-3.0-or-later
# # Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# # @author Neil Vaytet

# # Scipp imports
# from .plot import plot
# from .render import render_plot
# from .plot_2d import Slicer2d
# from ..show import _hex_to_rgb
# from .tools import to_bin_edges, parse_params, get_ylim
# from .._utils import name_with_unit
# from .._scipp import core as sc
# from .. import detail

# # Other imports
# import numpy as np
# import matplotlib.pyplot as plt
# from matplotlib.patches import Rectangle
# from matplotlib.collections import PathCollection
# import os

# def profiler(scipp_obj_dict=None,
#              axes=None,
#              masks=None,
#              filename=None,
#              figsize=None,
#              ax=None,
#              cax=None,
#              aspect=None,
#              cmap=None,
#              log=False,
#              vmin=None,
#              vmax=None,
#              color=None,
#              logx=False,
#              logy=False,
#              logxy=False,
#              resolution=None,
#              flatten_as="sum"):
#     """
#     Plot a 2D slice through a N dimensional dataset. For every dimension above
#     2, a slider is created to adjust the position of the slice in that
#     particular dimension.
#     """

#     sv = Profiler(scipp_obj_dict=scipp_obj_dict,
#                   axes=axes,
#                   masks=masks,
#                   ax=ax,
#                   cax=cax,
#                   aspect=aspect,
#                   cmap=cmap,
#                   log=log,
#                   vmin=vmin,
#                   vmax=vmax,
#                   color=color,
#                   logx=logx or logxy,
#                   logy=logy or logxy,
#                   resolution=resolution,
#                   flatten_as=flatten_as)

#     if ax is None:
#         render_plot(figure=sv.fig, widgets=sv.vbox, filename=filename)

#     return sv


# class Profiler(Slicer2d):
#     def __init__(self,
#                  scipp_obj_dict=None,
#                  axes=None,
#                  masks=None,
#                  ax=None,
#                  cax=None,
#                  aspect=None,
#                  cmap=None,
#                  log=None,
#                  vmin=None,
#                  vmax=None,
#                  color=None,
#                  logx=False,
#                  logy=False,
#                  resolution=None,
#                  flatten_as=None):

#         # Variables for profile plotting from pick need to be set before
#         # calling the init from Slicer2d as it calls update_axes() and
#         # update_slice(), which require these variables to be defined.
#         self.profile_viewer = None
#         self.profile_key = None
#         self.slice_pos_rectangle = None
#         self.profile_scatter = None
#         self.profile_update_lock = False
#         self.ax_pick = None
#         self.log = log
#         self.flatten_as = flatten_as
#         self.da_with_edges = None
#         self.vmin = vmin
#         self.vmax = vmax
#         self.ylim = None

#         super().__init__(scipp_obj_dict=scipp_obj_dict,
#                          axes=axes,
#                          masks=masks,
#                          ax=ax,
#                          cax=cax,
#                          aspect=aspect,
#                          cmap=cmap,
#                          log=log,
#                          vmin=vmin,
#                          vmax=vmax,
#                          color=color,
#                          logx=logx,
#                          logy=logy,
#                          resolution=resolution)

#         # Initial checks
#         if self.ndim != 3:
#             raise RuntimeError("Picking on a 2d plot is only supported for 3D "
#                                "data.")

#         # Save a copy of the data array with bin-edge coords
#         self.da_with_edges = self.make_data_array_with_bin_edges()

#         # Double the figure height
#         self.fig.set_figheight(2 * self.fig.get_figheight())
#         # Change the ax geometry so it becomes a subplot
#         self.ax.change_geometry(2, 1, 1)
#         # Add lower panel
#         self.ax_pick = self.fig.add_subplot(212)

#         # Also need to move the colorbar to the top panel.
#         # Easiest way to do this is to remove it and create it again.
#         if self.params["values"][self.name]["cbar"]:
#             self.cbar.remove()
#             del self.cbar
#             self.cbar = plt.colorbar(self.image, ax=self.ax, cax=self.cax)
#             self.cbar.set_label(name_with_unit(var=self.data_array, name=""))
#         if self.cax is None:
#             self.cbar.ax.yaxis.set_label_coords(-1.1, 0.5)
#         self.members["colorbar"] = self.cbar

#         # self.ax_pick.set_ylim([
#         #     self.params["values"][self.name]["vmin"],
#         #     self.params["values"][self.name]["vmax"]
#         # ])
#         self.ax_pick.set_ylim(get_ylim(
#                 var=self.data_array, errorbars=(self.data_array.variances is not None)))

#         # Connect picking events
#         self.fig.canvas.mpl_connect('pick_event', self.keep_or_delete_profile)
#         self.fig.canvas.mpl_connect('motion_notify_event', self.update_profile)

#         return

#     def make_data_array_with_bin_edges(self):
#         da_with_edges = detail.move_to_data_array(
#             data=sc.Variable(dims=self.data_array.dims,
#                              unit=sc.units.counts,
#                              values=self.data_array.values,
#                              variances=self.data_array.variances,
#                              dtype=sc.dtype.float32))
#         for dim, coord in self.slider_coord[self.name].items():
#             if self.histograms[self.name][dim][dim]:
#                 da_with_edges.coords[dim] = coord
#             else:
#                 da_with_edges.coords[dim] = to_bin_edges(coord, dim)
#         if len(self.masks[self.name]) > 0:
#             for m in self.masks[self.name]:
#                 da_with_edges.masks[m] = self.data_array.masks[m]
#         return da_with_edges

#     def update_axes(self):

#         # Run Slicer2d update_axes
#         super().update_axes()

#         # Clear profile axes if present and reset to None
#         del self.profile_viewer
#         if self.ax_pick is not None:
#             self.ax_pick.clear()
#             # ylim = get_ylim(
#             #     var=self.data_array, errorbars=(self.data_array.variances is not None))
#             self.ax_pick.set_ylim(get_ylim(
#                 var=self.data_array, errorbars=(self.data_array.variances is not None)))
#         self.profile_viewer = None
#         if self.profile_scatter is not None:
#             self.ax.collections = []
#             self.fig.canvas.draw_idle()
#             del self.profile_scatter
#             self.profile_scatter = None

#     def slice_data(self):

#         if self.flatten_as == "slice":
#             # Run Slicer2d slice_data
#             super().slice_data()
#             # Update the position of the slice position indicator
#             for dim, val in self.slider.items():
#                 if not val.disabled:
#                     if self.slice_pos_rectangle is not None:
#                         new_pos = self.slider_coord[self.name][dim][
#                             dim, val.value].value
#                         self.slice_pos_rectangle.set_x(new_pos)
#                         if self.histograms[self.name][dim][dim]:
#                             self.slice_pos_rectangle.set_width(self.slider_coord[
#                                 self.name][dim][dim, val.value + 1].value -
#                                                                new_pos)
#                         else:
#                             new_pos -= 0.5 * self.slice_pos_rectangle.get_width()
#                         self.slice_pos_rectangle.set_x(new_pos)
#         # elif self.flatten_as == "sum":
#         else:
#             # if self.da_with_edges is None:
#             #     # Save a copy of the data array with bin-edge coords
#             #     self.da_with_edges = self.make_data_array_with_bin_edges()
#             data_slice = self.da_with_edges
#             selected_dim = None
#             for dim, val in self.slider.items():
#                 if not val.disabled:
#                     selected_dim = dim
#                     break
#                     # # self.vslice = self.vslice[val.dim, val.value]
#                     # data_slice = self.resample_image(data_slice,
#                     #     coord_edges={dim: self.slider_coord[self.name][dim]},
#                     #     rebin_edges={dim: self.slider_xlims[self.name][dim]})[dim, 0]
#                     # depth = self.slider_xlims[self.name][dim][dim, 1] - self.slider_xlims[self.name][dim][dim, 0]
#                     # depth.unit = sc.units.one
#                     # data_slice *= depth
#             data_slice = getattr(sc, self.flatten_as)(self.data_array, selected_dim)
#             # Update the colorbar limits automatically
#             cbar_params = parse_params(globs={"vmin": self.vmin, "vmax": self.vmax},
#                                        variable=data_slice)
#             # self.params["values"][self.name]["norm"] = cbar_params["norm"]
#             self.image.set_norm(cbar_params["norm"])
#             if len(self.masks[self.name]) > 0:
#                 for m in self.masks[self.name]:
#                     self.members["masks"][m].set_norm(cbar_params["norm"])

#             # self.ylim = get_ylim(
#             #     var=data_slice, errorbars=(data_slice.variances is not None))
#             # # # The axes are None on the first pass when the __init__ from
#             # # # Slicer2d is called. In this case, they are updated when the
#             # # # profile_viewer is created.
#             # # if self.ax_pick is not None:
#             # #     self.ax_pick.set_ylim(self.ylim)

#             self.prepare_slice_for_resample(data_slice)

#         # # Update the position of the slice position indicator
#         # for dim, val in self.slider.items():
#         #     if not val.disabled:
#         #         if self.slice_pos_rectangle is not None:
#         #             new_pos = self.slider_coord[self.name][dim][
#         #                 dim, val.value].value
#         #             self.slice_pos_rectangle.set_x(new_pos)
#         #             if self.histograms[self.name][dim][dim]:
#         #                 self.slice_pos_rectangle.set_width(self.slider_coord[
#         #                     self.name][dim][dim, val.value + 1].value -
#         #                                                    new_pos)
#         #             else:
#         #                 new_pos -= 0.5 * self.slice_pos_rectangle.get_width()
#         #             self.slice_pos_rectangle.set_x(new_pos)

#     def toggle_mask(self, change):
#         super().toggle_mask(change)
#         if self.profile_viewer is not None:
#             self.profile_viewer[self.profile_key].masks[self.name][
#                 change["owner"].masks_name].value = change["new"]
#         self.fig.canvas.draw_idle()
#         return

#     def compute_profile(self, event):
#         # Find indices of pixel where cursor lies
#         dimx = self.xyrebin["x"].dims[0]
#         dimy = self.xyrebin["y"].dims[0]
#         ix = int((event.xdata - self.current_lims["x"][0]) /
#                  self.image_pixel_size[dimx])
#         iy = int((event.ydata - self.current_lims["y"][0]) /
#                  self.image_pixel_size[dimy])
#         # Resample the 3d cube down to a 1d profile
#         return self.resample_image(self.da_with_edges,
#                                    coord_edges={
#                                        dimy: self.da_with_edges.coords[dimy],
#                                        dimx: self.da_with_edges.coords[dimx]
#                                    },
#                                    rebin_edges={
#                                        dimy: self.xyrebin["y"][dimy,
#                                                                iy:iy + 2],
#                                        dimx: self.xyrebin["x"][dimx, ix:ix + 2]
#                                    })[dimy, 0][dimx, 0]

#     def create_profile_viewer(self, prof):
#         # We need to extract the data again and replace with the original
#         # coordinates, because coordinates have been forced to be bin-edges
#         # so that rebin could be used. Also reset original unit.
#         # os.write(1, "create_profile_viewer 1\n".encode())
#         to_plot = sc.DataArray(data=sc.Variable(dims=prof.dims,
#                                                 unit=self.data_array.unit,
#                                                 values=prof.values,
#                                                 variances=prof.variances))
#         # os.write(1, "create_profile_viewer 2\n".encode())
#         for dim in prof.dims:
#             to_plot.coords[dim] = self.slider_coord[self.name][dim]
#         if len(prof.masks) > 0:
#             for m in prof.masks:
#                 to_plot.masks[m] = prof.masks[m]
#         # os.write(1, "create_profile_viewer 3\n".encode())
#         self.profile_viewer = plot({self.name: to_plot},
#                                    ax=self.ax_pick,
#                                    logy=self.log)
#         self.profile_key = list(self.profile_viewer.keys())[0]
#         # os.write(1, "create_profile_viewer 4\n".encode())
#         # if self.flatten_as != "slice":
#         #     self.ax_pick.set_ylim(self.ylim)
#         # os.write(1, "create_profile_viewer 5\n".encode())
#         return to_plot

#     def update_profile(self, event):
#         # os.write(1, "update_profile 1\n".encode())
#         if event.inaxes == self.ax:
#             prof = self.compute_profile(event)
#             # os.write(1, "update_profile 2\n".encode())
#             if self.profile_viewer is None:
#                 to_plot = self.create_profile_viewer(prof)
#                 # os.write(1, "update_profile 3\n".encode())

#                 if self.flatten_as == "slice":
#                     # Add indicator of range covered by current slice
#                     dim = to_plot.dims[0]
#                     xlims = self.ax_pick.get_xlim()
#                     ylims = self.ax_pick.get_ylim()
#                     left = to_plot.coords[dim][dim, self.slider[dim].value].value
#                     if self.histograms[self.name][dim][dim]:
#                         width = (
#                             to_plot.coords[dim][dim, self.slider[dim].value + 1] -
#                             to_plot.coords[dim][dim, self.slider[dim].value]).value
#                     else:
#                         width = 0.01 * (xlims[1] - xlims[0])
#                         left -= 0.5 * width
#                     self.slice_pos_rectangle = Rectangle((left, ylims[0]),
#                                                          width,
#                                                          ylims[1] - ylims[0],
#                                                          facecolor="lightgray",
#                                                          zorder=-10)
#                     self.ax_pick.add_patch(self.slice_pos_rectangle)
#                 # os.write(1, "update_profile 4\n".encode())

#             else:
#                 # os.write(1, "update_profile 5\n".encode())
#                 self.profile_viewer[self.profile_key].update_slice(
#                     {"vslice": {
#                         self.name: prof
#                     }})
#                 # os.write(1, "update_profile 6\n".encode())
#             # os.write(1, "update_profile 7\n".encode())
#             self.toggle_visibility_of_hover_plot(True)
#         elif self.profile_viewer is not None:
#             self.toggle_visibility_of_hover_plot(False)
#         # self.fig.canvas.draw_idle()
#         # os.write(1, "update_profile 8\n".encode())

#     def toggle_visibility_of_hover_plot(self, value):
#         # If the mouse moves off the image, we hide the profile. If it moves
#         # back onto the image, we show the profile
#         self.profile_viewer[self.profile_key].members["lines"][
#             self.name].set_visible(value)
#         if self.profile_viewer[self.profile_key].errorbars[self.name]:
#             for item in self.profile_viewer[
#                     self.profile_key].members["error_y"][self.name]:
#                 if item is not None:
#                     for it in item:
#                         it.set_visible(value)
#         mask_dict = self.profile_viewer[self.profile_key].members["masks"][
#             self.name]
#         if len(mask_dict) > 0:
#             for m in mask_dict:
#                 mask_dict[m].set_visible(value if self.profile_viewer[
#                     self.profile_key].masks[self.name][m].value else False)
#                 mask_dict[m].set_gid("onaxes" if value else "offaxes")

#     def keep_or_delete_profile(self, event):
#         if isinstance(event.artist, PathCollection):
#             self.delete_profile(event)
#             self.profile_update_lock = True
#         elif self.profile_update_lock:
#             self.profile_update_lock = False
#         else:
#             self.keep_profile(event)

#     def keep_profile(self, event):
#         trace = list(
#             self.profile_viewer[self.profile_key].keep_buttons.values())[-1]
#         xdata = event.mouseevent.xdata
#         ydata = event.mouseevent.ydata
#         if self.profile_scatter is None:
#             self.profile_scatter = self.ax.scatter(
#                 [xdata], [ydata], c=[trace["colorpicker"].value], picker=5)
#         else:
#             new_offsets = np.concatenate(
#                 (self.profile_scatter.get_offsets(), [[xdata, ydata]]), axis=0)
#             col = np.array(_hex_to_rgb(trace["colorpicker"].value) + [255],
#                            dtype=np.float) / 255.0
#             new_colors = np.concatenate(
#                 (self.profile_scatter.get_facecolors(), [col]), axis=0)
#             self.profile_scatter.set_offsets(new_offsets)
#             self.profile_scatter.set_facecolors(new_colors)
#         self.profile_viewer[self.profile_key].keep_trace(trace["button"])

#     def delete_profile(self, event):
#         ind = event.ind[0]
#         xy = np.delete(self.profile_scatter.get_offsets(), ind, axis=0)
#         c = np.delete(self.profile_scatter.get_facecolors(), ind, axis=0)
#         self.profile_scatter.set_offsets(xy)
#         self.profile_scatter.set_facecolors(c)
#         self.fig.canvas.draw_idle()
#         # Also remove the line from the 1d plot
#         trace = list(
#             self.profile_viewer[self.profile_key].keep_buttons.values())[ind]
#         self.profile_viewer[self.profile_key].remove_trace(trace["button"])
