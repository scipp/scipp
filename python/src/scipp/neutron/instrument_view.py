# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import config
from ..plot.render import render_plot
from ..plot.sciplot import SciPlot
from ..plot.sparse import histogram_sparse_data, make_bins
from ..plot.tools import parse_params
from ..utils import name_with_unit, value_to_string
from .._scipp import core as sc, neutron as sn

# Other imports
import numpy as np
import importlib


def instrument_view(scipp_obj=None,
                    bins=None,
                    masks=None,
                    filename=None,
                    figsize=None,
                    aspect="equal",
                    cmap=None,
                    log=False,
                    vmin=None,
                    vmax=None,
                    size=0.1,
                    projection="3D",
                    nan_color="#d3d3d3",
                    continuous_update=True,
                    dim=sc.Dim.Tof):
    """
    Plot a 2D or 3D view of the instrument.
    A slider is also generated to navigate the dimension (dim) given as an
    input. Since this is designed for neutron science, the default dimension
    is neutron time-of-flight (sc.Dim.Tof), but this could be anything
    (wavelength, temperature...)

    Example:

    import scipp.neutron as sn
    sample = sn.load(filename="PG3_4844_event.nxs")
    sn.instrument_view(sample)
    """

    iv = InstrumentView(scipp_obj=scipp_obj,
                        bins=bins,
                        masks=masks,
                        cmap=cmap,
                        log=log,
                        vmin=vmin,
                        vmax=vmax,
                        aspect=aspect,
                        size=size,
                        projection=projection,
                        nan_color=nan_color,
                        continuous_update=continuous_update,
                        dim=dim)

    render_plot(widgets=iv.box, filename=filename)

    return SciPlot(iv.members)


