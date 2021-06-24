# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .objects import Plot
from .view2d import PlotView2d
from .figure2d import PlotFigure2d


def plot2d(scipp_obj_dict,
           filename=None,
           labels=None,
           errorbars=None,
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
    """
    Plot a 2d slice through a N dimensional dataset.
    For every dimension above 2, a slider is created to adjust the position
    of the slice in that particular dimension.

    It uses Matplotlib's `imshow` to view 2d arrays are images, and implements
    a dynamic image resampling for better performance with large images.
    """
    sp = Plot(scipp_obj_dict=scipp_obj_dict,
              labels=labels,
              cmap=cmap,
              norm=norm,
              scale=scale,
              vmin=vmin,
              vmax=vmax,
              errorbars=errorbars,
              masks=masks,
              view_ndims=2)
    # The view which will display the 2d image and send pick events back to
    # the controller
    sp.view = PlotView2d(figure=PlotFigure2d(
        ax=ax,
        cax=cax,
        figsize=figsize,
        aspect=aspect,
        cmap=sp.params["values"]["cmap"],
        norm=sp.params["values"]["norm"],
        name=sp.name,
        cbar=sp.params["values"]["cbar"],
        mask_cmap=sp.params['masks']['cmap'],
        extend=sp.extend_cmap,
        title=title,
        xlabel=xlabel,
        ylabel=ylabel),
                         formatters=sp._formatters)

    # Profile view which displays an additional dimension as a 1d plot
    if len(sp.dims) > 2:
        sp.profile = sp._make_profile(ax=pax)

    sp.controller = sp._make_controller(norm=norm,
                                        scale=scale,
                                        resolution=resolution)

    # Render the figure once all components have been created.
    sp.render()
    if filename is not None:
        sp.savefig(filename)
    else:
        return sp
