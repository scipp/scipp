# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import config
from .figure import get_mpl_axes
from .tools import get_line_param

# Other imports
import numpy as np
import copy as cp
import matplotlib.pyplot as plt
import ipywidgets as ipw
import warnings
import io


class LinePlot:
    def __init__(self,
                 errorbars=None,
                 ax=None,
                 mpl_line_params=None,
                 title=None,
                 unit=None,
                 logx=False,
                 logy=False,
                 grid=False,
                 mask_params=None,
                 masks=None,
                 figsize=None,
                 picker=None,
                 is_profile=False):

        # Matplotlib line containers
        self.data_lines = {}
        self.mask_lines = {}
        self.error_lines = {}

        self.errorbars = errorbars
        self.masks = masks
        self.mask_params = mask_params
        self.picker = picker
        self.is_profile = is_profile
        self.slice_area = None
        self.logx = logx
        self.logy = logy
        self.unit = unit

        # Get matplotlib figure and axes
        self.fig, self.ax, _, self.own_axes = get_mpl_axes(ax=ax,
                                                           figsize=figsize)
        # self.fig = None
        # self.ax = ax
        # self.mpl_axes = False
        # self.current_xcenters = None
        # if self.ax is None:
        #     if figsize is None:
        #         figsize = (config.plot.width / config.plot.dpi,
        #                    config.plot.height / config.plot.dpi)
        #     self.fig, self.ax = plt.subplots(1,
        #                                      1,
        #                                      figsize=figsize,
        #                                      dpi=config.plot.dpi)
        # else:
        #     self.mpl_axes = True
        self.grid = grid

        if self.own_axes:
            self.fig.tight_layout(rect=config.plot.padding)

        # Save the line parameters (color, linewidth...)
        self.mpl_line_params = mpl_line_params

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

    def toggle_view(self, visible=True):
        if hasattr(self.fig.canvas, "layout"):
            self.fig.canvas.layout.display = None if visible else 'none'

    def update_axes(self,
                    axparams=None,
                    axformatter=None,
                    axlocator=None,
                    logx=False,
                    logy=False,
                    clear=True):
        if self.own_axes:
            self.ax.clear()

        if self.mpl_line_params is None:
            self.mpl_line_params = {
                "color": {},
                "marker": {},
                "linestyle": {},
                "linewidth": {}
            }
            for i, name in enumerate(axparams["x"]["hist"]):
                self.mpl_line_params["color"][name] = get_line_param(
                    "color", i)
                self.mpl_line_params["marker"][name] = get_line_param(
                    "marker", i)
                self.mpl_line_params["linestyle"][name] = get_line_param(
                    "linestyle", i)
                self.mpl_line_params["linewidth"][name] = get_line_param(
                    "linewidth", i)

        if self.logx:
            self.ax.set_xscale("log")
        if self.logy:
            self.ax.set_yscale("log")

        self.ax.set_ylabel(self.unit)

        if self.grid:
            self.ax.grid()

        deltax = 0.05 * (axparams["x"]["lims"][1] - axparams["x"]["lims"][0])
        with warnings.catch_warnings():
            warnings.filterwarnings("ignore", category=UserWarning)
            self.ax.set_xlim([
                axparams["x"]["lims"][0] - deltax,
                axparams["x"]["lims"][1] + deltax
            ])

        self.ax.set_xlabel(axparams["x"]["label"])

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
                [self.data_lines[name]] = self.ax.step(
                    [1, 2], [1, 2],
                    label=label,
                    zorder=10,
                    picker=self.picker,
                    **{
                        key: self.mpl_line_params[key][name]
                        for key in ["color", "linewidth"]
                    })
                for m in self.masks[name]:
                    [self.mask_lines[name][m]] = self.ax.step(
                        [1, 2], [1, 2],
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
                [self.data_lines[name]] = self.ax.plot(
                    [1, 2], [1, 2],
                    label=label,
                    zorder=10,
                    picker=self.picker,
                    **{
                        key: self.mpl_line_params[key][name]
                        for key in self.mpl_line_params.keys()
                    })
                for m in self.masks[name]:
                    [self.mask_lines[name][m]] = self.ax.plot(
                        [1, 2], [1, 2],
                        zorder=11,
                        mec=self.mask_params["color"],
                        mfc="None",
                        mew=3.0,
                        linestyle="none",
                        marker=self.mpl_line_params["marker"][name])
                    self.mask_lines[name][m].set_gid("onaxes")

            self.data_lines[name].set_url(name)

            # Add error bars
            if self.errorbars[name]:
                self.error_lines[name] = self.ax.errorbar(
                    [1, 2], [1, 2],
                    yerr=[1, 1],
                    color=self.mpl_line_params["color"][name],
                    zorder=10,
                    fmt="none")

        if self.is_profile:
            self.slice_area = self.ax.axvspan(1,
                                              2,
                                              alpha=0.5,
                                              color='lightgrey')

        self.ax.legend()
        self.fig.canvas.draw_idle()

    def update_data(self, new_values):

        for name, vals in new_values.items():

            self.data_lines[name].set_data(vals["values"]["x"],
                                           vals["values"]["y"])

            for m in vals["masks"]:
                self.mask_lines[name][m].set_data(vals["values"]["x"],
                                                  vals["masks"][m])

            if self.errorbars[name]:
                coll = self.error_lines[name].get_children()[0]
                coll.set_segments(
                    self.change_segments_y(vals["variances"]["x"],
                                           vals["variances"]["y"],
                                           vals["variances"]["e"]))

        self.fig.canvas.draw_idle()

    def keep_line(self, name, color, line_id):
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

        self.ax.legend()
        self.fig.canvas.draw_idle()

    def remove_line(self, line_id):
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
        self.fig.canvas.draw_idle()

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

    def change_segments_y(self, x, y, e):
        arr1 = np.repeat(x, 2)
        arr2 = np.array([y - e, y + e]).T.flatten()
        return np.array([arr1, arr2]).T.flatten().reshape(len(y), 2, 2)

    def toggle_mask(self, mask_group, mask_name, value):
        msk = self.mask_lines[mask_group][mask_name]
        if msk.get_gid() == "onaxes":
            msk.set_visible(value)
        # Also toggle masks on additional lines created by keep button
        for line in self.ax.lines:
            if line.get_gid() == mask_name:
                line.set_visible(value)
        self.fig.canvas.draw_idle()

    def rescale_to_data(self):
        # if ylim is None:
        self.ax.autoscale(True)
        self.ax.relim()
        self.ax.autoscale_view()
        # else:
        #     self.ax.set_ylim(ylim)
        self.fig.canvas.draw_idle()
