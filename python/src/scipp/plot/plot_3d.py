# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import config
from .render import render_plot
from ..plot.sciplot import SciPlot
from .slicer import Slicer
from .tools import edges_to_centers
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


def plot_3d(scipp_obj_dict=None,
            axes=None,
            masks=None,
            filename=None,
            figsize=None,
            aspect=None,
            cmap=None,
            log=False,
            vmin=None,
            vmax=None,
            color=None,
            background="#f0f0f0",
            nan_color="#d3d3d3",
            pixel_size=1.0,
            tick_size=None,
            show_outline=True):
    """
    Plot a 3D point cloud through a N dimensional dataset.
    For every dimension above 3, a slider is created to adjust the position of
    the slice in that particular dimension.
    It is possible to add cut surfaces as cartesian, cylindrical or spherical
    planes.
    """

    sv = Slicer3d(scipp_obj_dict=scipp_obj_dict,
                  axes=axes,
                  masks=masks,
                  cmap=cmap,
                  log=log,
                  vmin=vmin,
                  vmax=vmax,
                  color=color,
                  aspect=aspect,
                  background=background,
                  nan_color=nan_color,
                  pixel_size=pixel_size,
                  tick_size=tick_size,
                  show_outline=show_outline)

    render_plot(widgets=sv.box, filename=filename)

    return SciPlot(sv.members)


