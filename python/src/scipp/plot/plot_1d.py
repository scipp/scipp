# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import config
from .render import render_plot
from .slicer import Slicer
from .tools import to_bin_centers
from .._utils import name_with_unit
from .._scipp import core as sc

# Other imports
import numpy as np
import copy as cp
import matplotlib.pyplot as plt
import ipywidgets as widgets
import warnings


def plot_1d(scipp_obj_dict=None,
            axes=None,
            errorbars=None,
            masks={"color": "k"},
            filename=None,
            figsize=None,
            ax=None,
            mpl_line_params=None,
            logx=False,
            logy=False,
            logxy=False,
            grid=False):
    """
    Plot a 1D spectrum.

    Input is a Dataset containing one or more Variables.
    If the coordinate of the x-axis contains bin edges, then a bar plot is
    made.
    If the data contains more than one dimensions, sliders are added.

    """

    sv = Slicer1d(scipp_obj_dict=scipp_obj_dict,
                  axes=axes,
                  errorbars=errorbars,
                  masks=masks,
                  ax=ax,
                  mpl_line_params=mpl_line_params,
                  logx=logx or logxy,
                  logy=logy or logxy,
                  grid=grid)

    if ax is None:
        render_plot(figure=sv.fig, widgets=sv.box, filename=filename)

    return sv.members


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

        super().__init__(scipp_obj_dict=scipp_obj_dict,
                         axes=axes,
                         masks=masks,
                         button_options=['X'])

        self.scipp_obj_dict = scipp_obj_dict
        self.fig = None
        self.ax = ax
        # self.ax.set_title('plot_1d 0')
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
        # self.ax.set_title('plot_1d 1')

        # Determine whether error bars should be plotted or not
        self.errorbars = {}
        for name, var in self.scipp_obj_dict.items():
            if var.unaligned is not None:
                self.errorbars[name] = var.unaligned.variances is not None
                self.input_contains_unaligned_data = True
            else:
                self.errorbars[name] = var.variances is not None
        if errorbars is not None:
            if isinstance(errorbars, bool):
                for name, var in self.scipp_obj_dict.items():
                    self.errorbars[name] &= errorbars
            elif isinstance(errorbars, dict):
                for name, v in errorbars.items():
                    if name in self.scipp_obj_dict:
                        self.errorbars[
                            name] = errorbars[name] and self.scipp_obj_dict[
                                name].variances is not None
                    else:
                        print("Warning: key {} was not found in list of "
                              "entries to plot and will be ignored.".format(
                                  name))
            else:
                raise TypeError("Unsupported type for argument "
                                "'errorbars': {}".format(type(errorbars)))
        # self.ax.set_title('plot_1d 2')

        # Initialise container for returning matplotlib objects
        self.members.update({
            "lines": {},
            "error_x": {},
            "error_y": {},
            "error_xy": {}
        })
        # Save the line parameters (color, linewidth...)
        self.mpl_line_params = mpl_line_params
        # self.ax.set_title('plot_1d 3')

        self.names = []
        self.ylim = [np.Inf, np.NINF]
        self.logx = logx
        self.logy = logy
        for name, var in self.scipp_obj_dict.items():
            self.names.append(name)
            if var.values is not None:
                self.ylim = self.get_ylim(var=var,
                                          ymin=self.ylim[0],
                                          ymax=self.ylim[1],
                                          errorbars=self.errorbars[name])
            ylab = name_with_unit(var=var, name="")

        if (not self.mpl_axes) and (var.values is not None):
            with warnings.catch_warnings():
                warnings.filterwarnings("ignore", category=UserWarning)
                self.ax.set_ylim(self.ylim)
        # self.ax.set_title('plot_1d 4')

        if self.logx:
            self.ax.set_xscale("log")
        if self.logy:
            self.ax.set_yscale("log")
        # self.ax.set_title('plot_1d 5')

        # Disable buttons
        for dim, button in self.buttons.items():
            if self.slider[dim].disabled:
                button.disabled = True
        # self.ax.set_title('plot_1d 5.5')
        self.update_axes(list(self.slider.keys())[-1])
        # self.ax.set_title('plot_1d 5.6')

        self.ax.set_ylabel(ylab)
        if len(self.ax.get_legend_handles_labels()[0]) > 0:
            self.ax.legend()
        # self.ax.set_title('plot_1d 6')

        self.keep_buttons = dict()
        if self.ndim > 1:
            self.make_keep_button()

        # vbox contains the original sliders and buttons. In mbox, we include
        # the keep trace buttons.
        self.mbox = self.vbox.copy()
        for key, val in self.keep_buttons.items():
            self.mbox.append(widgets.HBox(val))
        self.box = widgets.VBox(self.mbox)
        self.box.layout.align_items = 'center'

        # Populate the members
        self.members["fig"] = self.fig
        self.members["ax"] = self.ax

        return

    def get_finite_y(self, arr):
        if self.logy:
            with np.errstate(divide="ignore", invalid="ignore"):
                arr = np.log10(arr, out=arr)
        subset = np.where(np.isfinite(arr))
        return arr[subset]

    def get_ylim(self, var=None, ymin=None, ymax=None, errorbars=False):
        if errorbars:
            err = self.vars_to_err(var.variances)
        else:
            err = 0.0

        ymin_new = np.amin(self.get_finite_y(var.values - err))
        ymax_new = np.amax(self.get_finite_y(var.values + err))

        dy = 0.05 * (ymax_new - ymin_new)
        ymin_new -= dy
        ymax_new += dy
        if self.logy:
            ymin_new = 10.0**ymin_new
            ymax_new = 10.0**ymax_new
        return [min(ymin, ymin_new), max(ymax, ymax_new)]

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
        self.keep_buttons[key] = [drop, but, col]
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
        self.mbox = self.vbox.copy()
        for k, b in self.keep_buttons.items():
            self.mbox.append(widgets.HBox(b))
        self.box.children = tuple(self.mbox)
        return

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

        # if self.masks is not None:
        #     mslice = self.slice_masks().values
        # # self.ax.set_title('plot_1d 10')

        xmin = np.Inf
        xmax = np.NINF
        for name, var in self.scipp_obj_dict.items():
            # self.ax.set_title('plot_1d 11')
            new_x = self.slider_coord[name][dim].values
            xmin = min(new_x[0], xmin)
            xmax = max(new_x[-1], xmax)

            vslice = self.slice_data(var, name)
            ydata = vslice.values
            # self.ax.set_title('plot_1d 12')

            # If this is a histogram, plot a step function
            if self.histograms[name][dim][dim]:
                # self.ax.set_title('plot_1d 12.1')
                ye = np.concatenate((ydata[0:1], ydata))
                # # ye = sc.concatenate((vslice[dim, 0:1], vslice, dim))
                # print(vslice[dim, 0:1])
                # print(vslice)
                # if 
                # ye = sc.concatenate(vslice[dim, 0:1], vslice, dim)
                [self.members["lines"][name]
                 ] = self.ax.step(new_x,
                                  ye,
                                  label=name,
                                  zorder=10,
                                  **{
                                      key: self.mpl_line_params[key][name]
                                      for key in ["color", "linewidth"]
                                  })
                # Add masks if any
                if self.params["masks"][name]["show"]:
                    self.members["masks"][name] = {}
                    for m in self.masks[name]:
                        mdata = vslice.masks[m].values
                        me = np.concatenate((mdata[0:1], mdata))
                        # print(self.members["masks"][name][m])
                        # print(self.params["masks"])
                        [self.members["masks"][name][m]] = self.ax.step(
                            new_x,
                            self.mask_to_float(me, ye),
                            linewidth=self.mpl_line_params["linewidth"][name] * 3,
                            color=self.params["masks"][name]["color"],
                            zorder=9)

            else:
                # self.ax.set_title('plot_1d 12.2')

                # If this is not a histogram, just use normal plot
                [self.members["lines"][name]
                 ] = self.ax.plot(new_x,
                                  vslice.values,
                                  label=name,
                                  zorder=10,
                                  **{
                                      key: self.mpl_line_params[key][name]
                                      for key in self.mpl_line_params.keys()
                                  })
                # Add masks if any
                if self.params["masks"][name]["show"]:
                    self.members["masks"][name] = {}
                    for m in self.masks[name]:
                        [self.members["masks"][name][m]
                         ] = self.ax.plot(new_x,
                                          self.mask_to_float(vslice.masks[m].values, vslice.values),
                                          zorder=10,
                                          mec=self.params["masks"][name]["color"],
                                          mew=3,
                                          linestyle="none",
                                          **{
                                              key: self.mpl_line_params[key][name]
                                              for key in ["color", "marker"]
                                          })
            # self.ax.set_title('plot_1d 13')

            # Add error bars
            if self.errorbars[name]:
                if self.histograms[name][dim][dim]:
                    self.current_xcenters = to_bin_centers(
                        self.slider_coord[name][dim], dim).values
                else:
                    self.current_xcenters = new_x
                self.members["error_y"][name] = self.ax.errorbar(
                    self.current_xcenters,
                    ydata,
                    yerr=self.vars_to_err(vslice.variances),
                    color=self.mpl_line_params["color"][name],
                    zorder=10,
                    fmt="none")
        # self.ax.set_title('plot_1d 20')

        if not self.mpl_axes:
            deltax = 0.05 * (xmax - xmin)
            with warnings.catch_warnings():
                warnings.filterwarnings("ignore", category=UserWarning)
                self.ax.set_xlim([xmin - deltax, xmax + deltax])
                if self.input_contains_unaligned_data:
                    self.ax.set_ylim(self.ylim)

        self.ax.set_xlabel(
            name_with_unit(
                self.slider_label[self.name][dim]["coord"],
                name=self.slider_label[self.name][dim]["name"],
            ))
        self.ax.xaxis.set_major_formatter(
            self.slider_axformatter[self.name][dim][self.logx])
        self.ax.xaxis.set_major_locator(
            self.slider_axlocator[self.name][dim][self.logx])
        # self.ax.set_title('plot_1d 21')

        return

    def slice_data(self, var, name):
        vslice = var
        # Slice along dimensions with active sliders
        for dim, val in self.slider.items():
            if not val.disabled:
                self.lab[dim].value = self.make_slider_label(
                    self.slider_label[self.name][dim]["coord"], val.value)
                vslice = vslice[val.dim, val.value]
        if vslice.unaligned is not None:
            vslice = sc.histogram(vslice)
            self.ylim = self.get_ylim(var=vslice,
                                      ymin=self.ylim[0],
                                      ymax=self.ylim[1],
                                      errorbars=self.errorbars[name])
        return vslice

    def slice_masks(self):
        mslice = self.masks
        for dim, val in self.slider.items():
            if not val.disabled and (dim in mslice.dims):
                mslice = mslice[dim, val.value]
        return mslice

    # Define function to update slices
    def update_slice(self, change):
        # if self.masks is not None:
        #     mslice = self.slice_masks()
        for name, var in self.scipp_obj_dict.items():
            vslice = self.slice_data(var, name)
            # self.update_line(name, vslice)
            vals = vslice.values
            if self.histograms[name][self.button_axis_to_dim["x"]][
                    self.button_axis_to_dim["x"]]:
                vals = np.concatenate((vals[0:1], vals))
            self.members["lines"][name].set_ydata(vals)

            if self.params["masks"][name]["show"]:
                # for m in vslice.masks:
                msk = mslice.values
                if self.histograms[name][self.button_axis_to_dim["x"]][
                        self.button_axis_to_dim["x"]]:
                    msk = np.concatenate((msk[0:1], msk))
                self.members["masks"][name].set_ydata(
                    self.mask_to_float(msk, vals))
            if self.errorbars[name]:
                coll = self.members["error_y"][name].get_children()[0]
                coll.set_segments(
                    self.change_segments_y(self.current_xcenters,
                                           vslice.values, vslice.variances))
        if self.input_contains_unaligned_data and (not self.mpl_axes):
            with warnings.catch_warnings():
                warnings.filterwarnings("ignore", category=UserWarning)
                self.ax.set_ylim(self.ylim)

        return

    # def update_line(self, name, vslice):
    #     vals = vslice.values
    #     if self.histograms[name][self.button_axis_to_dim["x"]][
    #             self.button_axis_to_dim["x"]]:
    #         vals = np.concatenate((vals[0:1], vals))
    #     self.members["lines"][name].set_ydata(vals)

    #     if self.params["masks"][name]["show"]:
    #         msk = mslice.values
    #         if self.histograms[name][self.button_axis_to_dim["x"]][
    #                 self.button_axis_to_dim["x"]]:
    #             msk = np.concatenate((msk[0:1], msk))
    #         self.members["masks"][name].set_ydata(
    #             self.mask_to_float(msk, vals))
    #     if self.errorbars[name]:
    #         coll = self.members["error_y"][name].get_children()[0]
    #         coll.set_segments(
    #             self.change_segments_y(self.current_xcenters,
    #                                    vals, vslice.variances))

    def keep_remove_trace(self, owner):
        if owner.description == "Keep":
            self.keep_trace(owner)
        elif owner.description == "Remove":
            self.remove_trace(owner)
        return

    def keep_trace(self, owner):
        lab = self.keep_buttons[owner.id][0].value
        lines_to_keep = ["lines"]
        if self.params["masks"][lab]["show"]:
            lines_to_keep.append("masks")
        for line in lines_to_keep:
            self.ax.lines.append(cp.copy(self.members[line][lab]))
            self.ax.lines[-1].set_color(self.keep_buttons[owner.id][2].value)
            self.ax.lines[-1].set_url(owner.id)
            self.ax.lines[-1].set_zorder(1)
        if self.errorbars[lab]:
            err = self.members["error_y"][lab].get_children()
            self.ax.collections.append(cp.copy(err[0]))
            self.ax.collections[-1].set_color(
                self.keep_buttons[owner.id][2].value)
            self.ax.collections[-1].set_url(owner.id)
            self.ax.collections[-1].set_zorder(1)

        for dim, val in self.slider.items():
            if not val.disabled:
                lab = "{},{}:{}".format(lab, dim, val.value)
        self.keep_buttons[owner.id][0] = widgets.Label(
            value=lab, layout={'width': "initial"}, title=lab)
        self.make_keep_button()
        owner.description = "Remove"
        self.mbox = self.vbox.copy()
        for k, b in self.keep_buttons.items():
            self.mbox.append(widgets.HBox(b))
        self.box.children = tuple(self.mbox)
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
        self.fig.canvas.draw_idle()
        self.mbox = self.vbox.copy()
        for k, b in self.keep_buttons.items():
            self.mbox.append(widgets.HBox(b))
        self.box.children = tuple(self.mbox)
        return

    def update_trace_color(self, change):
        for line in self.ax.lines:
            if line.get_url() == change["owner"].id:
                line.set_color(change["new"])
        for coll in self.ax.collections:
            if coll.get_url() == change["owner"].id:
                coll.set_color(change["new"])
        self.fig.canvas.draw_idle()
        return

    def change_segments_y(self, x, y, e):
        e = self.vars_to_err(e)
        arr1 = np.repeat(x, 2)
        arr2 = np.array([y - e, y + e]).T.flatten()
        return np.array([arr1, arr2]).T.flatten().reshape(len(y), 2, 2)

    def toggle_mask(self, change):
        self.members["masks"][change["owner"].masks_group][change["owner"].masks_name].set_visible(change["new"])
        # for name in self.masks:
        #     for key in self.masks[name]:

        # for n, msk in self.members["masks"].items():
        #     msk.set_visible(change["new"])
        # change["owner"].description = "Hide masks" if change["new"] else \
        #     "Show masks"
        return

    def vars_to_err(self, v):
        with np.errstate(invalid="ignore"):
            v = np.sqrt(v)
        non_finites = np.where(np.logical_not(np.isfinite(v)))
        v[non_finites] = 0.0
        return v
