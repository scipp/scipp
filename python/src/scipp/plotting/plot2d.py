# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .objects import Plot, make_params, make_profile
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
    params = make_params(cmap=cmap,
                         norm=norm,
                         vmin=vmin,
                         vmax=vmax,
                         masks=masks)

    dims = next(iter(scipp_obj_dict.values())).dims
    if len(dims) > 2:
        params['extend_cmap'] = 'both'
        profile_figure = make_profile(ax=pax,
                                      mask_color=params['masks']['color'])
    else:
        profile_figure = None

    figure = PlotFigure2d(ax=ax,
                          cax=cax,
                          figsize=figsize,
                          aspect=aspect,
                          cmap=params["values"]["cmap"],
                          norm=params["values"]["norm"],
                          name=next(iter(scipp_obj_dict)),
                          cbar=params["values"]["cbar"],
                          mask_cmap=params['masks']['cmap'],
                          extend=params['extend_cmap'],
                          title=title,
                          xlabel=xlabel,
                          ylabel=ylabel)

    sp = Plot(scipp_obj_dict=scipp_obj_dict,
              figure=figure,
              profile_figure=profile_figure,
              errorbars=errorbars,
              labels=labels,
              resolution=resolution,
              params=params,
              norm=norm,
              scale=scale,
              view_ndims=2)

    if filename is not None:
        sp.savefig(filename)
    else:
        return sp