class InstrumentView:
    def __init__(self,
                 scipp_obj=None,
                 bins=None,
                 masks=None,
                 cmap=None,
                 log=None,
                 vmin=None,
                 vmax=None,
                 aspect=None,
                 size=None,
                 projection=None,
                 nan_color=None,
                 continuous_update=None,
                 dim=None):

        # Delayed imports to avoid hard dependencies
        self.widgets = importlib.import_module("ipywidgets")
        self.mpl = importlib.import_module("matplotlib")
        self.mpl_cm = importlib.import_module("matplotlib.cm")
        self.mpl_figure = importlib.import_module("matplotlib.figure")
        self.mpl_backend_agg = importlib.import_module(
            "matplotlib.backends.backend_agg")
        self.pil = importlib.import_module("PIL")
        self.p3 = importlib.import_module("pythreejs")

        self.fig = None
        self.aspect = aspect
        self.nan_color = nan_color
        self.log = log
        self.current_projection = None
        self.dim = dim
        self.cbar_image = self.widgets.Image()

        if len(np.shape(size)) == 0:
            self.size = [size, size]
        else:
            self.size = size

        self.data_arrays = {}
        tp = type(scipp_obj)
        if tp is sc.Dataset or tp is sc.DatasetView:
            for key in sorted(scipp_obj.keys()):
                var = scipp_obj[key]
                if self.dim in var.dims:
                    self.data_arrays[key] = var
        elif tp is sc.DataArray or tp is sc.DataArrayView:
            self.data_arrays[scipp_obj.name] = scipp_obj
        else:
            raise RuntimeError("Unknown input type: {}. Allowed inputs "
                               "are a Dataset or a DataArray (and their "
                               "respective proxies).".format(tp))

        self.globs = {"cmap": cmap, "log": log, "vmin": vmin, "vmax": vmax}
        self.params = {}
        self.cmap = {}
        self.hist_data_array = {}
        self.scalar_map = {}
        self.minmax = {}

        # Find the min/max time-of-flight limits and store them
        self.minmax["tof"] = [np.Inf, np.NINF, 1]
        for key, data_array in self.data_arrays.items():
            bins_here = bins
            if data_array.sparse_dim is not None and bins_here is None:
                bins_here = True
            if bins_here is not None:
                dim = None if data_array.sparse_dim is not None else self.dim
                spdim = None if data_array.sparse_dim is None else self.dim
                var = make_bins(data_array=data_array,
                                sparse_dim=spdim,
                                dim=dim,
                                bins=bins_here,
                                padding=(data_array.sparse_dim is not None))
            else:
                var = data_array.coords[self.dim]
            self.minmax["tof"][0] = min(self.minmax["tof"][0], var.values[0])
            self.minmax["tof"][1] = max(self.minmax["tof"][1], var.values[-1])
            self.minmax["tof"][2] = var.shape[0]

        # Rebin all DataArrays to common Tof axis
        self.rebin_data(np.linspace(*self.minmax["tof"]))

        # Create dropdown menu to select the DataArray
        keys = list(self.hist_data_array.keys())
        self.dropdown = self.widgets.Dropdown(options=keys,
                                              description="Select entry:",
                                              layout={"width": "initial"})
        self.dropdown.observe(self.change_data_array, names="value")

        # Store current active data entry (DataArray)
        self.key = keys[0]

        # Create a Tof slider and its label
        self.tof_dim_indx = self.hist_data_array[self.key].dims.index(self.dim)
        self.slider = self.widgets.IntSlider(
            value=0,
            min=0,
            step=1,
            description=str(self.dim).replace("Dim.", ""),
            max=self.hist_data_array[self.key].shape[self.tof_dim_indx] - 1,
            continuous_update=continuous_update,
            readout=False)
        self.slider.observe(self.update_colors, names="value")
        self.label = self.widgets.Label()

        # Add text boxes to change number of bins/bin size
        self.nbins = self.widgets.Text(value=str(
            self.hist_data_array[self.key].shape[self.tof_dim_indx]),
                                       description="Number of bins:",
                                       style={"description_width": "initial"})
        self.nbins.on_submit(self.update_nbins)

        tof_values = self.hist_data_array[self.key].coords[self.dim].values
        self.bin_size = self.widgets.Text(value=str(tof_values[1] -
                                                    tof_values[0]),
                                          description="Bin size:")
        self.bin_size.on_submit(self.update_bin_size)

        projections = [
            "3D", "Cylindrical X", "Cylindrical Y", "Cylindrical Z",
            "Spherical X", "Spherical Y", "Spherical Z"
        ]

        # Create toggle buttons to change projection
        self.buttons = {}
        for p in projections:
            self.buttons[p] = self.widgets.Button(
                description=p,
                disabled=False,
                button_style=("info" if (p == projection) else ""))
            self.buttons[p].on_click(self.change_projection)
        items = [self.buttons["3D"]]
        for x in "XYZ":
            items.append(self.buttons["Cylindrical {}".format(x)])
            items.append(self.buttons["Spherical {}".format(x)])
            if x != "Z":
                items.append(self.widgets.Label())

        self.togglebuttons = self.widgets.GridBox(
            items,
            layout=self.widgets.Layout(
                grid_template_columns="repeat(3, 150px)"))

        # Place widgets in boxes
        self.vbox = self.widgets.VBox([
            self.widgets.HBox([self.dropdown, self.slider, self.label]),
            self.widgets.HBox([self.nbins, self.bin_size]), self.togglebuttons
        ])

        # Get detector positions
        self.det_pos = np.array(sn.position(
            self.hist_data_array[self.key]).values,
                                dtype=np.float32)
        # Find extents of the detectors
        for i, x in enumerate("xyz"):
            self.minmax[x] = [
                np.amin(self.det_pos[:, i]),
                np.amax(self.det_pos[:, i])
            ]

        # Create texture for scatter points to represent detector shapes
        nx = 32
        det_aspect_ratio = int(round(nx * min(self.size) / max(self.size)))
        if det_aspect_ratio % 2 == 0:
            half_width = det_aspect_ratio // 2
            istart = nx // 2 - half_width
            iend = nx // 2 + half_width
        else:
            half_width = (det_aspect_ratio - 1) // 2
            istart = nx // 2 - 1 - half_width
            iend = nx // 2 + half_width
            nx -= 1
        texture_array = np.zeros([nx, nx, 4], dtype=np.float32)
        if np.argmin(self.size) == 0:
            texture_array[:, istart:iend, :] = 1.0
        else:
            texture_array[istart:iend, :, :] = 1.0
        texture = self.p3.DataTexture(data=texture_array,
                                      format="RGBAFormat",
                                      type="FloatType")

        # The point cloud and its properties
        self.pts = self.p3.BufferAttribute(array=self.det_pos)
        self.colors = self.p3.BufferAttribute(array=np.random.random(
            [np.shape(self.det_pos)[0], 4]).astype(np.float32))
        self.geometry = self.p3.BufferGeometry(attributes={
            'position': self.pts,
            'color': self.colors
        })
        self.material = self.p3.PointsMaterial(vertexColors='VertexColors',
                                               size=max(self.size),
                                               map=texture,
                                               depthTest=False,
                                               transparent=True)
        self.pcl = self.p3.Points(geometry=self.geometry,
                                  material=self.material)
        # Add the red green blue axes helper
        self.axes_helper = self.p3.AxesHelper(100)

        # Create the threejs scene with ambient light and camera
        self.camera = self.p3.PerspectiveCamera(position=[0, 0, 10],
                                                aspect=config.plot.width /
                                                config.plot.height)
        self.key_light = self.p3.DirectionalLight(position=[0, 10, 10])
        self.ambient_light = self.p3.AmbientLight()
        self.scene = self.p3.Scene(children=[
            self.pcl, self.camera, self.key_light, self.ambient_light,
            self.axes_helper
        ])
        self.controller = self.p3.OrbitControls(controlling=self.camera)

        # Render the scene into a widget
        self.renderer = self.p3.Renderer(camera=self.camera,
                                         scene=self.scene,
                                         controls=[self.controller],
                                         width=config.plot.width,
                                         height=config.plot.height)
        self.box = self.widgets.VBox(
            [self.widgets.HBox([self.renderer, self.cbar_image]), self.vbox])
        self.box.layout.align_items = "center"

        # Update the plot elements
        self.update_colorbar()
        self.change_projection(self.buttons[projection])

        # Create members object
        self.members = {
            "widgets": {
                "sliders": self.slider,
                "buttons": self.buttons,
                "text": {
                    "nbins": self.nbins,
                    "bin_size": self.bin_size
                },
                "dropdown": self.dropdown
            },
            "points": self.pcl,
            "camera": self.camera,
            "scene": self.scene,
            "renderer": self.renderer
        }

        return

    def rebin_data(self, bins):
        """
        Rebin the original data to Tof given some bins. This is executed both
        on first instrument display and when either the number of bins or the
        bin width is changed.
        """
        for key, data_array in self.data_arrays.items():

            # Histogram the data in the Tof dimension
            if data_array.sparse_dim is not None:
                self.hist_data_array[key] = histogram_sparse_data(
                    data_array, data_array.sparse_dim, bins)
            else:
                self.hist_data_array[key] = sc.rebin(
                    data_array, self.dim,
                    make_bins(data_array=data_array,
                              dim=self.dim,
                              bins=bins,
                              padding=False))

            # Parse input parameters for colorbar
            self.params[key] = parse_params(
                globs=self.globs, array=self.hist_data_array[key].values)
            self.cmap[key] = self.mpl_cm.get_cmap(self.params[key]["cmap"])
            self.cmap[key].set_bad(color=self.nan_color)
            self.scalar_map[key] = self.mpl_cm.ScalarMappable(
                cmap=self.cmap[key], norm=self.params[key]["norm"])
        return

    def update_colorbar(self):
        height_inches = config.plot.height / config.plot.dpi
        fig = self.mpl_figure.Figure(figsize=(height_inches * 0.2,
                                              height_inches),
                                     dpi=config.plot.dpi)
        canvas = self.mpl_backend_agg.FigureCanvasAgg(fig)
        ax = fig.add_axes([0.05, 0.02, 0.25, 0.96])
        cb1 = self.mpl.colorbar.ColorbarBase(
            ax, cmap=self.cmap[self.key], norm=self.params[self.key]["norm"])
        cb1.set_label(
            name_with_unit(var=self.hist_data_array[self.key], name=""))
        canvas.draw()
        image = np.fromstring(canvas.tostring_rgb(), dtype='uint8')
        shp = list(fig.canvas.get_width_height())[::-1] + [3]
        self.cbar_image.value = self.pil.Image.fromarray(
            image.reshape(shp))._repr_png_()

    def update_colors(self, change):
        arr = self.hist_data_array[self.key][self.dim, change["new"]].values
        if self.log:
            arr = np.ma.masked_where(arr <= 0, arr)
        self.geometry.attributes["color"].array = self.scalar_map[
            self.key].to_rgba(arr).astype(np.float32)

        self.label.value = name_with_unit(
            var=self.hist_data_array[self.key].coords[self.dim],
            name=value_to_string(self.hist_data_array[self.key].coords[
                self.dim].values[change["new"]]))
        return

    def change_projection(self, owner):

        if owner.description == self.current_projection:
            owner.button_style = "info"
            return
        if self.current_projection is not None:
            self.buttons[self.current_projection].button_style = ""

        projection = owner.description

        # Compute cylindrical or spherical projections
        permutations = {"X": [0, 2, 1], "Y": [1, 0, 2], "Z": [2, 1, 0]}
        axis = projection[-1]

        if projection == "3D":
            xyz = self.det_pos
        else:
            xyz = np.zeros_like(self.det_pos)
            xyz[:, 0] = np.arctan2(self.det_pos[:, permutations[axis][2]],
                                   self.det_pos[:, permutations[axis][1]])
            if projection.startswith("Cylindrical"):
                xyz[:, 1] = self.det_pos[:, permutations[axis][0]]
            elif projection.startswith("Spherical"):
                xyz[:, 1] = np.arcsin(
                    self.det_pos[:, permutations[axis][0]] /
                    np.sqrt(self.det_pos[:, 0]**2 + self.det_pos[:, 1]**2 +
                            self.det_pos[:, 2]**2))
        self.axes_helper.visible = projection == "3D"
        self.camera.position = [0, 0, 5]
        self.renderer.controls = [
            self.p3.OrbitControls(controlling=self.camera,
                                  enableRotate=projection == "3D")
        ]
        self.geometry.attributes["position"].array = xyz

        self.update_colors({"new": self.slider.value})

        self.current_projection = owner.description
        self.buttons[owner.description].button_style = "info"

        return

    def update_nbins(self, owner):
        try:
            nbins = int(owner.value)
        except ValueError:
            print("Warning: could not convert value: {} to an "
                  "integer.".format(owner.value))
            return
        self.rebin_data(
            np.linspace(self.minmax["tof"][0], self.minmax["tof"][1],
                        nbins + 1))
        x = self.hist_data_array[self.key].coords[self.dim].values
        self.bin_size.value = str(x[1] - x[0])
        self.update_slider()

    def update_bin_size(self, owner):
        try:
            binw = float(owner.value)
        except ValueError:
            print("Warning: could not convert value: {} to a "
                  "float.".format(owner.value))
            return
        self.rebin_data(
            np.arange(self.minmax["tof"][0], self.minmax["tof"][1], binw))
        self.nbins.value = str(
            self.hist_data_array[self.key].shape[self.tof_dim_indx])
        self.update_slider()

    def update_slider(self):
        """
        Try to replace the slider around the same position
        """

        # Compute percentage position
        perc_pos = self.slider.value / self.slider.max
        # Compute new position
        nbins = int(self.nbins.value) - 1
        new_pos = int(perc_pos * nbins)
        # Either place new upper boundary first, or change slider value first
        if new_pos > self.slider.max:
            self.slider.max = nbins
            self.slider.value = new_pos
        else:
            self.slider.value = new_pos
            self.slider.max = nbins
        self.update_colorbar()

    def change_data_array(self, change):
        self.key = change["new"]
        self.update_colorbar()
        self.update_colors({"new": self.slider.value})
