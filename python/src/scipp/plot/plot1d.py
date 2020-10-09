# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import config
from .controller1d import PlotController1d
from .model1d import PlotModel1d
from .panel1d import PlotPanel1d
from .profile import ProfileView
from .sciplot import SciPlot
from .view1d import PlotView1d
from .widgets import PlotWidgets

def plot1d(*args,
           masks={"color": "k"},
           filename=None,
           **kwargs):
           # figsize=None,
           # ax=None,
           # mpl_line_params=None,
           # logx=False,
           # logy=False,
           # logxy=False,
           # grid=False,
           # title=None):
    """
    Plot a 1D spectrum.

    Input is a Dataset containing one or more Variables.
    If the coordinate of the x-axis contains bin edges, then a bar plot is
    made.
    If the data contains more than one dimensions, sliders are added.
    """

    sp = SciPlot1d(*args, masks=masks, **kwargs)
      # scipp_obj_dict=scipp_obj_dict,
      #              axes=axes,
      #              errorbars=errorbars,
      #              masks=masks,
      #              ax=ax,
      #              mpl_line_params=mpl_line_params,
      #              logx=logx or logxy,
      #              logy=logy or logxy,
      #              grid=grid,
      #              title=title)

    if filename is not None:
        sp.savefig(filename)

    return sp


class SciPlot1d(SciPlot):
    def __init__(self,
                 scipp_obj_dict=None,
                 axes=None,
                 errorbars=None,
                 masks=None,
                 ax=None,
                 pax=None,
                 figsize=None,
                 mpl_line_params=None,
                 norm=None,
                 vmin=None,
                 vmax=None,
                 scale=None,
                 # logx=False,
                 # logy=False,
                 grid=False,
                 title=None):

        super().__init__(scipp_obj_dict=scipp_obj_dict,
                 axes=axes,
                 # cmap=cmap,
                 norm=norm,
                 vmin=vmin,
                 vmax=vmax,
                 # color=color,
                 errorbars=errorbars,
                 masks=masks,
                 view_ndims=1)


        self.widgets = PlotWidgets(axes=self.axes,
                                   ndim=self.ndim,
                                   name=self.name,
                                   dim_to_shape=self.dim_to_shape,
                                   masks=self.masks,
                                   button_options=['X'])

        # The model which takes care of all heavy calculations
        self.model = PlotModel1d(scipp_obj_dict=scipp_obj_dict,
                                 axes=self.axes,
                                 name=self.name,
                                 dim_to_shape=self.dim_to_shape,
                                 dim_label_map=self.dim_label_map)


        # # The main controller module which contains the slider widgets
        # self.controller = PlotController(scipp_obj_dict=scipp_obj_dict,
        #                                  axes=axes,
        #                                  masks=masks,
        #                                  logx=logx,
        #                                  logy=logy,
        #                                  errorbars=errorbars,
        #                                  button_options=['X'])

        # # The model which takes care of all heavy calculations
        # self.model = PlotModel1d(controller=self.controller,
        #                          scipp_obj_dict=scipp_obj_dict)

        # The view which will display the 1d plot and send pick events back to
        # the controller
        self.view = PlotView1d(
            # controller=self.controller,
            ax=ax,
            figsize=figsize,
            errorbars=self.errorbars,
            norm=norm,
            title=title,
            unit=self.params["values"][
                self.name]["unit"],
            # mask_params=self.controller.params["masks"][self.controller.name],
            masks=self.masks,
            # logx=logx,
            # logy=logy,
            mpl_line_params=mpl_line_params,
            picker=True,
            grid=grid)

        # # Profile view which displays an additional dimension as a 1d plot
        # if self.controller.ndim > 1:
        #     pad = config.plot.padding
        #     pad[2] = 0.75
        #     self.profile = ProfileView(
        #         errorbars=self.controller.errorbars,
        #         ax=pax,
        #         unit=self.controller.params["values"][
        #             self.controller.name]["unit"],
        #         mask_params=self.controller.params["masks"][
        #             self.controller.name],
        #         masks=self.controller.masks,
        #         logx=logx,
        #         logy=logy,
        #         figsize=(1.3 * config.plot.width / config.plot.dpi,
        #                  0.6 * config.plot.height / config.plot.dpi),
        #         padding=pad,
        #         legend={"show": True, "loc": (1.02, 0.0)})

        # An additional panel view with widgets to save/remove lines
        if self.ndim > 1:
            self.panel = PlotPanel1d(data_names=list(scipp_obj_dict.keys()))

        # # Connect controller to model, view, panel and profile
        # self._connect_controller_members()
        print("in sciplot", scale)

        # The main controller module which contains the slider widgets
        self.controller = PlotController1d(
          # scipp_obj_dict=scipp_obj_dict,
          axes=self.axes,
          name=self.name,
          dim_to_shape=self.dim_to_shape,
          coord_shapes=self.coord_shapes,
          # logx=logx,
          # logy=logy,
          vmin=self.params["values"][self.name]["vmin"],
          vmax=self.params["values"][self.name]["vmax"],
          norm=norm,
          scale=scale,
          # mask_names=self.mask_names,
          widgets=self.widgets,
          model=self.model,
          view=self.view,
          panel=self.panel)

        # Call update_slice once to make the initial plot
        self.controller.update_axes()

        return
