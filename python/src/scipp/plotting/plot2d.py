# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .. import config
from .controller2d import PlotController2d
from .model1d import PlotModel1d
from .model2d import PlotModel2d
from .objects import Plot
from .profile import PlotProfile
from .view2d import PlotView2d
from .figure2d import PlotFigure2d
from .widgets import PlotWidgets


def plot2d(*args, filename=None, **kwargs):
    """
    Plot a 2d slice through a N dimensional dataset.
    For every dimension above 2, a slider is created to adjust the position
    of the slice in that particular dimension.
    """
    sp = Plot2d(*args, **kwargs)
    if filename is not None:
        sp.savefig(filename)
    else:
        return sp


class Plot2d(Plot):
    """
    Class for 2 dimensional plots.

    It uses Matplotlib's `imshow` to view 2d arrays are images, and implements
    a dynamic image resampling for better performance with large images.
    """
    def __init__(self,
                 scipp_obj_dict=None,
                 labels=None,
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
                 resolution=None,
                 title=None,
                 xlabel=None,
                 ylabel=None):

        view_ndims = 2

        super().__init__(scipp_obj_dict=scipp_obj_dict,
                         labels=labels,
                         cmap=cmap,
                         norm=norm,
                         vmin=vmin,
                         vmax=vmax,
                         masks=masks,
                         view_ndims=view_ndims)

        # The model which takes care of all heavy calculations
        self.model = PlotModel2d(scipp_obj_dict=scipp_obj_dict,
                                 name=self.name,
                                 resolution=resolution)
        profile_model = PlotModel1d(scipp_obj_dict=scipp_obj_dict,
                                    name=self.name)

        # Run validation checks before rendering the plot.
        # Note that validation needs to be run after model is created.
        self.validate()

        # Create control widgets (sliders and buttons)
        self.widgets = PlotWidgets(
            dims=self.dims,
            formatters=self._formatters,
            ndim=view_ndims,
            name=self.name,
            dim_label_map=self.labels,
            masks=scipp_obj_dict,
            multid_coord=self.model.get_multid_coord(),
            is_binned_data=(scipp_obj_dict[self.name].bins is not None))

        # The view which will display the 2d image and send pick events back to
        # the controller
        self.view = PlotView2d(figure=PlotFigure2d(
            ax=ax,
            cax=cax,
            figsize=figsize,
            aspect=aspect,
            cmap=self.params["values"]["cmap"],
            norm=self.params["values"]["norm"],
            name=self.name,
            cbar=self.params["values"]["cbar"],
            masks=self.masks[self.name],
            extend=self.extend_cmap,
            title=title,
            xlabel=xlabel,
            ylabel=ylabel),
                               formatters=self._formatters)

        # Profile view which displays an additional dimension as a 1d plot
        if len(self.dims) > 2:
            pad = config.plot.padding.copy()
            pad[2] = 0.77
            self.profile = PlotProfile(
                errorbars=self.errorbars,
                ax=pax,
                masks=self.masks,
                figsize=(1.3 * config.plot.width / config.plot.dpi,
                         0.6 * config.plot.height / config.plot.dpi),
                padding=pad,
                legend={
                    "show": True,
                    "loc": (1.02, 0.0)
                })

        # The main controller module which connects and controls all the parts
        self.controller = PlotController2d(
            dims=self.dims,
            name=self.name,
            vmin=self.params["values"]["vmin"],
            vmax=self.params["values"]["vmax"],
            norm=norm,
            scale=scale,
            widgets=self.widgets,
            model=self.model,
            profile_model=profile_model,
            view=self.view,
            profile=self.profile,
            multid_coord=self.model.get_multid_coord())

        # Render the figure once all components have been created.
        self.render(norm=norm)
