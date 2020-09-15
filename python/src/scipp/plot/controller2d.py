# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import config
from .controller import PlotController
from .tools import to_bin_centers
from .._utils import name_with_unit, value_to_string
from .._scipp import core as sc

# Other imports
import numpy as np
# import ipywidgets as widgets
# from matplotlib import cm
# import matplotlib as mpl
# from matplotlib.backends import backend_agg
# import PIL as pil
# import pythreejs as p3
# from copy import copy



class PlotController2d(PlotController):

    def __init__(self,
                 scipp_obj_dict=None,
                 axes=None,
                 masks=None,
                 cmap=None,
                 log=None,
                 vmin=None,
                 vmax=None,
                 color=None,
                 logx=False,
                 logy=False,
                 logz=False,
                 button_options=None,
                 positions=None,
                 errorbars=None):

        super().__init__(scipp_obj_dict=scipp_obj_dict,
                 axes=axes,
                 masks=masks,
                 cmap=cmap,
                 log=log,
                 vmin=vmin,
                 vmax=vmax,
                 color=color,
                 logx=logx,
                 logy=logy,
                 logz=logz,
                 button_options=button_options,
                 # aspect=None,
                 positions=positions,
                 errorbars=errorbars)

        return

    def update_viewport(self, xylims):
        new_values = self.model.update_viewport(
            xylims, mask_info=self.get_mask_info())
        self.view.update_data(new_values)

