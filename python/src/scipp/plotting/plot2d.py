# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .objects import make_params, make_profile, make_plot
from .model2d import PlotModel2d
from .view2d import PlotView2d
from .figure2d import PlotFigure2d
from .controller2d import PlotController2d


def plot2d(scipp_obj_dict, **kwargs):
    """
    Plot a 2d slice through a N dimensional dataset.
    For every dimension above 2, a slider is created to adjust the position
    of the slice in that particular dimension.

    It uses Matplotlib's `imshow` to view 2d arrays are images, and implements
    a dynamic image resampling for better performance with large images.
    """
    def builder(*,
                dims,
                norm=None,
                masks=None,
                ax=None,
                cax=None,
                pax=None,
                figsize=None,
                aspect=None,
                cmap=None,
                vmin=None,
                vmax=None,
                title=None,
                xlabel=None,
                ylabel=None,
                grid=False):
        out = {
            'view_ndims': 2,
            'model': PlotModel2d,
            'view': PlotView2d,
            'controller': PlotController2d
        }
        params = make_params(cmap=cmap, norm=norm, vmin=vmin, vmax=vmax, masks=masks)
        out['vmin'] = params["values"]["vmin"]
        out['vmax'] = params["values"]["vmax"]
        if len(dims) > 2:
            params['extend_cmap'] = 'both'
            out['profile_figure'] = make_profile(ax=pax,
                                                 mask_color=params['masks']['color'])

        out['figure'] = PlotFigure2d(ax=ax,
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
                                     ylabel=ylabel,
                                     grid=grid)
        return out

    return make_plot(builder, scipp_obj_dict, **kwargs)
