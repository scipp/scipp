# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import config
from .render import render_plot
from .slicer import Slicer
from .tools import to_bin_centers, vars_to_err, mask_to_float, get_line_param
from .._utils import name_with_unit
from .._scipp import core as sc

# Other imports
import numpy as np
import copy as cp
import matplotlib.pyplot as plt
import ipywidgets as widgets
import warnings



class LinePlot:

    def __init__(self,
                 # dict_of_data_arrays=None,
                 errorbars=None,
                 # is_bin_edge=None,
                 ax=None,
                 mpl_line_params=None,
                 title=None,
                 unit=None,
                 logx=False,
                 logy=False,
                 grid=False,
                 mask_params=None,
                 mask_names=None,
                 figsize=None):
                 # axformatter=None,
                 # axlocator=None):

        # self.dict_of_data_arrays = dict_of_data_arrays
        # self.is_bin_edge = is_bin_edge
        # self.axformatter = axformatter
        # self.axlocator = axlocator
        self.data_lines = {}
        self.mask_lines = {}
        self.error_lines = {}

        self.errorbars = errorbars
        self.mask_names = mask_names
        self.mask_params = mask_params

        # self.masks = masks
        # if self.masks is None:
        #     self.masks = {"show": True, "color": "k"}

        self.fig = None
        self.ax = ax
        self.mpl_axes = False
        # self.input_contains_unaligned_data = False
        self.current_xcenters = None
        if self.ax is None:
            if figsize is None:
                figsize = (config.plot.width / config.plot.dpi,
                         config.plot.height / config.plot.dpi)
            self.fig, self.ax = plt.subplots(
                1,
                1,
                figsize=figsize,
                dpi=config.plot.dpi)
        else:
            self.mpl_axes = True
        self.grid = grid

        plt.tight_layout(pad=1.5)

        # if grid:
        #     self.ax.grid()

        # # Determine whether error bars should be plotted or not
        # self.errorbars = {}
        # for name, var in self.dict_of_data_arrays.items():
        #     if var.unaligned is not None:
        #         self.errorbars[name] = var.unaligned.variances is not None
        #         self.input_contains_unaligned_data = True
        #     else:
        #         self.errorbars[name] = var.variances is not None
        # if errorbars is not None:
        #     if isinstance(errorbars, bool):
        #         for name, var in self.data_arrays.items():
        #             self.errorbars[name] &= errorbars
        #     elif isinstance(errorbars, dict):
        #         for name, v in errorbars.items():
        #             if name in self.data_arrays:
        #                 self.errorbars[
        #                     name] = errorbars[name] and self.data_arrays[
        #                         name].variances is not None
        #             else:
        #                 print("Warning: key {} was not found in list of "
        #                       "entries to plot and will be ignored.".format(
        #                           name))
        #     else:
        #         raise TypeError("Unsupported type for argument "
        #                         "'errorbars': {}".format(type(errorbars)))

        # Save the line parameters (color, linewidth...)
        self.mpl_line_params = mpl_line_params
        # if self.mpl_line_params is None:
        #     self.mpl_line_params = {
        #         "color": get_line_param("color", 0),
        #         "marker": get_line_param("marker", 0),
        #         "linestyle": get_line_param("linestyle", 0),
        #         "linewidth": get_line_param("linewidth", 0)
        #     }

        # self.names = []
        # self.ylim = [np.Inf, np.NINF]
        self.logx = logx
        self.logy = logy
        self.unit = unit
        # print(self.logx, self.logx)

        # for name, var in self.dict_of_data_arrays.items():
        # #     self.names.append(name)
        # #     # if var.values is not None:
        # #     #     self.ylim = get_ylim(var=var,
        # #     #                               ymin=self.ylim[0],
        # #     #                               ymax=self.ylim[1],
        # #     #                               errorbars=self.errorbars[name],
        # #     #                               logy=self.logy)
        #     ylab = name_with_unit(var=var, name="")

        # # if (not self.mpl_axes) and (var.values is not None):
        # #     with warnings.catch_warnings():
        # #         warnings.filterwarnings("ignore", category=UserWarning)
        # #         self.ax.set_ylim(self.ylim)

        # if self.logx:
        #     self.ax.set_xscale("log")
        # if self.logy:
        #     self.ax.set_yscale("log")

        # # # Disable buttons
        # # for dim, button in self.buttons.items():
        # #     if self.slider[dim].disabled:
        # #         button.disabled = True
        # # self.plot_data(dict_of_data_arrays)

        # self.ax.set_ylabel(unit)
        # if len(self.ax.get_legend_handles_labels()[0]) > 0:
        #     self.ax.legend()

        # self.keep_buttons = dict()
        # self.make_keep_button()

        # # vbox contains the original sliders and buttons.
        # # In keep_buttons_box, we include the keep trace buttons.
        # self.keep_buttons_box = []
        # for key, val in self.keep_buttons.items():
        #     self.keep_buttons_box.append(widgets.HBox(list(val.values())))
        # self.keep_buttons_box = widgets.VBox(self.keep_buttons_box)
        # self.box = widgets.VBox(
        #     [widgets.VBox(self.vbox), self.keep_buttons_box])
        # # self.box.layout.align_items = 'center'
        # if self.ndim < 2:
        #     self.keep_buttons_box.layout.display = 'none'

        # # Populate the members
        # self.members["fig"] = self.fig
        # self.members["ax"] = self.ax

        return


    def _ipython_display_(self):
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        return self.fig.canvas

    def savefig(self, filename=None):
        self.fig.savefig(filename, bbox_inches="tight")

    def toggle_view(self, visible=True):
        self.fig.canvas.layout.display = None if visible else 'none'

    def update_axes(self, axparams=None, axformatter=None, axlocator=None, logx=False, logy=False, clear=True):
        #if clear:
        self.ax.clear()
        # if not self.mpl_axes:
        #     # self.ax.lines = []
        #     # self.ax.collections = []
        #     self.ax.clear()
        # # self.members.update({
        # #     "lines": {},
        # #     "error_x": {},
        # #     "error_y": {},
        # #     "error_xy": {},
        # #     "masks": {}
        # # })

        if self.mpl_line_params is None:
            self.mpl_line_params = {
                "color": {},
                "marker": {},
                "linestyle": {},
                "linewidth": {}
            }
            for i, name in enumerate(axparams["x"]["hist"]):
                self.mpl_line_params["color"][name] = get_line_param("color", i)
                self.mpl_line_params["marker"][name] =  get_line_param("marker", i)
                self.mpl_line_params["linestyle"][name] =  get_line_param("linestyle", i)
                self.mpl_line_params["linewidth"][name] =  get_line_param("linewidth", i)


        if self.logx:
            self.ax.set_xscale("log")
        if self.logy:
            self.ax.set_yscale("log")

        self.ax.set_ylabel(self.unit)

        if self.grid:
            self.ax.grid()


        # if not clear:
        deltax = 0.05 * (axparams["x"]["lims"][1] - axparams["x"]["lims"][0])
        with warnings.catch_warnings():
            warnings.filterwarnings("ignore", category=UserWarning)
            self.ax.set_xlim([axparams["x"]["lims"][0] - deltax, axparams["x"]["lims"][1] + deltax])
            # if self.input_contains_unaligned_data:
            #     self.ax.set_ylim(self.ylim)


        self.ax.set_xlabel(axparams["x"]["label"])

        # print(axlocator)
        if axlocator is not None:
            self.ax.xaxis.set_major_locator(
                axlocator[axparams["x"]["dim"]][logx])
        if axformatter is not None:
            self.ax.xaxis.set_major_formatter(
                axformatter[axparams["x"]["dim"]][logx])



        for name, hist in axparams["x"]["hist"].items():

            label = name if len(name) > 0 else " "

            self.mask_lines[name] = {}

            if hist:
                [self.data_lines[name]
                 ] = self.ax.step([1, 2],
                                  [1, 2],
                                  label=label,
                                  zorder=10,
                                  **{
                                      key: self.mpl_line_params[key][name]
                                      for key in ["color", "linewidth"]
                                  })
                for m in self.mask_names[name]:
                    [self.mask_lines[name][m]] = self.ax.step(
                        [1, 2],
                        [1, 2],
                        linewidth=self.mpl_line_params["linewidth"][name] *
                        3.0,
                        color=self.mask_params["color"],
                        zorder=9)
                    # Abuse a mostly unused property `gid` of Line2D to
                    # identify the line as a mask. We set gid to `onaxes`.
                    # This is used by the profile viewer in the 2D plotter
                    # to know whether to show the mask or not, depending on
                    # whether the cursor is hovering over the 2D image or
                    # not.
                    self.mask_lines[name][m].set_gid("onaxes")
            else:
                [self.data_lines[name]
                 ] = self.ax.plot([1, 2],
                                  [1, 2],
                                  label=label,
                                  zorder=10,
                                  **{
                                      key: self.mpl_line_params[key][name]
                                      for key in self.mpl_line_params.keys()
                                  })
                # print(self.mask_names)
                # print(self.mpl_line_params)
                # print(self.mpl_line_params["marker"])
                # print(self.mpl_line_params["marker"][name])
                # print(self.mask_params)
                # print(self.mask_params["color"])
                for m in self.mask_names[name]:
                    [self.mask_lines[name][m]] = self.ax.plot(
                        [1, 2],
                        [1, 2],
                        zorder=11,
                        mec=self.mask_params["color"],
                        mfc="None",
                        mew=3.0,
                        linestyle="none",
                        marker=self.mpl_line_params["marker"][name])
                    self.mask_lines[name][m].set_gid("onaxes")



            # Add error bars
            if self.errorbars[name]:
                # if self.histograms[name][dim][dim]:
                #     self.current_xcenters = to_bin_centers(
                #         self.slider_coord[name][dim], dim).values
                # else:
                #     self.current_xcenters = new_x
                self.error_lines[name] = self.ax.errorbar(
                    [1, 2],
                    [1, 2],
                    yerr=[1, 1],
                    color=self.mpl_line_params["color"][name],
                    zorder=10,
                    fmt="none")



        # if len(self.ax.get_legend_handles_labels()[0]) > 0:
        self.ax.legend()





        # if is_bin_edge is not None:
        #     self.is_bin_edge = is_bin_edge

        # dim = None


        # xmin = np.Inf
        # xmax = np.NINF
        # for name, array in dict_of_data_arrays.items():
        #     # new_x = self.slider_coord[name][dim].values
        #     # new_x = array.coords[dim].values

        #     dim = array.dims[0]

        #     xmin = min(sc.min(array.coords[dim]).value, xmin)
        #     xmax = max(sc.max(array.coords[dim]).value, xmax)


        #     # vslice = self.slice_data(array, name)
        #     ydata = array.values
        #     xcenters = to_bin_centers(array.coords[dim], dim).values

        #     self.mask_lines[name] = {}

        #     if len(array.masks) > 0:
        #         # self.members["masks"][name] = {}
        #         base_mask = sc.Variable(dims=array.dims,
        #                                 values=np.ones(array.shape,
        #                                                dtype=np.int32))

        #     # If this is a histogram, plot a step function
        #     if self.is_bin_edge[name]:
        #         ye = np.concatenate((ydata[0:1], ydata))
        #         [self.data_lines[name]
        #          ] = self.ax.step(array.coords[dim].values,
        #                           ye,
        #                           label=name,
        #                           zorder=10,
        #                           **{
        #                               key: self.mpl_line_params[key][name]
        #                               for key in ["color", "linewidth"]
        #                           })
        #         # Add masks if any
        #         # if len(self.masks[name]) > 0:
        #         for m in array.masks:
        #             mdata = (
        #                 base_mask *
        #                 sc.Variable(dims=array.masks[m].dims,
        #                             values=array.masks[m].values.astype(
        #                                 np.int32))).values

        #             me = np.concatenate((mdata[0:1], mdata))
        #             [self.mask_lines[name][m]] = self.ax.step(
        #                 array.coords[dim].values,
        #                 mask_to_float(me, ye),
        #                 linewidth=self.mpl_line_params["linewidth"][name] *
        #                 3.0,
        #                 color=self.masks["color"],
        #                 zorder=9)
        #             # Abuse a mostly unused property `gid` of Line2D to
        #             # identify the line as a mask. We set gid to `onaxes`.
        #             # This is used by the profile viewer in the 2D plotter
        #             # to know whether to show the mask or not, depending on
        #             # whether the cursor is hovering over the 2D image or
        #             # not.
        #             self.mask_lines[name][m].set_gid("onaxes")

        #     else:

        #         # If this is not a histogram, just use normal plot
        #         # x = to_bin_centers(vslice.coords[dim], dim).values
        #         [self.data_lines[name]
        #          ] = self.ax.plot(xcenters,
        #                           array.values,
        #                           label=name,
        #                           zorder=10,
        #                           **{
        #                               key: self.mpl_line_params[key][name]
        #                               for key in self.mpl_line_params.keys()
        #                           })
        #         # Add masks if any
        #         # if len(self.masks[name]) > 0:
        #         for m in array.masks:
        #             mdata = (
        #                 base_mask *
        #                 sc.Variable(dims=array.masks[m].dims,
        #                             values=array.masks[m].values.astype(
        #                                 np.int32))).values
        #             [self.mask_lines[name][m]] = self.ax.plot(
        #                 xcenters,
        #                 mask_to_float(mdata, array.values),
        #                 zorder=11,
        #                 mec=self.masks["color"],
        #                 mfc="None",
        #                 mew=3.0,
        #                 linestyle="none",
        #                 marker=self.mpl_line_params["marker"][name])
        #             self.mask_lines[name][m].set_gid("onaxes")


























        # if not clear:
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
        #     name_with_unit(self.dict_of_data_arrays[name].coords[dim]))

        # if self.axlocator is not None:
        #     self.ax.xaxis.set_major_locator(
        #         self.axlocator[self.logx])
        # if self.axformatter is not None:
        #     self.ax.xaxis.set_major_formatter(
        #         self.axformatter[self.logx])

        # self.rescale_to_data()

        return

    # def slice_data(self, var, name):
    #     vslice = var
    #     # Slice along dimensions with active sliders
    #     for dim, val in self.slider.items():
    #         if not val.disabled:
    #             # self.lab[dim].value = self.make_slider_label(
    #             #     self.slider_label[self.name][dim]["coord"], val.value)
    #             # self.lab[dim].value = self.make_slider_label(
    #             #     var.coords[dim], val.value)
    #             # self.lab[dim].value = self.make_slider_label(
    #             #     var.coords[dim], val.value, self.slider_axformatter[name][dim][False])

    #             # vslice = vslice[val.dim, val.value]

    #             deltax = self.thickness_slider[dim].value
    #             vslice = self.resample_image(vslice,
    #                     rebin_edges={dim: sc.Variable([dim], values=[val.value - 0.5 * deltax,
    #                                                                  val.value + 0.5 * deltax],
    #                                                         unit=vslice.coords[dim].unit)})[dim, 0]
    #             vslice *= (deltax * sc.units.one)











    #     if vslice.unaligned is not None:
    #         vslice = sc.histogram(vslice)
    #         # self.ylim = get_ylim(var=vslice,
    #         #                           ymin=self.ylim[0],
    #         #                           ymax=self.ylim[1],
    #         #                           errorbars=self.errorbars[name],
    #         #                           logy=self.logy)
    #     return vslice

    # def slice_masks(self):
    #     mslice = self.masks
    #     for dim, val in self.slider.items():
    #         if not val.disabled and (dim in mslice.dims):
    #             mslice = mslice[dim, val.value]
    #     return mslice

    def update_data(self, new_values):
        # Define function to update slices.
        # Special key in the change dict: if "vslice" is found, it means we are
        # calling from a profile viewer, and the slice has hence already been
        # generate.
        os.write(1, "lineplot: update_data 1\n".encode())
        for name, vals in new_values.items():
            os.write(1, ("lineplot: update_data 2" + name + "\n").encode())
            # # if "vslice" in change:
            # #     vslice = change["vslice"][name]
            # # else:
            # #     vslice = self.slice_data(var, name)
            # vals = array.values
            # # dim = self.button_axis_to_dim["x"]
            # dim = array.dims[0]
            # xcoord = array.coords[dim]
            # # hist = self.is_bin_edge[name]
            # if self.is_bin_edge[name]:
            #     vals = np.concatenate((vals[0:1], vals))
            # else:
            #     xcoord = to_bin_centers(xcoord, dim)
            # # self.members["lines"][name].set_ydata(vals)


            self.data_lines[name].set_data(vals["values"]["x"], vals["values"]["y"])
            os.write(1, "lineplot: update_data 3\n".encode())




            # if len(array.masks) > 0:
            #     base_mask = sc.Variable(dims=array.dims,
            #                             values=np.ones(array.shape,
            #                                            dtype=np.int32))
            for m in vals["masks"]:
                self.mask_lines[name][m].set_data(
                    vals["values"]["x"],
                    vals["masks"][m])
            os.write(1, "lineplot: update_data 4\n".encode())

            if self.errorbars[name]:
                coll = self.error_lines[name].get_children()[0]
                # if self.is_bin_edge[name]:
                #     xcoord = to_bin_centers(xcoord, dim)
                coll.set_segments(
                    self.change_segments_y(vals["variances"]["x"],
                                           vals["variances"]["y"],
                                           vals["variances"]["e"]))
            os.write(1, "lineplot: update_data 5\n".encode())

        # if self.input_contains_unaligned_data and (not self.mpl_axes):
        #     with warnings.catch_warnings():
        #         warnings.filterwarnings("ignore", category=UserWarning)
        #         self.ax.set_ylim(self.ylim)
        self.fig.canvas.draw_idle()

        return

    # def keep_remove_trace(self, owner):
    #     if owner.description == "Keep":
    #         self.keep_trace(owner)
    #     elif owner.description == "Remove":
    #         self.remove_trace(owner)
    #     self.fig.canvas.draw_idle()
    #     return

    def keep_line(self, name, color, line_id,):
        # lab = self.keep_buttons[owner.id]["dropdown"].value
        # The main line
        self.ax.lines.append(cp.copy(self.data_lines[name]))
        self.ax.lines[-1].set_url(line_id)
        self.ax.lines[-1].set_zorder(2)
        self.ax.lines[-1].set_label(None)
        if self.ax.lines[-1].get_marker() == "None":
            self.ax.lines[-1].set_color(color)
        else:
            self.ax.lines[-1].set_markerfacecolor(color)
            self.ax.lines[-1].set_markeredgecolor("None")

        # The masks
        # if len(self.masks[lab]) > 0:
        for m in self.mask_lines[name]:
            self.ax.lines.append(cp.copy(self.mask_lines[name][m]))
            self.ax.lines[-1].set_url(line_id)
            self.ax.lines[-1].set_gid(m)
            self.ax.lines[-1].set_zorder(3)
            if self.ax.lines[-1].get_marker() != "None":
                self.ax.lines[-1].set_zorder(3)
            else:
                self.ax.lines[-1].set_zorder(1)

        if self.errorbars[name]:
            err = self.error_lines[name].get_children()
            self.ax.collections.append(cp.copy(err[0]))
            self.ax.collections[-1].set_color(color)
            self.ax.collections[-1].set_url(line_id)
            self.ax.collections[-1].set_zorder(2)

        # for dim, val in self.slider.items():
        #     if not val.disabled:
        #         lab = "{},{}:{}".format(lab, dim, val.value)
        # self.keep_buttons[owner.id]["dropdown"].options = [lab]
        # self.keep_buttons[owner.id]["dropdown"].disabled = True
        # self.make_keep_button()
        # owner.description = "Remove"
        # self.update_button_box_widget()
        self.ax.legend()
        self.fig.canvas.draw_idle()
        return

    def remove_line(self, line_id):
        # del self.keep_buttons[owner.id]
        lines = []
        for line in self.ax.lines:
            if line.get_url() != line_id:
                lines.append(line)
        collections = []
        for coll in self.ax.collections:
            if coll.get_url() != line_id:
                collections.append(coll)
        self.ax.lines = lines
        self.ax.collections = collections
        # self.update_button_box_widget()
        self.fig.canvas.draw_idle()
        return

    def update_line_color(self, line_id, color):
        for line in self.ax.lines:
            if line.get_url() == line_id:
                if line.get_marker() == 'None':
                    line.set_color(color)
                else:
                    line.set_markerfacecolor(color)

        for coll in self.ax.collections:
            if coll.get_url() == line_id:
                coll.set_color(color)
        self.fig.canvas.draw_idle()
        return

    # def update_data(self):


    def change_segments_y(self, x, y, e):
        # e = vars_to_err(e)
        arr1 = np.repeat(x, 2)
        arr2 = np.array([y - e, y + e]).T.flatten()
        return np.array([arr1, arr2]).T.flatten().reshape(len(y), 2, 2)

    def toggle_mask(self, mask_group, mask_name, value):
        # msk = self.members["masks"][change["owner"].masks_group][
        #     change["owner"].masks_name]
        msk = self.mask_lines[mask_group][mask_name]
        if msk.get_gid() == "onaxes":
            msk.set_visible(value)
        # Also toggle masks on additional lines created by keep button
        for line in self.ax.lines:
            if line.get_gid() == mask_name:
                line.set_visible(value)
        self.fig.canvas.draw_idle()
        return

    def rescale_to_data(self):
        # self.ax.set_ylim(get_ylim())
        self.ax.autoscale(True)
        self.ax.relim()
        self.ax.autoscale_view()
        self.fig.canvas.draw_idle()
        return

    # def vars_to_err(self, v):
    #     with np.errstate(invalid="ignore"):
    #         v = np.sqrt(v)
    #     non_finites = np.where(np.logical_not(np.isfinite(v)))
    #     v[non_finites] = 0.0
    #     return v
