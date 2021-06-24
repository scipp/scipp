# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .objects import Plot
from .view2d import PlotView2d
from .figure2d import PlotFigure2d


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

        super().__init__(scipp_obj_dict=scipp_obj_dict,
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
        self.view = PlotView2d(figure=PlotFigure2d(
            ax=ax,
            cax=cax,
            figsize=figsize,
            aspect=aspect,
            cmap=self.params["values"]["cmap"],
            norm=self.params["values"]["norm"],
            name=self.name,
            cbar=self.params["values"]["cbar"],
            mask_cmap=self.params['masks']['cmap'],
            extend=self.extend_cmap,
            title=title,
            xlabel=xlabel,
            ylabel=ylabel),
                               formatters=self._formatters)

        # Profile view which displays an additional dimension as a 1d plot
        if len(self.dims) > 2:
            self.profile = self._make_profile(ax=pax)

        self.controller = self._make_controller(norm=norm,
                                                scale=scale,
                                                resolution=resolution)

        # Render the figure once all components have been created.
        self.render()
