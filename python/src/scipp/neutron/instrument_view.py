# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from ..config import plot as config
from ..plot.render import render_plot
from ..plot.sciplot import SciPlot
from ..plot.sparse import histogram_sparse_data, make_bins
from ..plot.tools import parse_params
from ..utils import name_with_unit, value_to_string
from .._scipp import core as sc

# Other imports
import numpy as np
import ipywidgets as widgets
import matplotlib.pyplot as plt
from matplotlib import cm
try:
    import ipyvolume as ipv
except ImportError:
    ipv = None
import IPython.display as disp


def instrument_view(data_array=None, bins=None, masks=None, filename=None,
                    figsize=None, aspect="equal", cmap=None, log=False,
                    vmin=None, vmax=None, size=1, projection="3D",
                    nan_color="#d3d3d3", continuous_update=True):
    """
    Plot a 2D or 3D view of the instrument.
    A slider is also generated to navigate the time-of-flight dimension.

    Example:

    import scipp.neutron as sn
    sample = sn.load(filename='PG3_4844_event.nxs')
    sn.instrument_view(sample)
    """

    iv = InstrumentView(data_array=data_array, bins=bins, masks=masks,
                        cmap=cmap, log=log, vmin=vmin, vmax=vmax,
                        aspect=aspect, size=size, projection=projection,
                        nan_color=nan_color,
                        continuous_update=continuous_update)

    render_plot(widgets=iv.box, filename=filename, ipv=ipv)

    return SciPlot(iv.members)


