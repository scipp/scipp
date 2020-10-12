# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import config
from .controller2d import PlotController2d
from .model2d import PlotModel2d
from .profile import ProfileView
from .sciplot import SciPlot
from .view2d import PlotView2d
from .widgets import PlotWidgets


# def plot2d(scipp_obj_dict=None,
#            axes=None,
#            masks=None,
#            filename=None,
#            figsize=None,
#            ax=None,
#            cax=None,
#            aspect=None,
#            cmap=None,
#            log=False,
#            vmin=None,
#            vmax=None,
#            color=None,
#            logx=False,
#            logy=False,
#            logxy=False,
#            resolution=None):
def plot2d(*args,
           filename=None,
           # logx=False,
           # logy=False,
           # logxy=False,
           **kwargs):
    """
    Plot a 2D slice through a N dimensional dataset. For every dimension above
    2, a slider is created to adjust the position of the slice in that
    particular dimension.
    """

    sp = SciPlot2d(*args,
                   # logx=logx or logxy,
                   # logy=logy or logxy,
                   **kwargs)

    if filename is not None:
        sp.savefig(filename)

    return sp


class SciPlot2d(SciPlot):
    def __init__(self,
                 scipp_obj_dict=None,
                 axes=None,
                 masks=None,
                 ax=None,
                 cax=None,
                 pax=None,
                 figsize=None,
                 aspect=None,
                 cmap=None,
                 norm=None,
                 scale=None,
                 vmin=None,
                 vmax=None,
                 # color=None,
                 # logx=False,
                 # logy=False,
                 resolution=None):

        super().__init__(scipp_obj_dict=scipp_obj_dict,
                 axes=axes,
                 cmap=cmap,
                 norm=norm,
                 vmin=vmin,
                 vmax=vmax,
                 masks=masks,
                 view_ndims=2)

        # button_options = ['X', 'Y']

        # Create control widgets (sliders and buttons).
        # Typically one set of slider/buttons for each dimension.
        self.widgets = PlotWidgets(axes=self.axes,
                                   ndim=self.ndim,
                                   name=self.name,
                                   dim_to_shape=self.dim_to_shape,
                                   masks=self.masks,
                                   button_options=['X', 'Y'])
                                   # positions=positions)
        # # return

        # # # The main controller module which contains the slider widgets
        # # self.controller = PlotController2d(scipp_obj_dict=scipp_obj_dict,
        # #                                    axes=axes,
        # #                                    dim_to_shape=dim_to_shape)
        # #                                    # masks=masks,
        # #                                    # cmap=cmap,
        # #                                    # log=log,
        # #                                    # vmin=vmin,
        # #                                    # vmax=vmax,
        # #                                    # color=color,
        # #                                    # logx=logx,
        # #                                    # logy=logy,
        # #                                    # button_options=button_options)

        # The model which takes care of all heavy calculations
        self.model = PlotModel2d(scipp_obj_dict=scipp_obj_dict,
                                 axes=self.axes,
                                 name=self.name,
                                 dim_to_shape=self.dim_to_shape,
                                 dim_label_map=self.dim_label_map,
                                 resolution=resolution)

        # The view which will display the 2d image and send pick events back to
        # the controller
        self.view = PlotView2d(
            ax=ax,
            cax=cax,
            figsize=figsize,
            aspect=aspect,
            cmap=self.params["values"][
                self.name]["cmap"],
            norm=self.params["values"][
                self.name]["norm"],
            title=self.name,
            cbar=self.params["values"][
                self.name]["cbar"],
            unit=self.params["values"][
                self.name]["unit"],
            # mask_cmap=self.params["masks"][
            #     self.name]["cmap"],
            masks=self.masks[self.name])
            # logx=logx,
            # logy=logy)

        # Profile view which displays an additional dimension as a 1d plot
        if self.ndim > 2:
            # mask_params = self.controller.params["masks"][self.controller.name]
            # mask_params["color"] = "k"
            pad = config.plot.padding
            pad[2] = 0.75
            self.profile = ProfileView(
                errorbars=self.errorbars,
                ax=pax,
                unit=self.params["values"][
                    self.name]["unit"],
                # mask_params=mask_params,
                masks=self.masks,
                # logx=logx,
                # logy=logy,
                figsize=(1.3 * config.plot.width / config.plot.dpi,
                         0.6 * config.plot.height / config.plot.dpi),
                padding=pad,
                legend={"show": True, "loc": (1.02, 0.0)})

        # # # Connect controller to model, view, panel and profile
        # # self._connect_controller_members()

        # The main controller module which contains the slider widgets
        self.controller = PlotController2d(
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
          # masks=self.masks,
          widgets=self.widgets,
          model=self.model,
          view=self.view,
          profile=self.profile)
          # masks=masks,
          # cmap=cmap,
          # log=log,

           # color=color,
           # logx=logx,
           # logy=logy,
           # button_options=button_options)

        # Call update_slice once to make the initial image
        self.controller.update_axes()

        return
