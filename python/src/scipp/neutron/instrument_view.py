# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import config
from ..plot.render import render_plot
from ..plot.sciplot import SciPlot
from ..plot.events import histogram_events_data, make_bins
from ..plot.tools import parse_params
from .._utils import name_with_unit, value_to_string
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
                    projection="3D Z",
                    nan_color="#d3d3d3",
                    continuous_update=True,
                    dim="tof",
                    rendering="Full",
                    pixel_size=0.02,
                    camera_pos=None,
                    look_at=None,
                    background="#f0f0f0"):
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
                        projection=projection,
                        nan_color=nan_color,
                        continuous_update=continuous_update,
                        dim=dim,
                        rendering=rendering,
                        pixel_size=pixel_size,
                        camera_pos=camera_pos,
                        look_at=look_at,
                        background=background)

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
                 projection=None,
                 nan_color=None,
                 continuous_update=None,
                 dim=None,
                 rendering=None,
                 pixel_size=None,
                 camera_pos=None,
                 look_at=None,
                 background=None):
        self._pixel_size = pixel_size
        self._camera_pos = camera_pos
        self._look_at = look_at

        # Delayed imports to avoid hard dependencies
        self.widgets = importlib.import_module("ipywidgets")
        self.mpl = importlib.import_module("matplotlib")
        self.mpl_cm = importlib.import_module("matplotlib.cm")
        self.mpl_plt = importlib.import_module("matplotlib.pyplot")
        self.mpl_ticker = importlib.import_module("matplotlib.ticker")
        self.mpl_figure = importlib.import_module("matplotlib.figure")
        self.mpl_colors = importlib.import_module("matplotlib.colors")
        self.mpl_backend_agg = importlib.import_module(
            "matplotlib.backends.backend_agg")
        self.pil_image = importlib.import_module("PIL.Image")
        self.p3 = importlib.import_module("pythreejs")

        self.fig = None
        self.aspect = aspect
        self.nan_color = nan_color
        self.log = log
        self.current_projection = ""
        self.lock_bin_inputs = False
        self.lock_camera = False
        self.slider_dim = dim
        self.other_dim = None
        self.cbar_image = self.widgets.Image()

        # Initialise variables for pythreejs objects
        self.detector_shape = None
        self.nverts = None
        self.nfaces = None
        self.ndets = None
        self.geometry = None
        self.material = None
        self.mesh = None
        self.points = None
        self.axes_2d = None
        self.axes_3d = None
        self.ticks_2d = None
        self.ticks_3d = None

        self.data_arrays = {}
        masks_present = False
        tp = type(scipp_obj)
        if tp is sc.Dataset or tp is sc.DatasetView:
            for key in sorted(scipp_obj.keys()):
                var = scipp_obj[key]
                if self.slider_dim in var.coords:
                    self.data_arrays[key] = var
                    if len(var.masks) > 0:
                        masks_present = True
        elif tp is sc.DataArray or tp is sc.DataArrayView:
            self.data_arrays[scipp_obj.name] = scipp_obj
            if len(scipp_obj.masks) > 0:
                masks_present = True
        else:
            raise RuntimeError("Unknown input type: {}. Allowed inputs "
                               "are a Dataset or a DataArray (and their "
                               "respective views).".format(tp))

        self.globs = {"cmap": cmap, "log": log, "vmin": vmin, "vmax": vmax}
        self.params = {}
        self.cmap = {}
        self.hist_data_array = {}
        self.scalar_map = {}
        self.masks = masks
        self.masks_variables = {}
        self.masks_params = {}
        self.masks_cmap = {}
        self.masks_scalar_map = {}

        # Find the min/max time-of-flight limits and store them
        self.minmax = [np.Inf, np.NINF, 1]
        for key, data_array in self.data_arrays.items():
            bins_here = bins
            contains_events = sc.contains_events(data_array)
            if contains_events and bins_here is None:
                bins_here = True
            if bins_here is not None:
                dim = None if contains_events else self.slider_dim
                spdim = None if not contains_events else self.slider_dim
                var = make_bins(data_array=data_array,
                                events_dim=spdim,
                                dim=dim,
                                bins=bins_here,
                                padding=contains_events)
            else:
                var = data_array.coords[self.slider_dim]
            self.minmax[0] = min(self.minmax[0], var.values[0])
            self.minmax[1] = max(self.minmax[1], var.values[-1])
            self.minmax[2] = var.shape[0]

        self.available_cmaps = sorted(m for m in self.mpl_plt.colormaps())

        # Store current active data entry (DataArray)
        keys = list(self.data_arrays.keys())
        self.key = keys[0]

        # This widget needs to be before the call to rebin_data
        self.masks_cmap_or_color = self.widgets.RadioButtons(
            options=['colormap', 'solid color'],
            description='Mask color:',
            layout={'width': "250px"})
        self.masks_cmap_or_color.observe(self.toggle_color_selection,
                                         names="value")

        # Rebin all DataArrays to common Tof axis
        self.rebin_data(np.linspace(*self.minmax))

        # Now we have 2D data, we can extract the other dimension
        all_dims = self.hist_data_array[self.key].dims
        all_dims.remove(self.slider_dim)
        self.other_dim = all_dims[0]

        # Create dropdown menu to select the DataArray
        self.dropdown = self.widgets.Dropdown(options=keys,
                                              description="Select entry:",
                                              layout={"width": "initial"})
        self.dropdown.observe(self.change_data_array, names="value")

        checkbox_label = self.widgets.Label(value="Continuous update:")
        self.continuous_update = self.widgets.Checkbox(
            value=continuous_update,
            description="",
            indent=False,
            layout={"width": "initial"})
        self.continuous_update.observe(self.toggle_continuous_update,
                                       names="value")

        # Create a Tof slider and its label
        self.tof_dim_indx = self.hist_data_array[self.key].dims.index(
            self.slider_dim)
        self.slider = self.widgets.IntSlider(
            value=0,
            min=0,
            step=1,
            description=str(self.slider_dim).replace("Dim.", ""),
            max=self.hist_data_array[self.key].shape[self.tof_dim_indx] - 1,
            continuous_update=self.continuous_update.value,
            readout=False)
        self.slider.observe(self.update_colors, names="value")
        self.label = self.widgets.Label(layout={"width": "150px"})

        # Add text boxes to change number of bins/bin size
        self.nbins = self.widgets.Text(value=str(
            self.hist_data_array[self.key].shape[self.tof_dim_indx]),
                                       description="Number of bins:",
                                       style={"description_width": "initial"},
                                       continuous_update=False)
        self.nbins.observe(self.update_nbins, names="value")

        tof_values = self.hist_data_array[self.key].coords[
            self.slider_dim].values
        self.bin_size = self.widgets.Text(value=str(tof_values[1] -
                                                    tof_values[0]),
                                          description="Bin size:",
                                          continuous_update=False)
        self.bin_size.observe(self.update_bin_size, names="value")

        # Add dropdown to change cmap
        self.select_colormap = self.widgets.Text(
            value=self.params[self.key]["cmap"],
            description="Select colormap",
            ensure_option=True,
            layout={'width': "200px"},
            style={"description_width": "initial"},
            continuous_update=False)
        self.select_colormap.observe(self.update_colormap, names="value")
        self.colormap_error = self.widgets.HTML(value="")

        self.opacity_slider = self.widgets.FloatSlider(min=0,
                                                       max=1.0,
                                                       value=1.0,
                                                       step=0.01,
                                                       description="Opacity")
        self.opacity_slider.observe(self.update_opacity, names="value")

        self.select_rendering = self.widgets.Dropdown(
            options=["Fast", "Full"],
            value=rendering,
            description="Rendering:",
            layout={"width": "initial"})
        # Disable Full rendering if there are not shapes or rotations
        if "shape" not in self.data_arrays[
                self.key].attrs or "rotation" not in self.data_arrays[
                    self.key].attrs:
            self.select_rendering.value = "Fast"
            self.select_rendering.disabled = True
        self.select_rendering.observe(self.change_rendering, names="value")

        projections = [
            "3D X", "3D Y", "3D Z", "Cylindrical X", "Cylindrical Y",
            "Cylindrical Z", "Spherical X", "Spherical Y", "Spherical Z"
        ]

        # Create toggle buttons to change projection
        self.buttons = {}
        for p in projections:
            self.buttons[p] = self.widgets.Button(
                description=p,
                disabled=False,
                button_style=("info" if (p == projection) else ""))
            self.buttons[p].on_click(self.change_projection)
        items = []
        for x in "XYZ":
            items.append(self.buttons["3D {}".format(x)])
            items.append(self.buttons["Cylindrical {}".format(x)])
            items.append(self.buttons["Spherical {}".format(x)])

        self.togglebuttons = self.widgets.GridBox(
            items,
            layout=self.widgets.Layout(
                grid_template_columns="repeat(3, 150px)"))

        # Create control section for masks
        self.masks_showhide = self.widgets.ToggleButton(
            value=True, description="Hide masks", button_style="")
        self.masks_showhide.observe(self.toggle_masks, names="value")

        self.masks_colormap = self.widgets.Text(
            value=self.masks_params[self.key]["cmap"],
            description="",
            ensure_option=True,
            disabled=not self.masks_cmap_or_color.value == "colormap",
            layout={'width': "100px"},
            continuous_update=False)
        self.masks_colormap.observe(self.update_masks_colormap, names="value")
        self.masks_colormap_error = self.widgets.HTML(value="")

        self.masks_solid_color = self.widgets.ColorPicker(
            concise=False,
            description='',
            value='#%02X%02X%02X' % (tuple(np.random.randint(0, 255, 3))),
            disabled=not self.masks_cmap_or_color.value == "solid color",
            layout={'width': "100px"})
        self.masks_solid_color.observe(self.update_masks_solid_color,
                                       names="value")

        # Place widgets in boxes
        box_list = [
            self.widgets.HBox([
                self.dropdown, self.slider, self.label, checkbox_label,
                self.continuous_update
            ]),
            self.widgets.HBox([self.nbins, self.bin_size]),
            self.widgets.HBox([
                self.select_rendering, self.opacity_slider,
                self.select_colormap, self.colormap_error
            ]), self.togglebuttons
        ]
        # Only show mask controls if masks are present
        if masks_present:
            box_list.append(
                self.widgets.HBox([
                    self.masks_showhide, self.masks_cmap_or_color,
                    self.masks_colormap, self.masks_colormap_error,
                    self.masks_solid_color
                ]))

        self.vbox = self.widgets.VBox(box_list)

        # Get detector positions
        self.det_pos = sn.position(self.hist_data_array[self.key])
        # Find extents of the detectors
        self.camera_pos = np.NINF
        for i, x in enumerate("xyz"):
            comp = getattr(sc.geometry, x)(self.det_pos)
            self.camera_pos = max(
                self.camera_pos,
                np.amax(
                    np.abs([
                        sc.min(comp, self.other_dim).value,
                        sc.max(comp, self.other_dim).value
                    ])))

        # Add the red green blue axes helper
        self.axes_3d = self.p3.AxesHelper(self.camera_pos * 50.0)

        # Add the 2D axes but don't show them yet
        line_geom = self.p3.BufferGeometry(
            attributes={
                'position':
                self.p3.BufferAttribute(array=np.array(
                    [[0, 0, 0], [1, 0, 0], [1, 1, 0], [0, 1, 0], [0, 0, 0]],
                    dtype=np.float32))
            })
        line_mat = self.p3.LineBasicMaterial(color="black", linewidth=1)
        self.axes_2d = self.p3.Line(geometry=line_geom, material=line_mat)

        # Create the threejs scene with ambient light and camera
        self.camera = self.p3.PerspectiveCamera(position=[self.camera_pos] * 3,
                                                aspect=config.plot.width /
                                                config.plot.height)

        self.scene = self.p3.Scene(
            children=[self.camera, self.axes_3d, self.axes_2d],
            background=background)

        # Call the rendering which will create the mesh and points objects
        self.change_rendering({"new": self.select_rendering.value})

        # Add camera controller
        if self._look_at is not None:
            self.controller = self.p3.OrbitControls(controlling=self.camera,
                                                    target=self._look_at)
            self.camera.lookAt(self._look_at)
        else:
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
            "camera": self.camera,
            "scene": self.scene,
            "renderer": self.renderer
        }

        return

    def create_mesh_geometry(self):
        """
        Generate a mesh object with the full detector geometry.
        """

        # Generate pixel shape from either box or cylinder for the 'Full'
        # rendering mode
        self.detector_shape, detector_faces = \
            self.get_detector_vertices_and_faces()

        # Create full geometry mesh
        self.nverts = len(self.detector_shape)
        self.nfaces = len(detector_faces)
        self.ndets = self.det_pos.shape[0]

        faces = np.tile(detector_faces, [self.ndets, 1]) + np.repeat(
            np.arange(
                0, self.ndets * self.nverts, self.nverts, dtype=np.uint32),
            self.nfaces * 3,
            axis=0).reshape(self.nfaces * self.ndets, 3)

        vertexcolors = np.zeros([self.ndets * self.nverts, 3],
                                dtype=np.float32)

        mesh_geometry = self.p3.BufferGeometry(attributes=dict(
            position=self.p3.BufferAttribute(np.zeros(
                [self.nverts * self.ndets, 3], dtype=np.float32),
                                             normalized=False),
            index=self.p3.BufferAttribute(faces.ravel(), normalized=False),
            color=self.p3.BufferAttribute(vertexcolors),
        ))

        mesh_material = self.p3.MeshBasicMaterial(vertexColors='VertexColors',
                                                  transparent=True)

        mesh = self.p3.Mesh(geometry=mesh_geometry, material=mesh_material)

        return mesh_geometry, mesh_material, mesh

    def create_points_geometry(self):
        """
        Make a simple PointsGeometry for the 'Fast' rendering mode
        """
        points_geometry = self.p3.BufferGeometry(
            attributes={
                'position':
                self.p3.BufferAttribute(array=self.det_pos.values),
                'color':
                self.p3.BufferAttribute(array=np.zeros(
                    [self.det_pos.shape[0], 3], dtype=np.float32))
            })
        points_material = self.p3.PointsMaterial(vertexColors='VertexColors',
                                                 size=self._pixel_size,
                                                 transparent=True)
        points = self.p3.Points(geometry=points_geometry,
                                material=points_material)
        return points_geometry, points_material, points

    def get_detector_vertices_and_faces(self):
        if "instrument-name" in self.data_arrays[self.key].attrs:
            instrument_name = self.data_arrays[
                self.key].attrs["instrument-name"].value
        else:
            instrument_name = ""
        cylindrical_major_axis = {"loki": "x"}
        instrument_name = instrument_name.lower()
        axis = None
        for name in cylindrical_major_axis:
            if instrument_name.startswith(name):
                axis = cylindrical_major_axis[name]
                break
        if axis is not None:
            sq2 = 0.25 * np.sqrt(2.0)
            vertices = np.array(
                [[0.0, 0.0, -0.5], [0.5, 0.0, -0.5], [sq2, sq2, -0.5],
                 [0.0, 0.5, -0.5], [-sq2, sq2, -0.5], [-0.5, 0.0, -0.5],
                 [-sq2, -sq2, -0.5], [0.0, -0.5, -0.5], [sq2, -sq2, -0.5],
                 [0.0, 0.0, 0.5], [0.5, 0.0, 0.5], [sq2, sq2, 0.5],
                 [0.0, 0.5, 0.5], [-sq2, sq2, 0.5], [-0.5, 0.0, 0.5],
                 [-sq2, -sq2, 0.5], [0.0, -0.5, 0.5], [sq2, -sq2, 0.5]],
                dtype=np.float32)

            # By default, we assume major axis of cylinder is z. If it is not
            # for that instrument, then swap the columns
            iswap = "xyz".find(axis)
            vertices[:, [iswap, 2]] = vertices[:, [2, iswap]]

            # The faces array contains the indices of the vertices for each
            # mesh triangle. In the case of cylinders, we start with the bottom
            # face (8 triangles), then the top face (8 triangles), then the
            # sides of the cylinder (8x2 triangles).
            faces = np.array(
                [[0, 2, 1], [0, 3, 2], [0, 4, 3], [0, 5, 4], [0, 6, 5],
                 [0, 7, 6], [0, 8, 7], [0, 1, 8], [9, 10, 11], [9, 11, 12],
                 [9, 12, 13], [9, 13, 14], [9, 14, 15], [9, 15, 16],
                 [9, 16, 17], [9, 17, 10], [1, 11, 10], [1, 2, 11],
                 [2, 12, 11], [2, 3, 12], [3, 13, 12], [3, 4, 13], [4, 14, 13],
                 [4, 5, 14], [5, 15, 14], [5, 6, 15], [6, 16, 15], [6, 7, 16],
                 [7, 17, 16], [7, 8, 17], [8, 10, 17], [8, 1, 10]],
                dtype=np.uint32)
        else:
            vertices = np.array(
                [[-0.5, -0.5, -0.5], [0.5, -0.5, -0.5], [0.5, 0.5, -0.5],
                 [-0.5, 0.5, -0.5], [-0.5, -0.5, 0.5], [0.5, -0.5, 0.5],
                 [0.5, 0.5, 0.5], [-0.5, 0.5, 0.5]],
                dtype=np.float32)

            # The faces array contains the indices of the vertices for each
            # mesh triangle. In the case of boxes, we have 2 triangles per side
            # of the box. The order of the faces is x=0, x=1, y=0, y=1, z=1,
            # z=1.
            faces = np.array([[0, 4, 3], [3, 4, 7], [1, 2, 6], [1, 6, 5],
                              [0, 1, 5], [0, 5, 4], [2, 3, 7], [2, 7, 6],
                              [0, 2, 1], [0, 3, 2], [4, 5, 7], [5, 6, 7]],
                             dtype=np.uint32)
        return vertices, faces

    def make_axis_tick(self, string, position, color="black", size=1.0):
        sm = self.p3.SpriteMaterial(map=self.p3.TextTexture(
            string=string, color=color, size=300, squareTexture=True),
                                    transparent=True)
        return self.p3.Sprite(material=sm,
                              position=position,
                              scaleToTexture=True,
                              scale=[size, size, size])

    def rebin_data(self, bins):
        """
        Rebin the original data to Tof given some bins. This is executed both
        on first instrument display and when either the number of bins or the
        bin width is changed.
        """
        for key, data_array in self.data_arrays.items():

            # Histogram the data in the Tof dimension
            if sc.contains_events(data_array):
                self.hist_data_array[key] = histogram_events_data(
                    data_array, self.slider_dim, bins)
            else:
                self.hist_data_array[key] = sc.rebin(
                    data_array, self.slider_dim,
                    make_bins(data_array=data_array,
                              dim=self.slider_dim,
                              bins=bins,
                              padding=False))

            # Parse input parameters for colorbar
            self.params[key] = parse_params(
                globs=self.globs, array=self.hist_data_array[key].values)
            if key not in self.cmap:
                self.cmap[key] = self.mpl_cm.get_cmap(self.params[key]["cmap"])
                self.cmap[key].set_bad(color=self.nan_color)
            self.scalar_map[key] = self.mpl_cm.ScalarMappable(
                cmap=self.cmap[key], norm=self.params[key]["norm"])

            # Parse mask parameters and combine masks into one
            self.masks_params[key] = parse_params(params=self.masks,
                                                  defaults={
                                                      "cmap": "gray",
                                                      "cbar": False
                                                  })
            self.masks_params[key]["show"] = (
                self.masks_params[key]["show"]
                and len(self.hist_data_array[key].masks) > 0)
            if self.masks_params[key]["show"] and (
                    key not in self.masks_variables):
                self.masks_variables[key] = sc.combine_masks(
                    self.hist_data_array[key].masks,
                    self.hist_data_array[key].dims,
                    self.hist_data_array[key].shape)

        # Update the colormap with the new normalization since binning has
        # changed
        if self.masks_cmap_or_color.value == "colormap":
            self.masks_cmap = self.mpl_cm.get_cmap(
                self.masks_params[self.key]["cmap"])
        else:
            self.mpl_colors.LinearSegmentedColormap.from_list(
                "tmp",
                [self.masks_solid_color.value, self.masks_solid_color.value])
        self.masks_scalar_map = self.mpl_cm.ScalarMappable(
            cmap=self.masks_cmap, norm=self.params[self.key]["norm"])

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
        image = np.frombuffer(canvas.tostring_rgb(), dtype='uint8')
        shp = list(fig.canvas.get_width_height())[::-1] + [3]
        self.cbar_image.value = self.pil_image.fromarray(
            image.reshape(shp))._repr_png_()

    def update_colors(self, change):
        arr = self.hist_data_array[self.key][self.slider_dim,
                                             change["new"]].values
        if self.select_rendering.value == "Full":
            arr = np.repeat(arr, self.nverts, axis=0)
        colors = self.scalar_map[self.key].to_rgba(arr).astype(np.float32)
        if self.key in self.masks_variables and self.masks_params[
                self.key]["show"]:
            msk = self.masks_variables[self.key].values
            if self.select_rendering.value == "Full":
                msk = np.repeat(msk, self.nverts, axis=0)
            masks_inds = np.where(msk)
            masks_colors = self.masks_scalar_map.to_rgba(
                arr[masks_inds]).astype(np.float32)
            colors[masks_inds] = masks_colors

        self.geometry.attributes["color"].array = colors[:, :3]

        self.label.value = name_with_unit(
            var=self.hist_data_array[self.key].coords[self.slider_dim],
            name=value_to_string(self.hist_data_array[self.key].coords[
                self.slider_dim].values[change["new"]]))
        return

    def change_projection(self, owner):

        projection = owner.description
        axis = projection[-1]

        # Just move the camera if we are staying as a 3D projection
        if projection.startswith("3D") and self.current_projection.startswith(
                "3D") and (not self.lock_camera):
            new_cam_pos = [
                self.camera_pos * (axis == "X"),
                self.camera_pos * (axis == "Y"),
                self.camera_pos * (axis == "Z")
            ]
            self.camera.position = new_cam_pos
            self.buttons[self.current_projection].button_style = ""
            self.current_projection = projection
            self.buttons[projection].button_style = "info"
            self.update_colors({"new": self.slider.value})
            return

        if self.current_projection in self.buttons:
            self.buttons[self.current_projection].button_style = ""

        # Compute cylindrical or spherical projections
        permutations = {"X": [0, 2, 1], "Y": [1, 0, 2], "Z": [2, 1, 0]}

        if self.select_rendering.value == "Full":

            # Duplicate the detector shape to the number of detectors by
            # creating a Variable of dims ["spectrum", "vertex"]. The rotation
            # operation will then be applied along the "spectrum" dimension
            # using the automatic broadcast
            vertices = sc.Variable(dims=[self.other_dim, "vertex"],
                                   shape=[self.ndets, self.nverts],
                                   unit=sc.units.m,
                                   dtype=sc.dtype.vector_3_float64)
            scaling = np.array(
                self.hist_data_array[self.key].attrs["shape"].values)
            for i in range(self.nverts):
                vertices["vertex", i] = sc.Variable(
                    dims=[self.other_dim],
                    values=np.tile(self.detector_shape[i], [self.ndets, 1]) *
                    scaling,
                    unit=sc.units.m,
                    dtype=sc.dtype.vector_3_float64)

            vertices = self.hist_data_array[
                self.key].attrs["rotation"] * vertices

            pixel_pos = np.array((vertices + self.det_pos).values,
                                 dtype=np.float32)
            pixel_pos = pixel_pos.reshape(-1, pixel_pos.shape[-1])
        else:
            pixel_pos = np.array(self.det_pos.values, dtype=np.float32)

        if projection.startswith("3D"):
            xyz = pixel_pos
        else:
            xyz = np.zeros_like(pixel_pos)
            xyz[:, 0] = np.arctan2(pixel_pos[:, permutations[axis][2]],
                                   pixel_pos[:, permutations[axis][1]])
            if projection.startswith("Cylindrical"):
                xyz[:, 1] = pixel_pos[:, permutations[axis][0]]
                ylab = "z"
            elif projection.startswith("Spherical"):
                xyz[:, 1] = np.arcsin(
                    pixel_pos[:, permutations[axis][0]] /
                    np.sqrt(pixel_pos[:, 0]**2 + pixel_pos[:, 1]**2 +
                            pixel_pos[:, 2]**2))
                ylab = u"\u03B8"

        if not self.lock_camera:
            if projection.startswith("3D"):
                self.axes_2d.visible = False
                self.axes_3d.visible = True
                if self.ticks_2d is not None:
                    self.scene.remove(self.ticks_2d)
                    self.ticks_2d = None
                # Add the ticks
                self.ticks_3d = self.generate_3d_axes_ticks()
                self.scene.add(self.ticks_3d)
                new_cam_pos = [
                    self.camera_pos * (axis == "X"),
                    self.camera_pos * (axis == "Y"),
                    self.camera_pos * (axis == "Z")
                ]
            else:
                self.axes_3d.visible = False
                xmin = np.amin(xyz[:, 0])
                xmax = np.amax(xyz[:, 0])
                ymin = np.amin(xyz[:, 1])
                ymax = np.amax(xyz[:, 1])
                dx = 0.05 * (xmax - xmin)
                dy = 0.05 * (ymax - ymin)
                xmin -= dx
                xmax += dx
                ymin -= dy
                ymax += dy
                self.axes_2d.geometry.attributes["position"].array = np.array(
                    [[xmin, ymin, 0], [xmax, ymin, 0], [xmax, ymax, 0],
                     [xmin, ymax, 0], [xmin, ymin, 0]],
                    dtype=np.float32)
                self.axes_2d.visible = True
                if self.ticks_3d is not None:
                    self.scene.remove(self.ticks_3d)
                    self.ticks_3d = None
                if self.ticks_2d is not None:
                    self.scene.remove(self.ticks_2d)
                self.ticks_2d = self.generate_2d_axes_ticks(
                    xmin, xmax, ymin, ymax, u"\u03C6", ylab)
                self.scene.add(self.ticks_2d)
                new_cam_pos = [0, 0, self.camera_pos]

            self.camera.position = new_cam_pos
            if self._camera_pos is not None:
                self.camera.position = self._camera_pos
            if self._look_at is not None:
                self.renderer.controls = [
                    self.p3.OrbitControls(
                        controlling=self.camera,
                        target=self._look_at,
                        enableRotate=projection.startswith("3D"))
                ]
                self.camera.lookAt(self._look_at)
            else:
                self.renderer.controls = [
                    self.p3.OrbitControls(
                        controlling=self.camera,
                        enableRotate=projection.startswith("3D"))
                ]

        self.geometry.attributes["position"].array = xyz

        self.update_colors({"new": self.slider.value})

        self.current_projection = projection
        self.buttons[projection].button_style = "info"

        return

    def generate_axis_ticks(self,
                            group=None,
                            xmin=0,
                            xmax=1,
                            axis=0,
                            size=1,
                            offset=None,
                            nmax=20,
                            range_start=0):
        ticker = self.mpl_ticker.MaxNLocator(nmax)
        ticks = ticker.tick_values(xmin, xmax)
        iden = np.identity(3, dtype=np.float32)
        for i in range(range_start, len(ticks)):
            tick_pos = iden[axis] * ticks[i]
            if offset is not None:
                tick_pos += offset
            group.add(
                self.make_axis_tick(string=value_to_string(ticks[i]),
                                    position=tick_pos.tolist(),
                                    size=size))
        return

    def generate_3d_axes_ticks(self):
        tick_size = 10.0 * self._pixel_size
        axticks = self.p3.Group()
        axticks.add(self.make_axis_tick("0", [0, 0, 0], size=tick_size))
        for i in range(3):
            self.generate_axis_ticks(group=axticks,
                                     xmin=self.camera_pos * 0.1,
                                     xmax=self.camera_pos * 10.0,
                                     axis=i,
                                     size=tick_size,
                                     range_start=1)
        return axticks

    def generate_2d_axes_ticks(self,
                               xmin=0,
                               xmax=1,
                               ymin=0,
                               ymax=1,
                               xlabel="",
                               ylabel=""):
        tick_size = min(0.05 * (xmax - xmin), 0.05 * (ymax - ymin))
        axticks = self.p3.Group()
        self.generate_axis_ticks(group=axticks,
                                 xmin=xmin,
                                 xmax=xmax,
                                 axis=0,
                                 size=tick_size,
                                 offset=[0, ymin, 0],
                                 nmax=10)
        self.generate_axis_ticks(group=axticks,
                                 xmin=ymin,
                                 xmax=ymax,
                                 axis=1,
                                 size=tick_size,
                                 offset=[xmin, 0, 0],
                                 nmax=10)
        axticks.add(
            self.make_axis_tick(
                string=xlabel,
                position=[0.5 * (xmin + xmax), ymin - 0.1 * (ymax - ymin), 0],
                size=tick_size * 1.5))
        axticks.add(
            self.make_axis_tick(
                string=ylabel,
                position=[xmin - 0.1 * (xmax - xmin), 0.5 * (ymin + ymax), 0],
                size=tick_size * 1.5))
        return axticks

    def update_nbins(self, change):
        if self.lock_bin_inputs:
            return
        try:
            nbins = int(change["new"])
        except ValueError:
            print("Warning: could not convert value: {} to an "
                  "integer.".format(change["new"]))
            return
        self.rebin_data(np.linspace(self.minmax[0], self.minmax[1], nbins + 1))
        x = self.hist_data_array[self.key].coords[self.slider_dim].values
        self.lock_bin_inputs = True
        self.bin_size.value = str(x[1] - x[0])
        self.update_slider()
        self.lock_bin_inputs = False

    def update_bin_size(self, change):
        if self.lock_bin_inputs:
            return
        try:
            binw = float(change["new"])
        except ValueError:
            print("Warning: could not convert value: {} to a "
                  "float.".format(change["new"]))
            return
        self.rebin_data(np.arange(self.minmax[0], self.minmax[1], binw))
        self.lock_bin_inputs = True
        self.nbins.value = str(
            self.hist_data_array[self.key].shape[self.tof_dim_indx])
        self.update_slider()
        self.lock_bin_inputs = False

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
        if self.select_colormap.value == self.params[self.key]["cmap"]:
            self.update_colorbar()
            self.update_colors({"new": self.slider.value})
        else:
            # Changing the colormap triggers the pixel color update
            self.select_colormap.value = self.params[self.key]["cmap"]

    def update_colormap(self, change):
        if change["new"] in self.available_cmaps:
            self.colormap_error.value = ""
            self.params[self.key]["cmap"] = change["new"]
            self.cmap[self.key] = self.mpl_cm.get_cmap(
                self.params[self.key]["cmap"])
            self.cmap[self.key].set_bad(color=self.nan_color)
            self.scalar_map[self.key] = self.mpl_cm.ScalarMappable(
                cmap=self.cmap[self.key], norm=self.params[self.key]["norm"])
            self.update_colorbar()
            self.update_colors({"new": self.slider.value})
        else:
            self.colormap_error.value = ('<span style="color: red;">&times'
                                         '</span>')

    def toggle_masks(self, change):
        self.masks_params[self.key]["show"] = change["new"]
        change["owner"].description = "Hide masks" if change["new"] else \
            "Show masks"
        self.update_colors({"new": self.slider.value})

    def update_masks_colormap(self, change):
        if change["new"] in self.available_cmaps:
            self.masks_colormap_error.value = ""
            self.masks_params[self.key]["cmap"] = change["new"]
            self.masks_cmap = self.mpl_cm.get_cmap(
                self.masks_params[self.key]["cmap"])
            self.masks_scalar_map = self.mpl_cm.ScalarMappable(
                cmap=self.masks_cmap, norm=self.params[self.key]["norm"])
            self.update_colors({"new": self.slider.value})
        else:
            self.masks_colormap_error.value = ('<span style="color: red;">'
                                               '&times</span>')

    def update_masks_solid_color(self, change):
        self.masks_cmap = self.mpl_colors.LinearSegmentedColormap.from_list(
            "tmp", [change["new"], change["new"]])
        self.masks_cmap.set_bad(color=change["new"])
        self.masks_scalar_map = self.mpl_cm.ScalarMappable(
            cmap=self.masks_cmap, norm=self.params[self.key]["norm"])
        self.update_colors({"new": self.slider.value})

    def toggle_color_selection(self, change):
        is_colormap = change["new"] == "colormap"
        self.masks_colormap.disabled = not is_colormap
        self.masks_solid_color.disabled = is_colormap
        if is_colormap:
            self.update_masks_colormap({"new": self.masks_colormap.value})
        else:
            self.update_masks_solid_color(
                {"new": self.masks_solid_color.value})

    def update_opacity(self, change):
        self.material.opacity = change["new"]

    def toggle_continuous_update(self, change):
        self.slider.continuous_update = change["new"]

    def change_rendering(self, change):
        if change["new"] == "Full":
            if self.points is not None and self.points in self.scene.children:
                self.scene.remove(self.points)
                del self.points
            self.geometry, self.material, self.mesh = \
                self.create_mesh_geometry()
            self.scene.add(self.mesh)
        else:
            if self.mesh is not None and self.mesh in self.scene.children:
                self.scene.remove(self.mesh)
                del self.mesh
            self.geometry, self.material, self.points = \
                self.create_points_geometry()
            self.scene.add(self.points)
        if "old" in change:
            self.lock_camera = True
            self.change_projection(self.buttons[self.current_projection])
            self.lock_camera = False