class InstrumentView:

    def __init__(self, data_array=None, bins=None, masks=None, cmap=None,
                 log=None, vmin=None, vmax=None, aspect=None, size=1,
                 projection=None, nan_color=None, continuous_update=None):

        self.fig = None
        self.scatter = None
        self.outline = None
        self.size = size
        self.aspect = aspect
        self.do_update = None
        self.figurewidget = widgets.Output()
        self.mpl_figure = False
        self.image = None
        self.nan_color = nan_color
        self.log = log
        # self.continuous_update = continuous_update

        # Get detector positions
        self.det_pos = np.array(data_array.labels["position"].values)

        # Find extents of the detectors
        self.xminmax = {}
        for i, x in enumerate("xyz"):
            self.xminmax[x] = [np.amin(self.det_pos[:, i]),
                               np.amax(self.det_pos[:, i])]

        if data_array.sparse_dim is not None and bins is None:
            bins = True

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
        self.params = parse_params(globs=globs,
                                   array=self.hist_data_array.values)
        # cmap = cm.get_cmap(self.params["cmap"])
        # cmap.set_bad(color=self.nan_color)
        self.scalar_map = cm.ScalarMappable(cmap=self.params["cmap"],
                                            norm=self.params["norm"])
        print(self.params)

        # Create a Tof slider and its label
        indx = self.hist_data_array.dims.index(sc.Dim.Tof)
        self.tof_slider = widgets.IntSlider(
            value=0, min=0, step=1, description="Tof",
            max=self.hist_data_array.shape[indx] - 1,
            continuous_update=continuous_update, readout=False)
        self.tof_slider.observe(self.update_colors, names="value")
        self.tof_label = widgets.Label()

        projections = ["3D", "Cylindrical Y", "Spherical Y"]

        # Create toggle buttons to change projection
        self.togglebuttons = widgets.ToggleButtons(
                options=projections, description="", value=projection,
                disabled=False, button_style="")
        self.togglebuttons.observe(self.change_projection, names="value")

        # Place widgets in boxes
        self.vbox = widgets.VBox(
            [self.figurewidget,
             widgets.HBox([self.tof_slider, self.tof_label]),
             self.togglebuttons])
        self.box = widgets.VBox([self.vbox])
        self.box.layout.align_items = "center"

        # Protect against uninstalled ipyvolume
        if ipv is None and projection == "3D":
            print("Warning: 3D projection requires ipyvolume to be "
                  "installed. Use conda/pip install ipyvolume. Reverting to "
                  "2D projection.")
            self.togglebuttons.value = projections[1]
            self.togglebuttons.options = projections[1:]

        # Update the figure
        self.change_projection({"new": self.togglebuttons.value, "old": "3D"})

        # Create members object
        self.members = {"widgets": {"sliders": self.tof_slider,
                                    "togglebuttons": self.togglebuttons},
                        "fig": self.fig, "scatter": self.scatter,
                        "outline": self.outline, "image": self.image}

        return

    def update_colors(self, change):
        self.do_update(change)
        self.tof_label.value = name_with_unit(
            var=self.hist_data_array.coords[sc.Dim.Tof],
            name=value_to_string(
                self.hist_data_array.coords[sc.Dim.Tof].values[change["new"]]))
        return

    def change_projection(self, change):

        # Temporarily disable automatic plotting in notebook
        if plt.isinteractive():
            plt.ioff()
            re_enable_interactive = True
        else:
            re_enable_interactive = False

        update_children = False

        if change["new"] == "3D":
            if self.fig is not None:
                self.fig.clear()
            # update_children = True
            self.projection_3d()
            self.do_update = self.update_colors_3d
        else:
            if change["old"] == "3D":
                if ipv is not None:
                    ipv.clear()
                update_children = True
            self.projection_2d(change["new"], update_children)
            self.do_update = self.update_colors_2d

        self.update_colors({"new": self.tof_slider.value})

        # # Replace the figure in the VBox container
        # if update_children:
        #     self.box.children = tuple([self.figurewidget, self.vbox])

        # Re-enable automatic plotting in notebook
        if re_enable_interactive:
            plt.ion()

        return

    def projection_3d(self):
        # Initialise Figure
        self.fig = ipv.figure(width=config.width, height=config.height,
                              animation=0, lighting=False)
        # Make plot outline if aspect ratio is to be conserved
        if self.aspect == "equal":
            max_size = 0.0
            dx = {"x": 0, "y": 0, "z": 0}
            for ax in dx.keys():
                dx[ax] = np.ediff1d(self.xminmax[ax])
            max_size = np.amax(list(dx.values()))
            arrays = dict()
            for ax, s in dx.items():
                diff = max_size - s
                arrays[ax] = [self.xminmax[ax][0] - 0.5 * diff,
                              self.xminmax[ax][1] + 0.5 * diff]

            outl_x, outl_y, outl_z = np.meshgrid(arrays["x"], arrays["y"],
                                                 arrays["z"], indexing="ij")
            self.outline = ipv.plot_wireframe(outl_x, outl_y, outl_z,
                                              color="black")
        self.scatter = ipv.scatter(x=self.det_pos[:, 0], y=self.det_pos[:, 1],
                                   z=self.det_pos[:, 2], marker="square_2d",
                                   size=self.size)
        # self.scatter.material.transparent = True

        # self.figurewidget = ipv.gcc()
        self.figurewidget.clear_output()
        self.mpl_figure = False
        self.box.children = tuple([self.figurewidget, ipv.gcc(), self.vbox])
        return

    def update_colors_3d(self, change):
        arr = self.hist_data_array[sc.Dim.Tof, change["new"]].values
        if self.log:
            arr = np.ma.masked_where(arr <= 0, arr)
        self.scatter.color = self.scalar_map.to_rgba(arr)
        return

    def projection_2d(self, projection, update_children):
        # Initialise figure if we switched from 3D view, if not re-use current
        # figure.
        if update_children:
            self.box.children = tuple([self.figurewidget, self.vbox])
        if not self.mpl_figure:
            self.fig, self.ax = plt.subplots(
                1, 1, figsize=(config.width / config.dpi,
                               config.height / config.dpi))
            self.figurewidget.clear_output()
            with self.figurewidget:
                disp.display(self.fig)


        # Compute cylindrical or spherical projections
        theta = np.arctan2(self.det_pos[:, 2], self.det_pos[:, 0])
        if projection == "Cylindrical Y":
            z_or_phi = self.det_pos[:, 1]
        elif projection == "Spherical Y":
            z_or_phi = np.arcsin(self.det_pos[:, 1] /
                                 np.sqrt(self.det_pos[:, 0]**2 +
                                         self.det_pos[:, 1]**2 +
                                         self.det_pos[:, 2]**2))

        # Create the scatter
        if not self.mpl_figure:
            self.scatter = self.ax.scatter(
                x=theta, y=z_or_phi, s=self.size,
                c=self.hist_data_array[sc.Dim.Tof,
                                       self.tof_slider.value].values,
                norm=self.params["norm"], cmap=self.params["cmap"],
                marker="s")
            if self.params["cbar"]:
                self.cbar = plt.colorbar(self.scatter, ax=self.ax)
                self.cbar.ax.set_ylabel(
                    name_with_unit(var=self.hist_data_array, name=""))
                self.cbar.ax.yaxis.set_label_coords(-1.1, 0.5)
            self.mpl_figure = True
        else:
            self.scatter.set_offsets(np.array([theta, z_or_phi]))
        return

    def update_colors_2d(self, change):
        self.scatter.set_array(self.hist_data_array[sc.Dim.Tof, change["new"]].values)
        self.fig.canvas.draw()
        return
