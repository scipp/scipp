# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import config
from .controller import PlotController
from .controller1d import PlotController1d
from .model1d import PlotModel1d
# from .render import render_plot
# from .profiler import Profiler
from .tools import to_bin_edges, parse_params
from .view1d import PlotView1d
from .._utils import name_with_unit
from .._scipp import core as sc

# Other imports
import numpy as np
import copy as cp
import matplotlib.pyplot as plt
import ipywidgets as ipw
import warnings


def plot1d(scipp_obj_dict=None,
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
            grid=False,
            title=None):
    """
    Plot a 1D spectrum.

    Input is a Dataset containing one or more Variables.
    If the coordinate of the x-axis contains bin edges, then a bar plot is
    made.
    If the data contains more than one dimensions, sliders are added.

    """

    sp = SciPlot1d(scipp_obj_dict=scipp_obj_dict,
                  axes=axes,
                  errorbars=errorbars,
                  masks=masks,
                  ax=ax,
                  mpl_line_params=mpl_line_params,
                  logx=logx or logxy,
                  logy=logy or logxy,
                  grid=grid,
                  title=title)

    if filename is not None:
        sp.savefig(filename)

    return sp



class SciPlot1d():
    def __init__(self,
                 scipp_obj_dict=None,
                 axes=None,
                 errorbars=None,
                 masks=None,
                 ax=None,
                 mpl_line_params=None,
                 logx=False,
                 logy=False,
                 grid=False,
                 title=None):



        self.controller = PlotController(scipp_obj_dict=scipp_obj_dict,
                         axes=axes,
                         masks=masks,
                         logx=logx,
                         logy=logy,
                         errorbars=errorbars,
            button_options=['X'])

        self.controller1d = PlotController1d(ndim=self.controller.ndim,
            data_names=list(scipp_obj_dict.keys()))


        self.model = PlotModel1d(controller=self.controller,
            scipp_obj_dict=scipp_obj_dict)

        # Connect controller to model
        self.controller.model = self.model



        # self.controller1d = PlotController1d(ndim=self.controller.ndim,
        #     data_names=list(scipp_obj_dict.keys()))


        self.view = PlotView1d(controller=self.controller,
            ax=ax,
            errorbars=self.controller.errorbars,
            title=title,
            unit=self.controller.params["values"][self.controller.name]["unit"],
            mask_params=self.controller.params["masks"][self.controller.name],
            mask_names=self.controller.mask_names,
            logx=logx,
            logy=logy,
            mpl_line_params=mpl_line_params,
            grid=grid)

        self.controller.view = self.view
        self.controller1d.view = self.view
        self.controller.slave = self.controller1d

        # Profile view
        self.profile = None


        # Call update_slice once to make the initial image
        self.controller.update_axes()


        # # Save the line parameters (color, linewidth...)
        # self.mpl_line_params = mpl_line_params

        # self.engine.update_axes(list(self.widgets.slider.keys())[-1])

        # self.names = []
        # # self.ylim = [np.Inf, np.NINF]
        # self.logx = logx
        # self.logy = logy
        # for name, var in self.data_arrays.items():
        #     self.names.append(name)
        #     # if var.values is not None:
        #     #     self.ylim = get_ylim(var=var,
        #     #                               ymin=self.ylim[0],
        #     #                               ymax=self.ylim[1],
        #     #                               errorbars=self.errorbars[name],
        #     #                               logy=self.logy)
        #     ylab = name_with_unit(var=var, name="")

        # # if (not self.mpl_axes) and (var.values is not None):
        # #     with warnings.catch_warnings():
        # #         warnings.filterwarnings("ignore", category=UserWarning)
        # #         self.ax.set_ylim(self.ylim)

        # if self.logx:
        #     self.ax.set_xscale("log")
        # if self.logy:
        #     self.ax.set_yscale("log")

        # # Disable buttons
        # for dim, button in self.buttons.items():
        #     if self.slider[dim].disabled:
        #         button.disabled = True
        # self.update_axes(list(self.slider.keys())[-1])

        # self.ax.set_ylabel(ylab)
        # if len(self.ax.get_legend_handles_labels()[0]) > 0:
        #     self.ax.legend()

        # self.widget_view = ipw.VBox()
        # self.keep_buttons = dict()
        # self.make_keep_button()

        # self.additional_widgets = None

        # vbox contains the original sliders and buttons.
        # In keep_buttons_box, we include the keep trace buttons.
        # self.additional_widgets = ipw.VBox()
        # for key, val in self.keep_buttons.items():
        #     self.additional_widgets.append(widgets.HBox(list(val.values())))
        # self.additional_widgets = widgets.VBox(self.additional_widgets)

        # self.update_additional_widgets()

        # self.box = widgets.VBox(
        #     [widgets.VBox(self.vbox), self.additional_widgets])

        # self.box.layout.align_items = 'center'
        # if self.controller.ndim < 2:
        #     self.additional_widgets.layout.display = 'none'

        # self.additional_widgets = 


        # # Populate the members
        # self.members["fig"] = self.fig
        # self.members["ax"] = self.ax

        return


    def _ipython_display_(self):
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        # widgets_ = [self.figure, self.widgets]
        # if self.overview["additional_widgets"] is not None:
        #     wdgts.append(self.overview["additional_widgets"])
        return ipw.VBox([self.view._to_widget(), self.controller._to_widget(),
            self.controller1d._to_widget()])

    def savefig(self, filename=None):
        self.view.savefig(filename=filename)

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

