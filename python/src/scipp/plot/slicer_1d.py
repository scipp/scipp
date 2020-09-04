# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import config
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
import os


# def plot_1d(scipp_obj_dict=None,
#             axes=None,
#             errorbars=None,
#             masks={"color": "k"},
#             filename=None,
#             figsize=None,
#             ax=None,
#             mpl_line_params=None,
#             logx=False,
#             logy=False,
#             logxy=False,
#             grid=False):
#     """
#     Plot a 1D spectrum.

#     Input is a Dataset containing one or more Variables.
#     If the coordinate of the x-axis contains bin edges, then a bar plot is
#     made.
#     If the data contains more than one dimensions, sliders are added.

#     """

#     sv = Slicer1d(scipp_obj_dict=scipp_obj_dict,
#                   axes=axes,
#                   errorbars=errorbars,
#                   masks=masks,
#                   ax=ax,
#                   mpl_line_params=mpl_line_params,
#                   logx=logx or logxy,
#                   logy=logy or logxy,
#                   grid=grid)

#     if ax is None:
#         render_plot(figure=sv.fig, widgets=sv.box, filename=filename)

#     return sv


class Slicer1d(Slicer):
    def __init__(self,
                 scipp_obj_dict=None,
                 axes=None,
                 errorbars=None,
                 masks=None,
                 ax=None,
                 mpl_line_params=None,
                 logx=False,
                 logy=False,
                 grid=False):
        os.write(1, "Slicer1d 0\n".encode())

        super().__init__(scipp_obj_dict=scipp_obj_dict,
                         axes=axes,
                         masks=masks,
                         button_options=['X'])

        # self.scipp_obj_dict = scipp_obj_dict
        os.write(1, "Slicer1d 1\n".encode())
        self.fig = None
        self.ax = ax
        self.mpl_axes = False
        self.input_contains_unaligned_data = False
        self.current_xcenters = None
        if self.ax is None:
            self.fig, self.ax = plt.subplots(
                1,
                1,
                figsize=(config.plot.width / config.plot.dpi,
                         config.plot.height / config.plot.dpi),
                dpi=config.plot.dpi)
        else:
            self.mpl_axes = True
        if grid:
            self.ax.grid()
        os.write(1, "Slicer1d 2\n".encode())

        # Determine whether error bars should be plotted or not
        self.errorbars = {}
        for name, var in self.data_arrays.items():
            if var.unaligned is not None:
                self.errorbars[name] = var.unaligned.variances is not None
                self.input_contains_unaligned_data = True
            else:
                self.errorbars[name] = var.variances is not None
        if errorbars is not None:
            if isinstance(errorbars, bool):
                for name, var in self.data_arrays.items():
                    self.errorbars[name] &= errorbars
            elif isinstance(errorbars, dict):
                for name, v in errorbars.items():
                    if name in self.data_arrays:
                        self.errorbars[
                            name] = errorbars[name] and self.data_arrays[
                                name].variances is not None
                    else:
                        print("Warning: key {} was not found in list of "
                              "entries to plot and will be ignored.".format(
                                  name))
            else:
                raise TypeError("Unsupported type for argument "
                                "'errorbars': {}".format(type(errorbars)))
        os.write(1, "Slicer1d 3\n".encode())

        # Save the line parameters (color, linewidth...)
        self.mpl_line_params = mpl_line_params

        self.names = []
        # self.ylim = [np.Inf, np.NINF]
        self.logx = logx
        self.logy = logy
        for name, var in self.data_arrays.items():
            self.names.append(name)
            # if var.values is not None:
            #     self.ylim = get_ylim(var=var,
            #                               ymin=self.ylim[0],
            #                               ymax=self.ylim[1],
            #                               errorbars=self.errorbars[name],
            #                               logy=self.logy)
            ylab = name_with_unit(var=var, name="")
        os.write(1, "Slicer1d 4\n".encode())

        # if (not self.mpl_axes) and (var.values is not None):
        #     with warnings.catch_warnings():
        #         warnings.filterwarnings("ignore", category=UserWarning)
        #         self.ax.set_ylim(self.ylim)

        if self.logx:
            self.ax.set_xscale("log")
        if self.logy:
            self.ax.set_yscale("log")

        # Disable buttons
        for dim, button in self.buttons.items():
            if self.slider[dim].disabled:
                button.disabled = True
        os.write(1, "Slicer1d 5\n".encode())
        self.update_axes(list(self.slider.keys())[-1])
        os.write(1, "Slicer1d 6\n".encode())

        self.ax.set_ylabel(ylab)
        if len(self.ax.get_legend_handles_labels()[0]) > 0:
            self.ax.legend()

        self.keep_buttons = dict()
        self.make_keep_button()
        os.write(1, "Slicer1d 7\n".encode())

        # vbox contains the original sliders and buttons.
        # In keep_buttons_box, we include the keep trace buttons.
        self.keep_buttons_box = []
        for key, val in self.keep_buttons.items():
            self.keep_buttons_box.append(widgets.HBox(list(val.values())))
        self.keep_buttons_box = widgets.VBox(self.keep_buttons_box)
        self.box = widgets.VBox(
            [widgets.VBox(self.vbox), self.keep_buttons_box])
        # self.box.layout.align_items = 'center'
        if self.ndim < 2:
            self.keep_buttons_box.layout.display = 'none'

        # Populate the members
        self.members["fig"] = self.fig
        self.members["ax"] = self.ax
        os.write(1, "Slicer1d 8\n".encode())

        return

    # def get_finite_y(self, arr):
    #     if self.logy:
    #         with np.errstate(divide="ignore", invalid="ignore"):
    #             arr = np.log10(arr, out=arr)
    #     subset = np.where(np.isfinite(arr))
    #     return arr[subset]

    # def get_ylim(self, var=None, ymin=None, ymax=None, errorbars=False):
    #     if errorbars:
    #         err = self.vars_to_err(var.variances)
    #     else:
    #         err = 0.0

    #     ymin_new = np.amin(self.get_finite_y(var.values - err))
    #     ymax_new = np.amax(self.get_finite_y(var.values + err))

    #     dy = 0.05 * (ymax_new - ymin_new)
    #     ymin_new -= dy
    #     ymax_new += dy
    #     if self.logy:
    #         ymin_new = 10.0**ymin_new
    #         ymax_new = 10.0**ymax_new
    #     return [min(ymin, ymin_new), max(ymax, ymax_new)]

    def make_keep_button(self):
        drop = widgets.Dropdown(options=self.names,
                                description='',
                                layout={'width': 'initial'})
        but = widgets.Button(description="Keep",
                             disabled=False,
                             button_style="",
                             layout={'width': "70px"})
        # Generate a random color. TODO: should we initialise the seed?
        col = widgets.ColorPicker(concise=True,
                                  description='',
                                  value='#%02X%02X%02X' %
                                  (tuple(np.random.randint(0, 255, 3))),
                                  disabled=False)
        # Make a unique id
        key = str(id(but))
        setattr(but, "id", key)
        setattr(col, "id", key)
        but.on_click(self.keep_remove_trace)
        col.observe(self.update_trace_color, names="value")
        self.keep_buttons[key] = {
            "dropdown": drop,
            "button": but,
            "colorpicker": col
        }
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
        self.keep_buttons = dict()
        self.make_keep_button()
        self.update_button_box_widget()
        return

    def update_button_box_widget(self):
        self.mbox = self.vbox.copy()
        for k, b in self.keep_buttons.items():
            self.mbox.append(widgets.HBox(list(b.values())))
        self.box.children = tuple(self.mbox)

    def update_axes(self, dim):
        if not self.mpl_axes:
            self.ax.lines = []
            self.ax.collections = []
        self.members.update({
            "lines": {},
            "error_x": {},
            "error_y": {},
            "error_xy": {},
            "masks": {}
        })

        xmin = np.Inf
        xmax = np.NINF
        os.write(1, "update_axes 1\n".encode())

        for name, array in self.data_arrays.items():
            # new_x = self.slider_coord[name][dim].values
            # new_x = array.coords[dim].values
            xmin = min(sc.min(array.coords[dim]).value, xmin)
            xmax = max(sc.max(array.coords[dim]).value, xmax)
            os.write(1, "update_axes 2\n".encode())

            vslice = self.slice_data(array, name)
            ydata = vslice.values
            xcenters = to_bin_centers(vslice.coords[dim], dim).values
            os.write(1, "update_axes 3\n".encode())

            if len(self.masks[name]) > 0:
                self.members["masks"][name] = {}
                base_mask = sc.Variable(dims=vslice.dims,
                                        values=np.ones(vslice.shape,
                                                       dtype=np.int32))
            os.write(1, "update_axes 4\n".encode())

            # If this is a histogram, plot a step function
            if self.histograms[name][dim][dim]:
                os.write(1, "update_axes 5\n".encode())
                ye = np.concatenate((ydata[0:1], ydata))
                os.write(1, "update_axes 5.1\n".encode())
                os.write(1, (str(vslice.coords[dim].values) + "\n").encode())
                os.write(1, (str(name) + "\n").encode())
                os.write(1, (str(ye) + "\n").encode())
                os.write(1, (str(self.mpl_line_params) + "\n").encode())

                [self.members["lines"][name]
                 ] = self.ax.step(vslice.coords[dim].values,
                                  ye,
                                  label=name,
                                  zorder=10,
                                  **{
                                      key: self.mpl_line_params[key][name]
                                      for key in ["color", "linewidth"]
                                  })
                os.write(1, "update_axes 5.2\n".encode())
                # Add masks if any
                if len(self.masks[name]) > 0:
                    for m in self.masks[name]:
                        mdata = (
                            base_mask *
                            sc.Variable(dims=vslice.masks[m].dims,
                                        values=vslice.masks[m].values.astype(
                                            np.int32))).values

                        me = np.concatenate((mdata[0:1], mdata))
                        [self.members["masks"][name][m]] = self.ax.step(
                            vslice.coords[dim].values,
                            self.mask_to_float(me, ye),
                            linewidth=self.mpl_line_params["linewidth"][name] *
                            3.0,
                            color=self.params["masks"][name]["color"],
                            zorder=9)
                        # Abuse a mostly unused property `gid` of Line2D to
                        # identify the line as a mask. We set gid to `onaxes`.
                        # This is used by the profile viewer in the 2D plotter
                        # to know whether to show the mask or not, depending on
                        # whether the cursor is hovering over the 2D image or
                        # not.
                        self.members["masks"][name][m].set_gid("onaxes")
                os.write(1, "update_axes 5.3\n".encode())

            else:
                os.write(1, "update_axes 6\n".encode())

                # If this is not a histogram, just use normal plot
                # x = to_bin_centers(vslice.coords[dim], dim).values
                [self.members["lines"][name]
                 ] = self.ax.plot(xcenters,
                                  vslice.values,
                                  label=name,
                                  zorder=10,
                                  **{
                                      key: self.mpl_line_params[key][name]
                                      for key in self.mpl_line_params.keys()
                                  })
                # Add masks if any
                if len(self.masks[name]) > 0:
                    for m in self.masks[name]:
                        mdata = (
                            base_mask *
                            sc.Variable(dims=vslice.masks[m].dims,
                                        values=vslice.masks[m].values.astype(
                                            np.int32))).values
                        [self.members["masks"][name][m]] = self.ax.plot(
                            xcenters,
                            self.mask_to_float(mdata, vslice.values),
                            zorder=11,
                            mec=self.params["masks"][name]["color"],
                            mfc="None",
                            mew=3.0,
                            linestyle="none",
                            marker=self.mpl_line_params["marker"][name])
                        self.members["masks"][name][m].set_gid("onaxes")
            os.write(1, "update_axes 7\n".encode())

            # Add error bars
            if self.errorbars[name]:
                # if self.histograms[name][dim][dim]:
                #     self.current_xcenters = to_bin_centers(
                #         self.slider_coord[name][dim], dim).values
                # else:
                #     self.current_xcenters = new_x
                self.members["error_y"][name] = self.ax.errorbar(
                    xcenters,
                    ydata,
                    yerr=vars_to_err(vslice.variances),
                    color=self.mpl_line_params["color"][name],
                    zorder=10,
                    fmt="none")
        os.write(1, "update_axes 8\n".encode())

        if not self.mpl_axes:
            deltax = 0.05 * (xmax - xmin)
            with warnings.catch_warnings():
                warnings.filterwarnings("ignore", category=UserWarning)
                self.ax.set_xlim([xmin - deltax, xmax + deltax])
                # if self.input_contains_unaligned_data:
                #     self.ax.set_ylim(self.ylim)

        # self.ax.set_xlabel(
        #     name_with_unit(
        #         self.slider_label[self.name][dim]["coord"],
        #         name=self.slider_label[self.name][dim]["name"],
        #     ))
        os.write(1, "update_axes 9\n".encode())
        self.ax.set_xlabel(
            name_with_unit(self.data_arrays[self.name].coords[dim]))
        self.ax.xaxis.set_major_formatter(
            self.slider_axformatter[self.name][dim][self.logx])
        self.ax.xaxis.set_major_locator(
            self.slider_axlocator[self.name][dim][self.logx])
        os.write(1, "update_axes 10\n".encode())

        self.rescale_to_data()
        os.write(1, "update_axes 11\n".encode())

        return

    def slice_data(self, var, name):
        vslice = var
        # Slice along dimensions with active sliders
        for dim, val in self.slider.items():
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

    def slice_masks(self):
        mslice = self.masks
        for dim, val in self.slider.items():
            if not val.disabled and (dim in mslice.dims):
                mslice = mslice[dim, val.value]
        return mslice

    def update_slice(self, change):
        # Define function to update slices.
        # Special key in the change dict: if "vslice" is found, it means we are
        # calling from a profile viewer, and the slice has hence already been
        # generate.
        for name, var in self.data_arrays.items():
            if "vslice" in change:
                vslice = change["vslice"][name]
            else:
                vslice = self.slice_data(var, name)
            vals = vslice.values
            dim = self.button_axis_to_dim["x"]
            xcoord = vslice.coords[dim]
            hist = self.histograms[name][dim][dim]
            if hist:
                vals = np.concatenate((vals[0:1], vals))
            else:
                xcoord = to_bin_centers(xcoord, dim)
            # self.members["lines"][name].set_ydata(vals)
            self.members["lines"][name].set_data(xcoord.values, vals)

            if len(self.masks[name]) > 0:
                base_mask = sc.Variable(dims=vslice.dims,
                                        values=np.ones(vslice.shape,
                                                       dtype=np.int32))
                for m in self.masks[name]:
                    # Use automatic broadcast to broadcast 0D masks
                    msk = (base_mask * sc.Variable(
                        dims=vslice.masks[m].dims,
                        values=vslice.masks[m].values.astype(np.int32))).values
                    if hist:
                        msk = np.concatenate((msk[0:1], msk))
                    self.members["masks"][name][m].set_data(
                        xcoord.values,
                        self.mask_to_float(msk, vals))
                    # self.members["masks"][name][m].set_ydata(
                    #     self.mask_to_float(msk, vals))

            if self.errorbars[name]:
                coll = self.members["error_y"][name].get_children()[0]
                if hist:
                    xcoord = to_bin_centers(xcoord, dim)
                coll.set_segments(
                    self.change_segments_y(xcoord.values,
                                           vslice.values, vslice.variances))

        # if self.input_contains_unaligned_data and (not self.mpl_axes):
        #     with warnings.catch_warnings():
        #         warnings.filterwarnings("ignore", category=UserWarning)
        #         self.ax.set_ylim(self.ylim)

        return

    def keep_remove_trace(self, owner):
        if owner.description == "Keep":
            self.keep_trace(owner)
        elif owner.description == "Remove":
            self.remove_trace(owner)
        self.fig.canvas.draw_idle()
        return

    def keep_trace(self, owner):
        lab = self.keep_buttons[owner.id]["dropdown"].value
        # The main line
        self.ax.lines.append(cp.copy(self.members["lines"][lab]))
        self.ax.lines[-1].set_url(owner.id)
        self.ax.lines[-1].set_zorder(2)
        if self.ax.lines[-1].get_marker() == "None":
            self.ax.lines[-1].set_color(
                self.keep_buttons[owner.id]["colorpicker"].value)
        else:
            self.ax.lines[-1].set_markerfacecolor(
                self.keep_buttons[owner.id]["colorpicker"].value)
            self.ax.lines[-1].set_markeredgecolor("None")

        # The masks
        if len(self.masks[lab]) > 0:
            for m in self.masks[lab]:
                self.ax.lines.append(cp.copy(self.members["masks"][lab][m]))
                self.ax.lines[-1].set_url(owner.id)
                self.ax.lines[-1].set_gid(m)
                self.ax.lines[-1].set_zorder(3)
                if self.ax.lines[-1].get_marker() != "None":
                    self.ax.lines[-1].set_zorder(3)
                else:
                    self.ax.lines[-1].set_zorder(1)

        if self.errorbars[lab]:
            err = self.members["error_y"][lab].get_children()
            self.ax.collections.append(cp.copy(err[0]))
            self.ax.collections[-1].set_color(
                self.keep_buttons[owner.id]["colorpicker"].value)
            self.ax.collections[-1].set_url(owner.id)
            self.ax.collections[-1].set_zorder(2)

        for dim, val in self.slider.items():
            if not val.disabled:
                lab = "{},{}:{}".format(lab, dim, val.value)
        self.keep_buttons[owner.id]["dropdown"].options = [lab]
        self.keep_buttons[owner.id]["dropdown"].disabled = True
        self.make_keep_button()
        owner.description = "Remove"
        self.update_button_box_widget()
        return

    def remove_trace(self, owner):
        del self.keep_buttons[owner.id]
        lines = []
        for line in self.ax.lines:
            if line.get_url() != owner.id:
                lines.append(line)
        collections = []
        for coll in self.ax.collections:
            if coll.get_url() != owner.id:
                collections.append(coll)
        self.ax.lines = lines
        self.ax.collections = collections
        self.update_button_box_widget()
        return

    def update_trace_color(self, change):
        for line in self.ax.lines:
            if line.get_url() == change["owner"].id:
                if line.get_marker() == 'None':
                    line.set_color(change["new"])
                else:
                    line.set_markerfacecolor(change["new"])

        for coll in self.ax.collections:
            if coll.get_url() == change["owner"].id:
                coll.set_color(change["new"])
        self.fig.canvas.draw_idle()
        return

    def change_segments_y(self, x, y, e):
        e = vars_to_err(e)
        arr1 = np.repeat(x, 2)
        arr2 = np.array([y - e, y + e]).T.flatten()
        return np.array([arr1, arr2]).T.flatten().reshape(len(y), 2, 2)

    def toggle_mask(self, change):
        msk = self.members["masks"][change["owner"].masks_group][
            change["owner"].masks_name]
        if msk.get_gid() == "onaxes":
            msk.set_visible(change["new"])
        # Also toggle masks on additional lines created by keep button
        for line in self.ax.lines:
            if line.get_gid() == change["owner"].masks_name:
                line.set_visible(change["new"])
        return

    def rescale_to_data(self, button=None):
        # self.ax.set_ylim(get_ylim())
        self.ax.autoscale(True)
        self.ax.relim()
        self.ax.autoscale_view()
        return

    # def vars_to_err(self, v):
    #     with np.errstate(invalid="ignore"):
    #         v = np.sqrt(v)
    #     non_finites = np.where(np.logical_not(np.isfinite(v)))
    #     v[non_finites] = 0.0
    #     return v
