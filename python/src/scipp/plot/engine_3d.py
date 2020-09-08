# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import config
from .render import render_plot
from .slicer import Slicer
from .tools import to_bin_centers
from .._utils import name_with_unit, value_to_string
from .._scipp import core as sc

# Other imports
import numpy as np
import ipywidgets as widgets
from matplotlib import cm
import matplotlib as mpl
from matplotlib.backends import backend_agg
import PIL as pil
import pythreejs as p3
from copy import copy



class PlotEngine3d(PlotEngine):

    def __init__(self,
                 parent=None,
                 scipp_obj_dict=None,
                 axes=None,
                 masks=None,
                 cmap=None,
                 log=None,
                 vmin=None,
                 vmax=None,
                 color=None):

        super().__init__(parent=parent,
                         scipp_obj_dict=scipp_obj_dict,
                         axes=axes,
                         masks=masks,
                         cmap=cmap,
                         log=log,
                         vmin=vmin,
                         vmax=vmax,
                         color=color)



        self.vslice = None
        return


    def slice_data(self, change=None, autoscale_cmap=False):
        """
        Slice the extra dimensions down and update the slice values
        """
        self.vslice = self.data_arrays[self.name]
        # Slice along dimensions with active sliders
        for dim, val in self.parent.widgets.slider.items():
            if not val.disabled:
                # self.lab[dim].value = self.make_slider_label(
                #     self.slider_coord[self.name][dim], val.value)
                # self.lab[dim].value = self.make_slider_label(
                #     self.vslice.coords[dim], val.value, self.slider_axformatter[self.name][dim][False])
                # self.vslice = self.vslice[val.dim, val.value]

                deltax = self.parent.widgets.thickness_slider[dim].value
                self.vslice = self.resample_image(self.vslice,
                        rebin_edges={dim: sc.Variable([dim], values=[val.value - 0.5 * deltax,
                                                                     val.value + 0.5 * deltax],
                                                            unit=self.vslice.coords[dim].unit)})[dim, 0]
                self.vslice *= (deltax * sc.units.one)


        # Handle masks
        # if len(self.masks[self.name]) > 0:
        msk = None
        if len(self.parent.widgets.mask_checkboxes[self.name]) > 0:
            # Use automatic broadcasting in Scipp variables
            msk = sc.Variable(dims=self.vslice.dims,
                              values=np.zeros(self.vslice.shape,
                                              dtype=np.int32))
            for m, chbx in self.parent.widgets.mask_checkboxes[self.name].items():
                if chbx.value:
                    msk += sc.Variable(
                        dims=self.vslice.masks[m].dims,
                        values=self.vslice.masks[m].values.astype(np.int32))
            msk = msk.values

        self.vslice = self.vslice.values.flatten()
        if autoscale_cmap:
            self.parent.scalar_map.set_clim(self.vslice.min(), self.vslice.max())
        colors = self.parent.scalar_map.to_rgba(self.vslice).astype(np.float32)

        if msk is not None:
            masks_inds = np.where(msk.flatten())
            masks_colors = self.parent.masks_scalar_map.to_rgba(
                self.vslice[masks_inds]).astype(np.float32)
            colors[masks_inds] = masks_colors

        return colors

    def update_slice(self, change=None, autoscale_cmap=False):
        """
        Update colors of points.
        """
        new_colors = self.slice_data(change=change, autoscale_cmap=autoscale_cmap)
        new_colors[:,
                   3] = self.parent.points_geometry.attributes["rgba_color"].array[:,
                                                                            3]
        self.parent.points_geometry.attributes["rgba_color"].array = new_colors
        if self.cut_surface_buttons.value == self.cut_options["Value"]:
            self.update_cut_surface(None)
        return

    # def toggle_mask(self, change):
    #     """
    #     Show/hide masks
    #     """
    #     self.update_slice()
    #     return

    # def toggle_outline(self, change):
    #     self.outline.visible = change["new"]
    #     self.axticks.visible = change["new"]
    #     desc = "Hide" if change["new"] else "Show"
    #     self.toggle_outline_button.description = desc + " outline"

    # def rescale_to_data(self, button=None):
    #     # self.scalar_map.set_clim(self.vslice.min(), self.vslice.max())
    #     self.update_slice(autoscale_cmap=True)
