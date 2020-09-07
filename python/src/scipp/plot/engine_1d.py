# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import config
from .engine import PlotEngine
from .lineplot import LinePlot
from .render import render_plot
from .slicer import Slicer
from .tools import to_bin_centers, vars_to_err
from .._utils import name_with_unit
from .._scipp import core as sc

# Other imports
import numpy as np
import copy as cp
import matplotlib.pyplot as plt
import ipywidgets as widgets
import warnings





class PlotEngine1d(PlotEngine):

    def __init__(self,
                 parent=None,
                 scipp_obj_dict=None,
                 axes=None,
                 masks=None,
                 cmap=None,
                 log=None,
                 vmin=None,
                 vmax=None,
                 color=None):

        super().__init__(parent=parent,
                         scipp_obj_dict=scipp_obj_dict,
                         axes=axes,
                         masks=masks,
                         cmap=cmap,
                         log=log,
                         vmin=vmin,
                         vmax=vmax,
                         color=color)

        self.axparams = {"x": {}, "y": {}}
        self.button_dims = [None, None]
        self.dim_to_xy = {}
        self.xyrebin = {}
        self.xywidth = {}
        self.image_pixel_size = {}

        return




    def update_buttons(self, owner, event, dummy):
        for dim, button in self.buttons.items():
            if dim == owner.dim:
                self.slider[dim].disabled = True
                button.disabled = True
                self.button_axis_to_dim["x"] = dim
            else:
                self.slider[dim].disabled = False
                button.value = None
                button.disabled = False
        self.update_axes(owner.dim)
        self.parent.clear_keep_buttons()
        # self.keep_buttons = dict()
        self.parent.make_keep_button()
        # self.update_button_box_widget()
        return

    # def update_button_box_widget(self):
    #     self.mbox = self.vbox.copy()
    #     for k, b in self.keep_buttons.items():
    #         self.mbox.append(widgets.HBox(list(b.values())))
    #     self.box.children = tuple(self.mbox)

    def update_axes(self, dim):
        # if not self.mpl_axes:
        #     self.ax.lines = []
        #     self.ax.collections = []
        # self.members.update({
        #     "lines": {},
        #     "error_x": {},
        #     "error_y": {},
        #     "error_xy": {},
        #     "masks": {}
        # })

        # xmin = np.Inf
        # xmax = np.NINF
        to_line_plot = {}
        is_bin_edge = {}
        for name, array in self.data_arrays.items():
            # # new_x = self.slider_coord[name][dim].values
            # # new_x = array.coords[dim].values
            # xmin = min(sc.min(array.coords[dim]).value, xmin)
            # xmax = max(sc.max(array.coords[dim]).value, xmax)

            vslice = self.slice_data(array, name)
            to_line_plot[name] = vslice
            # dim = vslice.dims[0]
            is_bin_edge[name] = self.histograms[name][dim][dim]

        if self.parent.figure is None:
            self.parent.figure = LinePlot(to_line_plot, is_bin_edge=is_bin_edge,
                mpl_line_params=self.parent.mpl_line_params)
        else:
            self.parent.figure.plot_data(to_line_plot, is_bin_edge=is_bin_edge, clear=True)

        #     new_data_arrays
        #     ydata = vslice.values
        #     xcenters = to_bin_centers(vslice.coords[dim], dim).values

        #     if len(self.masks[name]) > 0:
        #         self.members["masks"][name] = {}
        #         base_mask = sc.Variable(dims=vslice.dims,
        #                                 values=np.ones(vslice.shape,
        #                                                dtype=np.int32))

        #     # If this is a histogram, plot a step function
        #     if self.histograms[name][dim][dim]:
        #         ye = np.concatenate((ydata[0:1], ydata))
        #         [self.members["lines"][name]
        #          ] = self.ax.step(vslice.coords[dim].values,
        #                           ye,
        #                           label=name,
        #                           zorder=10,
        #                           **{
        #                               key: self.mpl_line_params[key][name]
        #                               for key in ["color", "linewidth"]
        #                           })
        #         # Add masks if any
        #         if len(self.masks[name]) > 0:
        #             for m in self.masks[name]:
        #                 mdata = (
        #                     base_mask *
        #                     sc.Variable(dims=vslice.masks[m].dims,
        #                                 values=vslice.masks[m].values.astype(
        #                                     np.int32))).values

        #                 me = np.concatenate((mdata[0:1], mdata))
        #                 [self.members["masks"][name][m]] = self.ax.step(
        #                     vslice.coords[dim].values,
        #                     self.mask_to_float(me, ye),
        #                     linewidth=self.mpl_line_params["linewidth"][name] *
        #                     3.0,
        #                     color=self.params["masks"][name]["color"],
        #                     zorder=9)
        #                 # Abuse a mostly unused property `gid` of Line2D to
        #                 # identify the line as a mask. We set gid to `onaxes`.
        #                 # This is used by the profile viewer in the 2D plotter
        #                 # to know whether to show the mask or not, depending on
        #                 # whether the cursor is hovering over the 2D image or
        #                 # not.
        #                 self.members["masks"][name][m].set_gid("onaxes")

        #     else:

        #         # If this is not a histogram, just use normal plot
        #         # x = to_bin_centers(vslice.coords[dim], dim).values
        #         [self.members["lines"][name]
        #          ] = self.ax.plot(xcenters,
        #                           vslice.values,
        #                           label=name,
        #                           zorder=10,
        #                           **{
        #                               key: self.mpl_line_params[key][name]
        #                               for key in self.mpl_line_params.keys()
        #                           })
        #         # Add masks if any
        #         if len(self.masks[name]) > 0:
        #             for m in self.masks[name]:
        #                 mdata = (
        #                     base_mask *
        #                     sc.Variable(dims=vslice.masks[m].dims,
        #                                 values=vslice.masks[m].values.astype(
        #                                     np.int32))).values
        #                 [self.members["masks"][name][m]] = self.ax.plot(
        #                     xcenters,
        #                     self.mask_to_float(mdata, vslice.values),
        #                     zorder=11,
        #                     mec=self.params["masks"][name]["color"],
        #                     mfc="None",
        #                     mew=3.0,
        #                     linestyle="none",
        #                     marker=self.mpl_line_params["marker"][name])
        #                 self.members["masks"][name][m].set_gid("onaxes")

        #     # Add error bars
        #     if self.errorbars[name]:
        #         # if self.histograms[name][dim][dim]:
        #         #     self.current_xcenters = to_bin_centers(
        #         #         self.slider_coord[name][dim], dim).values
        #         # else:
        #         #     self.current_xcenters = new_x
        #         self.members["error_y"][name] = self.ax.errorbar(
        #             xcenters,
        #             ydata,
        #             yerr=vars_to_err(vslice.variances),
        #             color=self.mpl_line_params["color"][name],
        #             zorder=10,
        #             fmt="none")

        # if not self.mpl_axes:
        #     deltax = 0.05 * (xmax - xmin)
        #     with warnings.catch_warnings():
        #         warnings.filterwarnings("ignore", category=UserWarning)
        #         self.ax.set_xlim([xmin - deltax, xmax + deltax])
        #         # if self.input_contains_unaligned_data:
        #         #     self.ax.set_ylim(self.ylim)

        # # self.ax.set_xlabel(
        # #     name_with_unit(
        # #         self.slider_label[self.name][dim]["coord"],
        # #         name=self.slider_label[self.name][dim]["name"],
        # #     ))
        # self.ax.set_xlabel(
        #     name_with_unit(self.data_arrays[self.name].coords[dim]))
        # self.ax.xaxis.set_major_formatter(
        #     self.slider_axformatter[self.name][dim][self.logx])
        # self.ax.xaxis.set_major_locator(
        #     self.slider_axlocator[self.name][dim][self.logx])

        # self.rescale_to_data()

        return

    def slice_data(self, var, name):
        vslice = var
        # Slice along dimensions with active sliders
        for dim, val in self.parent.widgets.slider.items():
            if not val.disabled:
                # self.lab[dim].value = self.make_slider_label(
                #     self.slider_label[self.name][dim]["coord"], val.value)
                # self.lab[dim].value = self.make_slider_label(
                #     var.coords[dim], val.value)
                # self.lab[dim].value = self.make_slider_label(
                #     var.coords[dim], val.value, self.slider_axformatter[name][dim][False])

                # vslice = vslice[val.dim, val.value]

                deltax = self.thickness_slider[dim].value
                vslice = self.resample_image(vslice,
                        rebin_edges={dim: sc.Variable([dim], values=[val.value - 0.5 * deltax,
                                                                     val.value + 0.5 * deltax],
                                                            unit=vslice.coords[dim].unit)})[dim, 0]
                vslice *= (deltax * sc.units.one)











        if vslice.unaligned is not None:
            vslice = sc.histogram(vslice)
            # self.ylim = get_ylim(var=vslice,
            #                           ymin=self.ylim[0],
            #                           ymax=self.ylim[1],
            #                           errorbars=self.errorbars[name],
            #                           logy=self.logy)
        return vslice

    # def slice_masks(self):
    #     mslice = self.masks
    #     for dim, val in self.slider.items():
    #         if not val.disabled and (dim in mslice.dims):
    #             mslice = mslice[dim, val.value]
    #     return mslice

    def update_slice(self, change):
        # Define function to update slices.
        # Special key in the change dict: if "vslice" is found, it means we are
        # calling from a profile viewer, and the slice has hence already been
        # generate.
        new_slices = {}
        for name, var in self.data_arrays.items():
            # if "vslice" in change:
            #     vslice = change["vslice"][name]
            # else:
            new_slices[name] = self.slice_data(var, name)

        self.parent.figure.update_data(new_slices)

        #     vals = vslice.values
        #     dim = self.button_axis_to_dim["x"]
        #     xcoord = vslice.coords[dim]
        #     hist = self.histograms[name][dim][dim]
        #     if hist:
        #         vals = np.concatenate((vals[0:1], vals))
        #     else:
        #         xcoord = to_bin_centers(xcoord, dim)
        #     # self.members["lines"][name].set_ydata(vals)
        #     self.members["lines"][name].set_data(xcoord.values, vals)

        #     if len(self.masks[name]) > 0:
        #         base_mask = sc.Variable(dims=vslice.dims,
        #                                 values=np.ones(vslice.shape,
        #                                                dtype=np.int32))
        #         for m in self.masks[name]:
        #             # Use automatic broadcast to broadcast 0D masks
        #             msk = (base_mask * sc.Variable(
        #                 dims=vslice.masks[m].dims,
        #                 values=vslice.masks[m].values.astype(np.int32))).values
        #             if hist:
        #                 msk = np.concatenate((msk[0:1], msk))
        #             self.members["masks"][name][m].set_data(
        #                 xcoord.values,
        #                 self.mask_to_float(msk, vals))
        #             # self.members["masks"][name][m].set_ydata(
        #             #     self.mask_to_float(msk, vals))

        #     if self.errorbars[name]:
        #         coll = self.members["error_y"][name].get_children()[0]
        #         if hist:
        #             xcoord = to_bin_centers(xcoord, dim)
        #         coll.set_segments(
        #             self.change_segments_y(xcoord.values,
        #                                    vslice.values, vslice.variances))

        # # if self.input_contains_unaligned_data and (not self.mpl_axes):
        # #     with warnings.catch_warnings():
        # #         warnings.filterwarnings("ignore", category=UserWarning)
        # #         self.ax.set_ylim(self.ylim)

        return

    # def keep_remove_trace(self, owner):
    #     if owner.description == "Keep":
    #         self.keep_trace(owner)
    #     elif owner.description == "Remove":
    #         self.remove_trace(owner)
    #     self.fig.canvas.draw_idle()
    #     return

    # def keep_trace(self, owner):
    #     lab = self.keep_buttons[owner.id]["dropdown"].value
    #     # The main line
    #     self.ax.lines.append(cp.copy(self.members["lines"][lab]))
    #     self.ax.lines[-1].set_url(owner.id)
    #     self.ax.lines[-1].set_zorder(2)
    #     if self.ax.lines[-1].get_marker() == "None":
    #         self.ax.lines[-1].set_color(
    #             self.keep_buttons[owner.id]["colorpicker"].value)
    #     else:
    #         self.ax.lines[-1].set_markerfacecolor(
    #             self.keep_buttons[owner.id]["colorpicker"].value)
    #         self.ax.lines[-1].set_markeredgecolor("None")

    #     # The masks
    #     if len(self.masks[lab]) > 0:
    #         for m in self.masks[lab]:
    #             self.ax.lines.append(cp.copy(self.members["masks"][lab][m]))
    #             self.ax.lines[-1].set_url(owner.id)
    #             self.ax.lines[-1].set_gid(m)
    #             self.ax.lines[-1].set_zorder(3)
    #             if self.ax.lines[-1].get_marker() != "None":
    #                 self.ax.lines[-1].set_zorder(3)
    #             else:
    #                 self.ax.lines[-1].set_zorder(1)

    #     if self.errorbars[lab]:
    #         err = self.members["error_y"][lab].get_children()
    #         self.ax.collections.append(cp.copy(err[0]))
    #         self.ax.collections[-1].set_color(
    #             self.keep_buttons[owner.id]["colorpicker"].value)
    #         self.ax.collections[-1].set_url(owner.id)
    #         self.ax.collections[-1].set_zorder(2)

    #     for dim, val in self.slider.items():
    #         if not val.disabled:
    #             lab = "{},{}:{}".format(lab, dim, val.value)
    #     self.keep_buttons[owner.id]["dropdown"].options = [lab]
    #     self.keep_buttons[owner.id]["dropdown"].disabled = True
    #     self.make_keep_button()
    #     owner.description = "Remove"
    #     self.update_button_box_widget()
    #     return

    # def remove_trace(self, owner):
    #     del self.keep_buttons[owner.id]
    #     lines = []
    #     for line in self.ax.lines:
    #         if line.get_url() != owner.id:
    #             lines.append(line)
    #     collections = []
    #     for coll in self.ax.collections:
    #         if coll.get_url() != owner.id:
    #             collections.append(coll)
    #     self.ax.lines = lines
    #     self.ax.collections = collections
    #     self.update_button_box_widget()
    #     return

    # def update_trace_color(self, change):
    #     for line in self.ax.lines:
    #         if line.get_url() == change["owner"].id:
    #             if line.get_marker() == 'None':
    #                 line.set_color(change["new"])
    #             else:
    #                 line.set_markerfacecolor(change["new"])

    #     for coll in self.ax.collections:
    #         if coll.get_url() == change["owner"].id:
    #             coll.set_color(change["new"])
    #     self.fig.canvas.draw_idle()
    #     return

    # def change_segments_y(self, x, y, e):
    #     e = vars_to_err(e)
    #     arr1 = np.repeat(x, 2)
    #     arr2 = np.array([y - e, y + e]).T.flatten()
    #     return np.array([arr1, arr2]).T.flatten().reshape(len(y), 2, 2)

    # def toggle_mask(self, change):
    #     msk = self.members["masks"][change["owner"].masks_group][
    #         change["owner"].masks_name]
    #     if msk.get_gid() == "onaxes":
    #         msk.set_visible(change["new"])
    #     # Also toggle masks on additional lines created by keep button
    #     for line in self.ax.lines:
    #         if line.get_gid() == change["owner"].masks_name:
    #             line.set_visible(change["new"])
    #     return

    # def rescale_to_data(self, button=None):
    #     # self.ax.set_ylim(get_ylim())
    #     self.ax.autoscale(True)
    #     self.ax.relim()
    #     self.ax.autoscale_view()
    #     return

    # # def vars_to_err(self, v):
    # #     with np.errstate(invalid="ignore"):
    # #         v = np.sqrt(v)
    # #     non_finites = np.where(np.logical_not(np.isfinite(v)))
    # #     v[non_finites] = 0.0
    # #     return v
