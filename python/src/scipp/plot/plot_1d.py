# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from ..config import plot as config
from .render import render_plot
from .slicer import Slicer
from .tools import edges_to_centers
from ..utils import name_with_unit

# Other imports
import numpy as np
import copy as cp
import matplotlib.pyplot as plt
import ipywidgets as widgets
import warnings


def plot_1d(scipp_obj_dict=None, output=None, axes=None, values=None,
            variances=None, masks={"color": "k"}, filename=None, figsize=None,
            mpl_axes=None, mpl_line_params=None, logx=False, logy=False,
            logxy=False):
    """
    Plot a 1D spectrum.

    Input is a Dataset containing one or more Variables.
    If the coordinate of the x-axis contains bin edges, then a bar plot is
    made.
    If the data contains more than one dimensions, sliders are added.

    """

    # Get the first entry in the dict of scipp objects
    _, data_array = next(iter(scipp_obj_dict.items()))
    if axes is None:
        axes = data_array.dims

    sv = Slicer1d(scipp_obj_dict=scipp_obj_dict, data_array=data_array,
                  axes=axes, values=values, variances=variances, masks=masks,
                  mpl_axes=mpl_axes, mpl_line_params=mpl_line_params,
                  logx=logx or logxy, logy=logy or logxy)

    if mpl_axes is None:
        render_plot(figure=sv.fig, widgets=sv.box, filename=filename,
                    output=output)

    return sv.members


