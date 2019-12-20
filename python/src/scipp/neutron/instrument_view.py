# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from ..config import plot as config
from ..plot.render import render_plot
from ..plot.sciplot import SciPlot
from ..plot.sparse import histogram_sparse_data, make_bins
from ..plot.tools import parse_params
from .._scipp import core as sc

# Other imports
import numpy as np
import ipywidgets as widgets
from matplotlib import cm
try:
    import ipyvolume as ipv
except ImportError:
    ipv = None


def instrument_view(data_array=None, bins=None, masks=None, filename=None,
                    figsize=None, aspect="equal", cmap=None, log=False,
                    vmin=None, vmax=None):
    """
    Plot a 3-dimensional view of the instrument.
    Sliders are also generated to navigate the time-of-flight dimension.
    """

    if ipv is None:
        raise RuntimeError("The instrument view requires ipyvolume  to be "
                           "installed. Use conda/pip install ipyvolume.")

    iv = Instrument3d(data_array=data_array, bins=bins, masks=masks, cmap=cmap,
                      log=log, vmin=vmin, vmax=vmax, aspect=aspect)

    render_plot(figure=iv.fig, widgets=iv.box, filename=filename, ipv=ipv)

    return SciPlot(iv.members)


class Instrument3d:

    def __init__(self, data_array=None, bins=None, masks=None, cmap=None,
                 log=None, vmin=None, vmax=None, aspect=None):

        # Initialise Figure
        self.fig = ipv.figure(width=config.width, height=config.height,
                              animation=0)

        # Get detector positions
        self.det_pos = np.array(data_array.labels["position"].values)

        # Find extents of the detectors
        self.xminmax = {}
        for i, x in enumerate("xyz"):
            self.xminmax[x] = [np.amin(self.det_pos[:, i]),
                               np.amax(self.det_pos[:, i])]

        # Make plot outline if aspect ratio is to be conserved
        if aspect == "equal":
            max_size = 0.0
            dx = {"x": 0, "y": 0, "z": 0}
            for ax in dx.keys():
                dx[ax] = np.ediff1d(self.xminmax[ax])
            max_size = np.amax(list(dx.values()))
            arrays = dict()
            for ax, size in dx.items():
                diff = max_size - size
                arrays[ax] = [self.xminmax[ax][0] - 0.5 * diff,
                              self.xminmax[ax][1] + 0.5 * diff]

            outl_x, outl_y, outl_z = np.meshgrid(arrays["x"], arrays["y"],
                                                 arrays["z"], indexing="ij")
            self.outline = ipv.plot_wireframe(outl_x, outl_y, outl_z,
                                              color="black")

        # Histogram the data in the Tof dimension
        if bins is not None:
            if data_array.sparse_dim is not None:
                self.hist_data_array = histogram_sparse_data(
                    data_array, data_array.sparse_dim, bins)
            else:
                self.hist_data_array = sc.rebin(
                    data_array, sc.Dim.Tof, make_bins(data_array=data_array,
                                                      dim=sc.Dim.Tof,
                                                      bins=bins))
        else:
            self.hist_data_array = data_array

        # Parse input parameters
        globs = {"cmap": cmap, "log": log, "vmin": vmin, "vmax": vmax}
        params = parse_params(globs=globs, array=self.hist_data_array.values)

        self.scalar_map = cm.ScalarMappable(cmap=params["cmap"])

        self.scatter = ipv.scatter(x=self.det_pos[:, 0], y=self.det_pos[:, 1],
                                   z=self.det_pos[:, 2], marker="square_2d",
                                   size=1)

        indx = self.hist_data_array.dims.index(sc.Dim.Tof)
        self.tof_slider = widgets.IntSlider(
            value=0, min=0, step=1, description="Tof",
            max=self.hist_data_array.shape[indx] - 1,
            continuous_update=True, readout=False)
        self.tof_slider.observe(self.update_colors, names="value")
        self.update_colors({"new": 0})

        self.box = widgets.VBox([ipv.gcc(), self.tof_slider])
        self.box.layout.align_items = 'center'

        self.members = {"widgets": {"sliders": self.tof_slider},
                        "fig": self.fig, "scatter": self.scatter,
                        "outline": self.outline}

        return

    def update_colors(self, change):
        self.scatter.color = self.scalar_map.to_rgba(
            self.hist_data_array[sc.Dim.Tof, change["new"]].values)
        return