class Slicer3d(Slicer):
    def __init__(self,
                 scipp_obj_dict=None,
                 axes=None,
                 masks=None,
                 cmap=None,
                 log=None,
                 vmin=None,
                 vmax=None,
                 color=None,
                 aspect=None,
                 background=None,
                 nan_color=None,
                 pixel_size=None,
                 tick_size=None,
                 show_outline=True):

        super().__init__(scipp_obj_dict=scipp_obj_dict,
                         axes=axes,
                         masks=masks,
                         cmap=cmap,
                         log=log,
                         vmin=vmin,
                         vmax=vmax,
                         color=color,
                         aspect=aspect,
                         button_options=['X', 'Y', 'Z'])

        self.vslice = None
        self.current_cut_surface_value = None
        self.cut_slider_steps = 10.
        self.cbar_image = widgets.Image()
        self.cut_options = {
            "Xplane": 0,
            "Yplane": 1,
            "Zplane": 2,
            "Xcylinder": 3,
            "Ycylinder": 4,
            "Zcylinder": 5,
            "Sphere": 6,
            "Value": 7
        }

        # Prepare colormaps
        self.cmap = cm.get_cmap(self.params["values"][self.name]["cmap"])
        self.cmap.set_bad(color=nan_color)
        self.scalar_map = cm.ScalarMappable(
            norm=self.params["values"][self.name]["norm"], cmap=self.cmap)
        self.masks_scalar_map = None
        if self.params["masks"][self.name]["show"]:
            self.masks_cmap = cm.get_cmap(
                self.params["masks"][self.name]["cmap"])
            self.masks_cmap.set_bad(color=nan_color)
            self.masks_scalar_map = cm.ScalarMappable(
                norm=self.params["values"][self.name]["norm"],
                cmap=self.masks_cmap)

        # Generate the colorbar image
        self.create_colorbar()

        # Useful variables
        self.permutations = {"x": ["y", "z"], "y": ["x", "z"], "z": ["x", "y"]}
        self.remaining_inds = [0, 1]

        # Search the coordinates to see if one contains vectors. If so, it will
        # be used as position vectors.
        self.axlabels = {"x": "", "y": "", "z": ""}
        self.positions = None
        self.pixel_size = pixel_size
        self.tick_size = tick_size
        for coord in self.data_array.coords.values():
            if coord.dtype == sc.dtype.vector_3_float64 and len(
                    coord.dims) > 0:
                self.positions = np.array(coord.values, dtype=np.float32)
                self.axlabels.update({
                    "x": name_with_unit(coord, name="X"),
                    "y": name_with_unit(coord, name="Y"),
                    "z": name_with_unit(coord, name="Z")
                })
                break
        # If no positions are found, create a meshgrid from coordinate axes.
        if self.positions is None:
            coords = []
            labels = []
            for dim, val in self.slider.items():
                if val.disabled:
                    arr = self.slider_x[self.name][dim].values
                    if self.histograms[self.name][dim]:
                        arr = edges_to_centers(arr)
                    coords.append(arr)
                    labels.append(name_with_unit(
                        self.slider_x[self.name][dim]))
            z, y, x = np.meshgrid(*coords, indexing='ij')
            self.positions = np.array(
                [x.ravel(), y.ravel(), z.ravel()], dtype=np.float32).T
            if self.pixel_size is None:
                self.pixel_size = coords[0][1] - coords[0][0]
            self.axlabels.update({
                "z": labels[0],
                "y": labels[1],
                "x": labels[2]
            })

        # Find spatial and value limits
        self.xminmax, self.center_of_mass = self.get_spatial_extents()
        self.vminmax = [
            sc.min(self.data_array.data).value,
            sc.max(self.data_array.data).value
        ]

        # Create the point cloud with pythreejs
        self.points_geometry, self.points_material, self.points = \
            self.create_points_geometry()

        # Create outline around point positions
        self.outline, self.axticks = self.create_outline()

        # Save the size of the outline box for later
        self.box_size = np.diff(list(self.xminmax.values()), axis=1).ravel()

        # Define camera: look at the centre of mass of the points
        camera_lookat = self.center_of_mass
        camera_pos = np.array(self.center_of_mass) + 1.2 * self.box_size
        self.camera = p3.PerspectiveCamera(position=list(camera_pos),
                                           aspect=config.plot.width /
                                           config.plot.height)

        # Add red/green/blue axes helper
        self.axes_3d = p3.AxesHelper(10.0 * np.linalg.norm(camera_pos))

        # Create the pythreejs scene
        self.scene = p3.Scene(children=[
            self.camera, self.axes_3d, self.points, self.outline, self.axticks
        ],
                              background=background)

        # Add camera controller
        self.controller = p3.OrbitControls(controlling=self.camera,
                                           target=camera_lookat)
        self.camera.lookAt(camera_lookat)

        # Render the scene into a widget
        self.renderer = p3.Renderer(camera=self.camera,
                                    scene=self.scene,
                                    controls=[self.controller],
                                    width=config.plot.width,
                                    height=config.plot.height)

        # Update visibility of outline according to keyword arg
        self.outline.visible = show_outline
        self.axticks.visible = show_outline

        # Opacity slider: top value controls opacity if no cut surface is
        # active. If a cut curface is present, the upper slider is the opacity
        # of the slice, while the lower slider value is the opacity of the
        # data not in the cut surface.
        self.opacity_slider = widgets.FloatRangeSlider(
            min=0.0,
            max=1.0,
            value=[0.1, 1],
            step=0.01,
            description="Opacity slider: When no cut surface is active, the max "
            "value of the range slider controls the overall "
            "opacity, and the lower value has no effect. "
            "When a cut surface is present, the max "
            "value is the opacity of the slice, while the min "
            "value is the opacity of the background.",
            continuous_update=True,
            style={'description_width': '60px'})
        self.opacity_slider.observe(self.update_opacity, names="value")
        self.opacity_checkbox = widgets.Checkbox(
            value=self.opacity_slider.continuous_update,
            description="Continuous update",
            indent=False,
            layout={"width": "20px"})
        self.opacity_checkbox_link = widgets.jslink(
            (self.opacity_checkbox, 'value'),
            (self.opacity_slider, 'continuous_update'))

        self.toggle_outline_button = widgets.ToggleButton(value=show_outline,
                                                          description='',
                                                          button_style='')
        self.toggle_outline_button.observe(self.toggle_outline, names="value")
        # Run a trigger to update button text
        self.toggle_outline({"new": show_outline})

        # Add buttons to provide a choice of different cut surfaces:
        # - Cartesian X, Y, Z
        # - Cylindrical X, Y, Z (cylinder major axis)
        # - Sperical R
        # - Value-based iso-surface
        # Note additional spaces required in cylindrical names because
        # options must be unique.
        self.cut_surface_buttons = widgets.ToggleButtons(
            options=[('X ', self.cut_options["Xplane"]),
                     ('Y ', self.cut_options["Yplane"]),
                     ('Z ', self.cut_options["Zplane"]),
                     ('R ', self.cut_options["Sphere"]),
                     (' X ', self.cut_options["Xcylinder"]),
                     (' Y ', self.cut_options["Ycylinder"]),
                     (' Z ', self.cut_options["Zcylinder"]),
                     ('', self.cut_options["Value"])],
            value=None,
            description='Cut surface:',
            button_style='',
            tooltips=[
                'X-plane', 'Y-plane', 'Z-plane', 'Sphere', 'Cylinder-X',
                'Cylinder-Y', 'Cylinder-Z', 'Value'
            ],
            icons=(['cube'] * 3) + ['circle-o'] + (['toggle-on'] * 3) +
            ['magic'],
            style={"button_width": "55px"},
            layout={'width': '350px'})
        self.cut_surface_buttons.observe(self.update_cut_surface_buttons,
                                         names="value")
        # Add a capture for a click event: if the active button is clicked,
        # this resets the togglebuttons value to None and deletes the cut
        # surface.
        self.cut_surface_buttons.on_msg(self.check_if_reset_needed)

        # Add slider to control position of cut surface
        self.cut_slider = widgets.FloatSlider(min=0,
                                              max=1,
                                              description="Position:",
                                              disabled=True,
                                              value=0.5,
                                              layout={"width": "350px"})
        self.cut_checkbox = widgets.Checkbox(value=True,
                                             description="Continuous update",
                                             indent=False,
                                             layout={"width": "20px"},
                                             disabled=True)
        self.cut_checkbox_link = widgets.jslink(
            (self.cut_checkbox, 'value'),
            (self.cut_slider, 'continuous_update'))
        self.cut_slider.observe(self.update_cut_surface, names="value")

        # Allow to change the thickness of the cut surface
        self.cut_surface_thickness = widgets.BoundedFloatText(
            value=0.05 * self.box_size.max(),
            min=0,
            layout={"width": "150px"},
            disabled=True,
            description="Thickness:",
            style={'description_width': 'initial'})
        self.cut_surface_thickness.observe(self.update_cut_surface,
                                           names="value")
        self.cut_thickness_link = widgets.jslink(
            (self.cut_slider, 'step'), (self.cut_surface_thickness, 'value'))
        self.cut_slider.observe(self.update_cut_surface, names="value")

        # Put widgets into boxes
        self.cut_surface_controls = widgets.HBox([
            self.cut_surface_buttons,
            widgets.VBox([
                widgets.HBox([self.cut_slider, self.cut_checkbox]),
                self.cut_surface_thickness
            ])
        ])

        self.box = widgets.VBox([
            widgets.HBox([self.renderer, self.cbar_image]),
            widgets.VBox(self.vbox),
            widgets.HBox([
                self.opacity_slider, self.opacity_checkbox,
                self.toggle_outline_button
            ]), self.cut_surface_controls
        ])

        # Update list of members to be returned in the SciPlot object
        self.members.update({
            "camera": self.camera,
            "scene": self.scene,
            "renderer": self.renderer
        })

        return

    def create_points_geometry(self):
        """
        Make a PointsGeometry using pythreejs
        """
        points_geometry = p3.BufferGeometry(
            attributes={
                'position': p3.BufferAttribute(array=self.positions),
                'rgba_color': p3.BufferAttribute(array=self.slice_data(None))
            })
        points_material = self.create_points_material()
        points = p3.Points(geometry=points_geometry, material=points_material)
        return points_geometry, points_material, points

    def create_points_material(self):
        """
        Define custom raw shader for point cloud to allow to RGBA color format.
        """
        return p3.ShaderMaterial(
            vertexShader='''
precision highp float;
attribute vec4 rgba_color;
varying vec3 mypos;
varying vec4 vColor;
varying vec4 projected;
float xDelta, yDelta, zDelta, delta;
void main(){
    vColor = rgba_color;
    gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
    // use the delta between the point position and camera position for size
    xDelta = (position[0]-cameraPosition[0]) * (position[0]-cameraPosition[0]);
    yDelta = (position[1]-cameraPosition[1]) * (position[1]-cameraPosition[1]);
    zDelta = (position[2]-cameraPosition[2]) * (position[2]-cameraPosition[2]);
    delta = pow(xDelta + yDelta + zDelta, 0.5);
    gl_PointSize = %f / delta;
}
''' % (500.0 * self.pixel_size, ),  # the value of 500 is from trial and error
            fragmentShader='''
precision highp float;
varying vec4 vColor;
void main() {
    gl_FragColor = vColor;
}
''',
            vertexColors='VertexColors',
            transparent=True,
            depthTest=True)

    def get_spatial_extents(self):
        """
        Find extents of points in 3D
        """
        xminmax = {}
        for i, x in enumerate('xyz'):
            xminmax[x] = [
                np.amin(self.positions[:, i]) - 0.5 * self.pixel_size,
                np.amax(self.positions[:, i]) + 0.5 * self.pixel_size
            ]
        center_of_mass = [
            0.5 * np.sum(xminmax['x']), 0.5 * np.sum(xminmax['y']),
            0.5 * np.sum(xminmax['z'])
        ]
        return xminmax, center_of_mass

    def create_outline(self):
        """
        Make a wireframe cube with tick labels
        """

        box_geometry = p3.BoxBufferGeometry(
            self.xminmax['x'][1] - self.xminmax['x'][0],
            self.xminmax['y'][1] - self.xminmax['y'][0],
            self.xminmax['z'][1] - self.xminmax['z'][0])
        edges = p3.EdgesGeometry(box_geometry)
        outline = p3.LineSegments(
            geometry=edges,
            material=p3.LineBasicMaterial(color='#000000'),
            position=[
                0.5 * np.sum(self.xminmax['x']),
                0.5 * np.sum(self.xminmax['y']),
                0.5 * np.sum(self.xminmax['z'])
            ])

        ticks_and_labels = self.generate_axis_ticks_and_labels()

        return outline, ticks_and_labels

    def make_axis_tick(self, string, position, color="black", size=1.0):
        """
        Make a text-based sprite for axis tick
        """
        sm = p3.SpriteMaterial(map=p3.TextTexture(string=string,
                                                  color=color,
                                                  size=300,
                                                  squareTexture=True),
                               transparent=True)
        return p3.Sprite(material=sm,
                         position=position,
                         scaleToTexture=True,
                         scale=[size, size, size])

    def generate_axis_ticks_and_labels(self):
        """
        Create ticklabels on outline edges
        """
        if self.tick_size is None:
            self.tick_size = 0.05 * np.amin(
                np.diff(list(self.xminmax.values()), axis=1).ravel())
        ticks_and_labels = p3.Group()
        iden = np.identity(3, dtype=np.float32)
        ticker = mpl.ticker.MaxNLocator(5)
        offsets = {
            'x': [0, self.xminmax['y'][0], self.xminmax['z'][0]],
            'y': [self.xminmax['x'][0], 0, self.xminmax['z'][0]],
            'z': [self.xminmax['x'][0], self.xminmax['y'][0], 0]
        }

        for axis, x in enumerate('xyz'):
            ticks = ticker.tick_values(self.xminmax[x][0], self.xminmax[x][1])
            for tick in ticks:
                if tick >= self.xminmax[x][0] and tick <= self.xminmax[x][1]:
                    tick_pos = iden[axis] * tick + offsets[x]
                    ticks_and_labels.add(
                        self.make_axis_tick(string=value_to_string(
                            tick, precision=1),
                                            position=tick_pos.tolist(),
                                            size=self.tick_size))
            ticks_and_labels.add(
                self.make_axis_tick(
                    string=self.axlabels[x],
                    position=(iden[axis] * 0.5 * np.sum(self.xminmax[x]) +
                              offsets[x]).tolist(),
                    size=self.tick_size * 0.3 * len(self.axlabels[x])))

        return ticks_and_labels

    def create_colorbar(self):
        """
        Make image from matplotlib colorbar.
        """
        height_inches = config.plot.height / config.plot.dpi
        fig = mpl.figure.Figure(figsize=(height_inches * 0.2, height_inches),
                                dpi=config.plot.dpi)
        canvas = backend_agg.FigureCanvasAgg(fig)
        ax = fig.add_axes([0.05, 0.02, 0.25, 0.96])
        cb1 = mpl.colorbar.ColorbarBase(
            ax,
            cmap=cm.get_cmap(self.params["values"][self.name]["cmap"]),
            norm=self.params["values"][self.name]["norm"])
        cb1.set_label(name_with_unit(var=self.data_array, name=""))
        canvas.draw()
        image = np.frombuffer(canvas.tostring_rgb(), dtype='uint8')
        shp = list(fig.canvas.get_width_height())[::-1] + [3]
        self.cbar_image.value = pil.Image.fromarray(
            image.reshape(shp))._repr_png_()

    def update_opacity(self, change):
        """
        Update opacity of all points when opacity slider is changed.
        Take cut surface into account if present.
        """
        if self.cut_surface_buttons.value is None:
            arr = self.points_geometry.attributes["rgba_color"].array
            arr[:, 3] = change["new"][1]
            self.points_geometry.attributes["rgba_color"].array = arr
            # There is a strange effect with point clouds and opacities.
            # Results are best when depthTest is False, at low opacities.
            # But when opacities are high, the points appear in the order
            # they were drawn, and not in the order they are with respect
            # to the camera position. So for high opacities, we switch to
            # depthTest = True.
            self.points_material.depthTest = change["new"][1] > 0.9
        else:
            self.update_cut_surface({"new": self.cut_slider.value})

    def check_if_reset_needed(self, owner, content, buffers):
        if owner.value == self.current_cut_surface_value:
            self.cut_surface_buttons.value = None
        self.current_cut_surface_value = owner.value

    def update_cut_surface_buttons(self, change):
        if change["new"] is None:
            self.cut_slider.disabled = True
            self.cut_checkbox.disabled = True
            self.cut_surface_thickness.disabled = True
            self.update_opacity({"new": self.opacity_slider.value})
        else:
            self.points_material.depthTest = False
            if change["old"] is None:
                self.cut_slider.disabled = False
                self.cut_checkbox.disabled = False
                self.cut_surface_thickness.disabled = False
            self.update_cut_slider_bounds()

    def update_cut_slider_bounds(self):
        # Cartesian X, Y, Z
        if self.cut_surface_buttons.value < self.cut_options["Xcylinder"]:
            minmax = self.xminmax["xyz"[self.cut_surface_buttons.value]]
            if minmax[0] < self.cut_slider.max:
                self.cut_slider.min = minmax[0]
                self.cut_slider.max = minmax[1]
            else:
                self.cut_slider.max = minmax[1]
                self.cut_slider.min = minmax[0]
            self.cut_slider.value = 0.5 * (minmax[0] + minmax[1])
        # Cylindrical X, Y, Z
        elif self.cut_surface_buttons.value < self.cut_options["Sphere"]:
            j = self.cut_surface_buttons.value - 3
            remaining_axes = self.permutations["xyz"[j]]
            self.remaining_inds = [(j + 1) % 3, (j + 2) % 3]
            rmax = np.abs([
                self.xminmax[remaining_axes[0]][0],
                self.xminmax[remaining_axes[1]][0],
                self.xminmax[remaining_axes[0]][1],
                self.xminmax[remaining_axes[1]][1]
            ]).max()
            self.cut_slider.min = 0
            self.cut_slider.max = rmax * np.sqrt(2.0)
            self.cut_slider.value = 0.5 * self.cut_slider.max
        # Spherical
        elif self.cut_surface_buttons.value == self.cut_options["Sphere"]:
            rmax = np.abs(list(self.xminmax.values())).max()
            self.cut_slider.min = 0
            self.cut_slider.max = rmax * np.sqrt(3.0)
            self.cut_slider.value = 0.5 * self.cut_slider.max
        # Value iso-surface
        elif self.cut_surface_buttons.value == self.cut_options["Value"]:
            self.cut_slider.min = self.vminmax[0]
            self.cut_slider.max = self.vminmax[1]
            self.cut_slider.value = 0.5 * (self.vminmax[0] + self.vminmax[1])

        # Update slider step to avoid too fine granularity which slows down
        # interaction. Slice thickness is linked to the step via jslink.
        self.cut_slider.step = (self.cut_slider.max -
                                self.cut_slider.min) / self.cut_slider_steps

    def update_cut_surface(self, change):
        newc = None
        target = self.cut_slider.value
        # Cartesian X, Y, Z
        if self.cut_surface_buttons.value < self.cut_options["Xcylinder"]:
            newc = np.where(
                np.abs(self.positions[:, self.cut_surface_buttons.value] -
                       target) < 0.5 * self.cut_surface_thickness.value,
                self.opacity_slider.upper, self.opacity_slider.lower)
        # Cylindrical X, Y, Z
        elif self.cut_surface_buttons.value < self.cut_options["Sphere"]:
            newc = np.where(
                np.abs(
                    np.sqrt(self.positions[:, self.remaining_inds[0]] *
                            self.positions[:, self.remaining_inds[0]] +
                            self.positions[:, self.remaining_inds[1]] *
                            self.positions[:, self.remaining_inds[1]]) -
                    target) < 0.5 * self.cut_surface_thickness.value,
                self.opacity_slider.upper, self.opacity_slider.lower)
        # Spherical
        elif self.cut_surface_buttons.value == self.cut_options["Sphere"]:
            newc = np.where(
                np.abs(
                    np.sqrt(self.positions[:, 0] * self.positions[:, 0] +
                            self.positions[:, 1] * self.positions[:, 1] +
                            self.positions[:, 2] * self.positions[:, 2]) -
                    target) < 0.5 * self.cut_surface_thickness.value,
                self.opacity_slider.upper, self.opacity_slider.lower)
        # Value iso-surface
        elif self.cut_surface_buttons.value == self.cut_options["Value"]:
            newc = np.where(
                np.abs(self.vslice - target) <
                0.5 * self.cut_surface_thickness.value,
                self.opacity_slider.upper, self.opacity_slider.lower)

        # Unfortunately, one cannot edit the value of the geometry array
        # in-place, as this does not trigger an update on the threejs side.
        # We have to update the entire array.
        c3 = self.points_geometry.attributes["rgba_color"].array
        c3[:, 3] = newc
        self.points_geometry.attributes["rgba_color"].array = c3

    def slice_data(self, change):
        """
        Slice the extra dimensions down and update the slice values
        """
        self.vslice = self.data_array
        if self.params["masks"][self.name]["show"]:
            mslice = self.masks
        # Slice along dimensions with active sliders
        for dim, val in self.slider.items():
            if not val.disabled:
                self.lab[dim].value = self.make_slider_label(
                    self.slider_x[self.name][dim], val.value)
                self.vslice = self.vslice[val.dim, val.value]
                # At this point, after masks were combined, all their
                # dimensions should be contained in the data_array.dims.
                if self.params["masks"][
                        self.name]["show"] and dim in mslice.dims:
                    mslice = mslice[val.dim, val.value]

        if self.params["masks"][self.name]["show"]:
            # Use automatic broadcasting in Scipp variables
            msk = sc.Variable(dims=self.vslice.dims,
                              values=np.ones(self.vslice.shape,
                                             dtype=np.int32))
            msk *= sc.Variable(dims=mslice.dims,
                               values=mslice.values.astype(np.int32))
            msk = msk.values

        self.vslice = self.vslice.values.flatten()
        colors = self.scalar_map.to_rgba(self.vslice).astype(np.float32)

        if self.params["masks"][self.name]["show"]:
            masks_inds = np.where(msk.flatten())
            masks_colors = self.masks_scalar_map.to_rgba(
                self.vslice[masks_inds]).astype(np.float32)
            colors[masks_inds] = masks_colors

        return colors

    def update_slice(self, change):
        """
        Update colors of points.
        """
        new_colors = self.slice_data(change)
        new_colors[:,
                   3] = self.points_geometry.attributes["rgba_color"].array[:,
                                                                            3]
        self.points_geometry.attributes["rgba_color"].array = new_colors
        if self.cut_surface_buttons.value == self.cut_options["Value"]:
            self.update_cut_surface(None)
        return

    def toggle_masks(self, change):
        """
        Show/hide masks
        """
        self.params["masks"][self.name]["show"] = change["new"]
        change["owner"].description = "Hide masks" if change["new"] else \
            "Show masks"
        self.update_slice(None)
        return

    def toggle_outline(self, change):
        self.outline.visible = change["new"]
        self.axticks.visible = change["new"]
        desc = "Hide" if change["new"] else "Show"
        self.toggle_outline_button.description = desc + " outline"