class Slicer1d(Slicer):

    def __init__(self, scipp_obj_dict=None, data_array=None, axes=None,
                 values=None, variances=None, masks=None, mpl_axes=None,
                 mpl_line_params=None, logx=False, logy=False):

        super().__init__(scipp_obj_dict=scipp_obj_dict, data_array=data_array,
                         axes=axes, values=values, variances=variances,
                         masks=masks, button_options=['X'])

        self.scipp_obj_dict = scipp_obj_dict
        self.fig = None
        self.mpl_axes = mpl_axes
        if self.mpl_axes is not None:
            self.ax = self.mpl_axes
        else:
            self.fig, self.ax = plt.subplots(
                1, 1, figsize=(config.width/config.dpi,
                               config.height/config.dpi),
                dpi=config.dpi)

        # Initialise container for returning matplotlib objects
        self.members.update({"lines": {}, "error_x": {}, "error_y": {},
                             "error_xy": {}})
        # Save the line parameters (color, linewidth...)
        self.mpl_line_params = mpl_line_params

        self.names = []
        ymin = 1.0e30
        ymax = -1.0e30
        for i, (name, var) in enumerate(sorted(self.scipp_obj_dict.items())):
            self.names.append(name)
            if var.variances is not None:
                err = np.sqrt(var.variances)
            else:
                err = 0.0

            if logy:
                with np.errstate(divide="ignore", invalid="ignore"):
                    arr = np.log10(var.values - err)
                    subset = np.where(np.isfinite(arr))
                    ymin = min(ymin, np.amin(arr[subset]))
                    arr = np.log10(var.values + err)
                    subset = np.where(np.isfinite(arr))
                    ymax = max(ymax, np.amax(arr[subset]))
            else:
                ymin = min(ymin, np.nanmin(var.values - err))
                ymax = max(ymax, np.nanmax(var.values + err))
            ylab = name_with_unit(var=var, name="")

        if self.mpl_axes is None:
            dy = 0.05*(ymax - ymin)
            yrange = [ymin-dy, ymax+dy]
            if logy:
                yrange = 10.0**np.array(yrange)
            with warnings.catch_warnings():
                warnings.filterwarnings("ignore", category=UserWarning)
                self.ax.set_ylim(yrange)

        if logx:
            self.ax.set_xscale("log")
        if logy:
            self.ax.set_yscale("log")

        # Disable buttons
        for dim, button in self.buttons.items():
            if self.slider[dim].disabled:
                button.disabled = True
        self.update_axes(list(self.slider.keys())[-1])

        self.ax.set_ylabel(ylab)
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

    def make_keep_button(self):
        drop = widgets.Dropdown(options=self.names, description='',
                                layout={'width': 'initial'})
        but = widgets.Button(description="Keep", disabled=False,
                             button_style="", layout={'width': "70px"})
        # Generate a random color. TODO: should we initialise the seed?
        col = widgets.ColorPicker(
            concise=True, description='',
            value='#%02X%02X%02X' % (tuple(np.random.randint(0, 255, 3))),
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
        if self.mpl_axes is None:
            self.ax.lines = []
            self.ax.collections = []
            self.members.update({"lines": {}, "error_x": {}, "error_y": {},
                                 "error_xy": {}, "masks": {}})

        new_x = self.slider_x[dim].values
        xc = edges_to_centers(new_x)

        if self.params["masks"]["show"]:
            mslice = self.slice_masks()

        for i, (name, var) in enumerate(sorted(self.scipp_obj_dict.items())):
            vslice = self.slice_data(var)

            # If this is a histogram, plot a step function
            if self.histograms[name][dim]:
                ye = np.concatenate(([0], vslice.values))
                [self.members["lines"][name]] = self.ax.step(
                    new_x, ye, label=name, zorder=10,
                    **{key: self.mpl_line_params[key][i] for key in
                       ["color", "linewidth"]})
                # Add masks if any
                if self.params["masks"]["show"]:
                    me = np.concatenate(([False], mslice.values))
                    [self.members["masks"][name]] = self.ax.step(
                        new_x, self.mask_to_float(me, ye),
                        linewidth=self.mpl_line_params["linewidth"][i]*3,
                        color=self.params["masks"]["color"], zorder=9)

            else:

                # If this is not a histogram, just use normal plot
                [self.members["lines"][name]] = self.ax.plot(
                    new_x, vslice.values, label=name, zorder=10,
                    **{key: self.mpl_line_params[key][i] for key in
                       self.mpl_line_params.keys()})
                # Add masks if any
                if self.params["masks"]["show"]:
                    [self.members["masks"][name]] = self.ax.plot(
                        new_x,
                        self.mask_to_float(mslice.values, vslice.values),
                        zorder=10, mec=self.params["masks"]["color"], mew=3,
                        linestyle="none",
                        **{key: self.mpl_line_params[key][i] for key in
                           ["color", "marker"]})

            # Add error bars
            if var.variances is not None:
                if self.histograms[name][dim]:
                    self.members["error_y"][name] = self.ax.errorbar(
                        xc, vslice.values, yerr=np.sqrt(vslice.variances),
                        color=self.mpl_line_params["color"][i], zorder=10,
                        fmt="none")
                else:
                    self.members["error_y"][name] = self.ax.errorbar(
                        new_x, vslice.values, yerr=np.sqrt(vslice.variances),
                        color=self.mpl_line_params["color"][i], zorder=10,
                        fmt="none")

        if self.mpl_axes is None:
            deltax = 0.05 * (new_x[-1] - new_x[0])
            with warnings.catch_warnings():
                warnings.filterwarnings("ignore", category=UserWarning)
                self.ax.set_xlim([new_x[0] - deltax, new_x[-1] + deltax])
        self.ax.set_xlabel(name_with_unit(self.slider_x[dim],
                                          name=self.slider_labels[dim]))
        if self.slider_ticks[dim] is not None:
            self.ax.set_xticklabels(self.get_custom_ticks(self.ax, dim))
        return

    def slice_data(self, var):
        vslice = var
        # Slice along dimensions with active sliders
        for dim, val in self.slider.items():
            if not val.disabled:
                self.lab[dim].value = self.make_slider_label(
                    self.slider_x[dim], val.value)
                vslice = vslice[val.dim, val.value]
        return vslice

    def slice_masks(self):
        mslice = self.masks
        for dim, val in self.slider.items():
            if not val.disabled and (dim in mslice.dims):
                mslice = mslice[dim, val.value]
        return mslice

    # Define function to update slices
    def update_slice(self, change):
        if self.params["masks"]["show"]:
            mslice = self.slice_masks()
        for i, (name, var) in enumerate(sorted(self.scipp_obj_dict.items())):
            vslice = self.slice_data(var)
            vals = vslice.values
            if self.histograms[name][self.button_axis_to_dim["x"]]:
                vals = np.concatenate(([0], vals))
            self.members["lines"][name].set_ydata(vals)

            if self.params["masks"]["show"]:
                msk = mslice.values
                if self.histograms[name][self.button_axis_to_dim["x"]]:
                    msk = np.concatenate(([False], msk))
                self.members["masks"][name].set_ydata(
                    self.mask_to_float(msk, vals))
            if var.variances is not None:
                coll = self.members["error_y"][name].get_children()[0]
                coll.set_segments(
                    self.change_segments_y(
                        coll.get_segments(),
                        vslice.values,
                        np.sqrt(vslice.variances)))
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
        if self.params["masks"]["show"]:
            lines_to_keep.append("masks")
        for l in lines_to_keep:
            self.ax.lines.append(cp.copy(self.members[l][lab]))
            self.ax.lines[-1].set_color(self.keep_buttons[owner.id][2].value)
            self.ax.lines[-1].set_url(owner.id)
            self.ax.lines[-1].set_zorder(1)
        if self.scipp_obj_dict[lab].variances is not None:
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

    def generate_new_segments(self, x, y):
        arr1 = np.array([x, x]).T.flatten()
        arr2 = np.array([y, y]).T.flatten()
        return np.array([arr1, arr2]).T.flatten().reshape(len(x), 2, 2)

    def change_segments_y(self, s, y, e):
        # TODO: this can be optimized to substitute the y values in-place in
        # the original segments array
        arr1 = np.array(s).flatten()[::2]
        arr2 = np.array([y-np.sqrt(e), y+np.sqrt(e)]).T.flatten()
        return np.array([arr1, arr2]).T.flatten().reshape(len(y), 2, 2)

    def toggle_masks(self, change):
        for n, msk in self.members["masks"].items():
            msk.set_visible(change["new"])
        change["owner"].description = "Hide masks" if change["new"] else \
            "Show masks"
        return
