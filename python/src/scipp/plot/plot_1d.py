# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import config
from .render import render_plot
from .slicer import Slicer
from .tools import edges_to_centers
from .._utils import name_with_unit
from .._scipp.core import histogram as scipp_histogram

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
        self.mpl_axes = False
        self.input_contains_unaligned_data = False
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

        # Initialise container for returning matplotlib objects
        self.members.update({
            "lines": {},
            "error_x": {},
            "error_y": {},
            "error_xy": {}
        })
        # Save the line parameters (color, linewidth...)
        self.mpl_line_params = mpl_line_params

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

        if self.logx:
            self.ax.set_xscale("log")
        if self.logy:
            self.ax.set_yscale("log")

        # Disable buttons
        for dim, button in self.buttons.items():
            if self.slider[dim].disabled:
                button.disabled = True
        self.update_axes(list(self.slider.keys())[-1])

        self.ax.set_ylabel(ylab)
        if len(self.ax.get_legend_handles_labels()[0]) > 0:
            self.ax.legend()

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

    def get_ylim(self, var=None, ymin=None, ymax=None, errorbars=False):
        if errorbars:
            err = np.sqrt(var.variances)
        else:
            err = 0.0

        if self.logy:
            with np.errstate(divide="ignore", invalid="ignore"):
                arr = np.log10(var.values - err)
                subset = np.where(np.isfinite(arr))
                ymin_new = np.amin(arr[subset])
                arr = np.log10(var.values + err)
                subset = np.where(np.isfinite(arr))
                ymax_new = np.amax(arr[subset])
        else:
            ymin_new = np.nanmin(var.values - err)
            ymax_new = np.nanmax(var.values + err)

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

        if self.masks is not None:
            mslice = self.slice_masks().values

        xmin = np.Inf
        xmax = np.NINF
        for name, var in self.scipp_obj_dict.items():
            new_x = self.slider_x[name][dim].values
            xmin = min(new_x[0], xmin)
            xmax = max(new_x[-1], xmax)

            vslice = self.slice_data(var, name)
            ydata = vslice.values

            # If this is a histogram, plot a step function
            if self.histograms[name][dim]:
                ye = np.concatenate((ydata[0:1], ydata))
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
                    me = np.concatenate((mslice[0:1], mslice))
                    [self.members["masks"][name]] = self.ax.step(
                        new_x,
                        self.mask_to_float(me, ye),
                        linewidth=self.mpl_line_params["linewidth"][name] * 3,
                        color=self.params["masks"][name]["color"],
                        zorder=9)

            else:

                # If this is not a histogram, just use normal plot
                [self.members["lines"][name]
                 ] = self.ax.plot(new_x,
                                  ydata,
                                  label=name,
                                  zorder=10,
                                  **{
                                      key: self.mpl_line_params[key][name]
                                      for key in self.mpl_line_params.keys()
                                  })
                # Add masks if any
                if self.params["masks"][name]["show"]:
                    [self.members["masks"][name]
                     ] = self.ax.plot(new_x,
                                      self.mask_to_float(mslice, ydata),
                                      zorder=10,
                                      mec=self.params["masks"][name]["color"],
                                      mew=3,
                                      linestyle="none",
                                      **{
                                          key: self.mpl_line_params[key][name]
                                          for key in ["color", "marker"]
                                      })

            # Add error bars
            if self.errorbars[name]:
                if self.histograms[name][dim]:
                    err_x = edges_to_centers(new_x)
                else:
                    err_x = new_x
                self.members["error_y"][name] = self.ax.errorbar(
                    err_x,
                    ydata,
                    yerr=np.sqrt(vslice.variances),
                    color=self.mpl_line_params["color"][name],
                    zorder=10,
                    fmt="none")

        if not self.mpl_axes:
            deltax = 0.05 * (xmax - xmin)
            with warnings.catch_warnings():
                warnings.filterwarnings("ignore", category=UserWarning)
                self.ax.set_xlim([xmin - deltax, xmax + deltax])
                if self.input_contains_unaligned_data:
                    self.ax.set_ylim(self.ylim)

        self.ax.set_xlabel(
            name_with_unit(self.slider_x[self.name][dim], name=str(dim)))
        self.ax.xaxis.set_major_formatter(
            self.slider_axformatter[self.name][dim][self.logx])
        self.ax.xaxis.set_major_locator(
            self.slider_axlocator[self.name][dim][self.logx])

        return

    def slice_data(self, var, name):
        vslice = var
        # Slice along dimensions with active sliders
        for dim, val in self.slider.items():
            if not val.disabled:
                self.lab[dim].value = self.make_slider_label(
                    self.slider_x[self.name][dim], val.value)
                vslice = vslice[val.dim, val.value]
        if vslice.unaligned is not None:
            vslice = scipp_histogram(vslice)
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
        if self.masks is not None:
            mslice = self.slice_masks()
        for name, var in self.scipp_obj_dict.items():
            vslice = self.slice_data(var, name)
            vals = vslice.values
            if self.histograms[name][self.button_axis_to_dim["x"]]:
                vals = np.concatenate((vals[0:1], vals))
            self.members["lines"][name].set_ydata(vals)

            if self.params["masks"][name]["show"]:
                msk = mslice.values
                if self.histograms[name][self.button_axis_to_dim["x"]]:
                    msk = np.concatenate((msk[0:1], msk))
                self.members["masks"][name].set_ydata(
                    self.mask_to_float(msk, vals))
            if self.errorbars[name]:
                coll = self.members["error_y"][name].get_children()[0]
                coll.set_segments(
                    self.change_segments_y(coll.get_segments(), vslice.values,
                                           np.sqrt(vslice.variances)))
        if self.input_contains_unaligned_data and (not self.mpl_axes):
            with warnings.catch_warnings():
                warnings.filterwarnings("ignore", category=UserWarning)
                self.ax.set_ylim(self.ylim)

        return

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

    def change_segments_y(self, s, y, e):
        # TODO: this can be optimized to substitute the y values in-place in
        # the original segments array
        arr1 = np.array(s).flatten()[::2]
        arr2 = np.array([y - np.sqrt(e), y + np.sqrt(e)]).T.flatten()
        return np.array([arr1, arr2]).T.flatten().reshape(len(y), 2, 2)

    def toggle_masks(self, change):
        for n, msk in self.members["masks"].items():
            msk.set_visible(change["new"])
        change["owner"].description = "Hide masks" if change["new"] else \
            "Show masks"
        return
