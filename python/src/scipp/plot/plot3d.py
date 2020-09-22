# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import config
# from .render import render_plot
from .controller3d import PlotController3d
from .model3d import PlotModel3d
from .panel3d import PlotPanel3d
from .sciplot import SciPlot
from .tools import to_bin_centers
from .view3d import PlotView3d

# from .widgets import PlotWidgets
from .._utils import name_with_unit, value_to_string
from .._scipp import core as sc

# Other imports
import numpy as np
import ipywidgets as ipw
from matplotlib import cm
import matplotlib as mpl
import matplotlib.pyplot as plt
# from matplotlib.backends import backend_agg
# import PIL as pil
import pythreejs as p3
from copy import copy
import io


def plot3d(scipp_obj_dict=None,
            positions=None,
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

    sp = SciPlot3d(scipp_obj_dict=scipp_obj_dict,
                  positions=positions,
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

    if filename is not None:
        sp.savefig(filename)

    return sp


class SciPlot3d(Sciplot):
    def __init__(self,
                 scipp_obj_dict=None,
                 positions=None,
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

        # The main controller module which contains the slider widgets
        self.controller = PlotController3d(scipp_obj_dict=scipp_obj_dict,
                         axes=axes,
                         masks=masks,
                         cmap=cmap,
                         log=log,
                         vmin=vmin,
                         vmax=vmax,
                         color=color,
                         positions=positions,
            pixel_size=pixel_size,
            button_options=['X', 'Y', 'Z'])

        # The model which takes care of all heavy calculations
        self.model = PlotModel3d(controller=self.controller,
            scipp_obj_dict=scipp_obj_dict,
            positions=positions,
            cut_options=self.panel.cut_options)

        # The view which will display the 3d scene and send pick events back to
        # the controller
        self.view = PlotView3d(controller=self.controller,
            cmap=self.controller.params["values"][self.controller.name]["cmap"],
            norm=self.controller.params["values"][self.controller.name]["norm"],
            unit=self.controller.params["values"][self.controller.name]["unit"],
            mask_cmap=self.controller.params["masks"][self.controller.name]["cmap"],
            mask_names=self.controller.mask_names[self.controller.name],
            nan_color=nan_color,
            pixel_size=pixel_size,
            tick_size=tick_size,
            background=background,
            show_outline=show_outline)

        # An additional panel view with widgets to control the cut surface
        self.panel = PlotPanel3d(controller=self.controller, pixel_size=pixel_size)

        # Connect controller to model, view, panel and profile
        self._connect_controller_members()

        # Call update_slice once to make the initial image
        self.controller.update_axes()

        return




































#     def nono:

#         self.widgets = PlotWidgets(self, engine=self.engine,

#         # self.vslice = None
#         # self.current_cut_surface_value = None
#         # self.cut_slider_steps = 10.
#         self.cbar_image = ipw.Image()
#         self.cut_options = {
#             "Xplane": 0,
#             "Yplane": 1,
#             "Zplane": 2,
#             "Xcylinder": 3,
#             "Ycylinder": 4,
#             "Zcylinder": 5,
#             "Sphere": 6,
#             "Value": 7
#         }

#         # Prepare colormaps
#         self.cmap = copy(cm.get_cmap(self.engine.params["values"][self.engine.name]["cmap"]))
#         self.cmap.set_bad(color=nan_color)
#         self.scalar_map = cm.ScalarMappable(
#             norm=self.engine.params["values"][self.engine.name]["norm"], cmap=self.cmap)
#         self.masks_scalar_map = None
#         if len(self.engine.data_arrays[self.engine.name].masks) > 0:
#             self.masks_cmap = copy(
#                 cm.get_cmap(self.engine.params["masks"][self.engine.name]["cmap"]))
#             self.masks_cmap.set_bad(color=nan_color)
#             self.masks_scalar_map = cm.ScalarMappable(
#                 norm=self.engine.params["values"][self.engine.name]["norm"],
#                 cmap=self.masks_cmap)

#         # # Generate the colorbar image
#         # self.create_colorbar()

#         # # Useful variables
#         # self.permutations = {"x": ["y", "z"], "y": ["x", "z"], "z": ["x", "y"]}
#         # self.remaining_inds = [0, 1]

#         # Search the coordinates to see if one contains vectors. If so, it will
#         # be used as position vectors.
#         self.axlabels = {"x": "", "y": "", "z": ""}
#         self.positions = None
#         self.pixel_size = pixel_size
#         self.tick_size = tick_size
#         if positions is not None:
#             coord = scipp_obj_dict[self.engine.name].coords[positions]
#             self.positions = np.array(coord.values, dtype=np.float32)
#             self.axlabels.update({
#                 "x": name_with_unit(coord, name="X"),
#                 "y": name_with_unit(coord, name="Y"),
#                 "z": name_with_unit(coord, name="Z")
#             })
#         else:
#             # If no positions are supplied, create a meshgrid from coordinate
#             # axes.
#             coords = []
#             labels = []
#             # for dim, val in self.slider.items():
#             for dim in self.engine.data_arrays[self.engine.name].dims:
#                 # print("in here", dim)
#                 if self.widgets.slider[dim].disabled:
#                     # arr = self.slider_coord[self.engine.name][dim]
#                     # if self.histograms[self.engine.name][dim][dim]:
#                     #     arr = to_bin_centers(arr, dim)
#                     coord = self.engine.data_arrays[self.engine.name].coords[dim]
#                     coords.append(to_bin_centers(coord, dim).values)
#                     labels.append(name_with_unit(coord))
#             # z, y, x = np.meshgrid(*coords, indexing='ij')
#             x, y, z = np.meshgrid(*coords, indexing='ij')
#             self.positions = np.array(
#                 [x.ravel(), y.ravel(), z.ravel()], dtype=np.float32).T
#             if self.pixel_size is None:
#                 self.pixel_size = coords[0][1] - coords[0][0]
#             self.axlabels.update({
#                 "z": labels[0],
#                 "y": labels[1],
#                 "x": labels[2]
#             })

#         # Find spatial and value limits
#         self.xminmax, self.center_of_mass = self.get_spatial_extents()
#         self.vminmax = [
#             sc.min(self.engine.data_arrays[self.engine.name].data).value,
#             sc.max(self.engine.data_arrays[self.engine.name].data).value
#         ]

#         # Create the point cloud with pythreejs
#         self.points_geometry, self.points_material, self.points = \
#             self.create_points_geometry()

#         # Create outline around point positions
#         self.outline, self.axticks = self.create_outline()

#         # Save the size of the outline box for later
#         self.box_size = np.diff(list(self.xminmax.values()), axis=1).ravel()

#         # Define camera: look at the centre of mass of the points
#         camera_lookat = self.center_of_mass
#         camera_pos = np.array(self.center_of_mass) + 1.2 * self.box_size
#         self.camera = p3.PerspectiveCamera(position=list(camera_pos),
#                                            aspect=config.plot.width /
#                                            config.plot.height)

#         # Add red/green/blue axes helper
#         self.axes_3d = p3.AxesHelper(10.0 * np.linalg.norm(camera_pos))

#         # Create the pythreejs scene
#         self.scene = p3.Scene(children=[
#             self.camera, self.axes_3d, self.points, self.outline, self.axticks
#         ],
#                               background=background)

#         # Add camera controller
#         self.controller = p3.OrbitControls(controlling=self.camera,
#                                            target=camera_lookat)
#         self.camera.lookAt(camera_lookat)

#         # Render the scene into a widget
#         self.renderer = p3.Renderer(camera=self.camera,
#                                     scene=self.scene,
#                                     controls=[self.controller],
#                                     width=config.plot.width,
#                                     height=config.plot.height)

#         # Update visibility of outline according to keyword arg
#         self.outline.visible = show_outline
#         self.axticks.visible = show_outline

#         self.create_cut_surface_controls(show_outline)

#         # Generate the colorbar image
#         self.create_colorbar()

#         self.figure = ipw.HBox([self.renderer, self.cbar_image])

#         self.additional_widgets = ipw.HBox([
#                 self.opacity_slider, self.opacity_checkbox,
#                 self.toggle_outline_button
#             ])

#         # self.box = ipw.VBox([
#         #     ipw.HBox([self.renderer, self.cbar_image]),
#         #     ipw.VBox(self.vbox),
#         #     ipw.HBox([
#         #         self.opacity_slider, self.opacity_checkbox,
#         #         self.toggle_outline_button
#         #     ]), self.cut_surface_controls
#         # ])

#         # # Update list of members to be returned in the SciPlot object
#         # self.members.update({
#         #     "camera": self.camera,
#         #     "scene": self.scene,
#         #     "renderer": self.renderer
#         # })

#         return

#     def _ipython_display_(self):
#         return self._to_widget()._ipython_display_()

#     def _to_widget(self):
#         # widgets_ = [self.figure, self.widgets]
#         # if self.overview["additional_widgets"] is not None:
#         #     wdgts.append(self.overview["additional_widgets"])
#         return ipw.VBox([self.figure, self.widgets.container, self.additional_widgets,
#             self.cut_surface_controls])


#     def create_points_geometry(self):
#         """
#         Make a PointsGeometry using pythreejs
#         """
#         points_geometry = p3.BufferGeometry(
#             attributes={
#                 'position': p3.BufferAttribute(array=self.positions),
#                 'rgba_color': p3.BufferAttribute(array=self.engine.slice_data(change=None, autoscale_cmap=True))
#             })
#         points_material = self.create_points_material()
#         points = p3.Points(geometry=points_geometry, material=points_material)
#         return points_geometry, points_material, points

#     def create_points_material(self):
#         """
#         Define custom raw shader for point cloud to allow to RGBA color format.
#         """
#         return p3.ShaderMaterial(
#             vertexShader='''
# precision highp float;
# attribute vec4 rgba_color;
# varying vec3 mypos;
# varying vec4 vColor;
# varying vec4 projected;
# float xDelta, yDelta, zDelta, delta;
# void main(){
#     vColor = rgba_color;
#     gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
#     // use the delta between the point position and camera position for size
#     xDelta = (position[0]-cameraPosition[0]) * (position[0]-cameraPosition[0]);
#     yDelta = (position[1]-cameraPosition[1]) * (position[1]-cameraPosition[1]);
#     zDelta = (position[2]-cameraPosition[2]) * (position[2]-cameraPosition[2]);
#     delta = pow(xDelta + yDelta + zDelta, 0.5);
#     gl_PointSize = %f / delta;
# }
# ''' % (500.0 * self.pixel_size, ),  # the value of 500 is from trial and error
#             fragmentShader='''
# precision highp float;
# varying vec4 vColor;
# void main() {
#     gl_FragColor = vColor;
# }
# ''',
#             vertexColors='VertexColors',
#             transparent=True,
#             depthTest=True)

#     def get_spatial_extents(self):
#         """
#         Find extents of points in 3D
#         """
#         xminmax = {}
#         for i, x in enumerate('xyz'):
#             xminmax[x] = [
#                 np.amin(self.positions[:, i]) - 0.5 * self.pixel_size,
#                 np.amax(self.positions[:, i]) + 0.5 * self.pixel_size
#             ]
#         center_of_mass = [
#             0.5 * np.sum(xminmax['x']), 0.5 * np.sum(xminmax['y']),
#             0.5 * np.sum(xminmax['z'])
#         ]
#         return xminmax, center_of_mass

#     def create_outline(self):
#         """
#         Make a wireframe cube with tick labels
#         """

#         box_geometry = p3.BoxBufferGeometry(
#             self.xminmax['x'][1] - self.xminmax['x'][0],
#             self.xminmax['y'][1] - self.xminmax['y'][0],
#             self.xminmax['z'][1] - self.xminmax['z'][0])
#         edges = p3.EdgesGeometry(box_geometry)
#         outline = p3.LineSegments(
#             geometry=edges,
#             material=p3.LineBasicMaterial(color='#000000'),
#             position=[
#                 0.5 * np.sum(self.xminmax['x']),
#                 0.5 * np.sum(self.xminmax['y']),
#                 0.5 * np.sum(self.xminmax['z'])
#             ])

#         ticks_and_labels = self.generate_axis_ticks_and_labels()

#         return outline, ticks_and_labels

#     def make_axis_tick(self, string, position, color="black", size=1.0):
#         """
#         Make a text-based sprite for axis tick
#         """
#         sm = p3.SpriteMaterial(map=p3.TextTexture(string=string,
#                                                   color=color,
#                                                   size=300,
#                                                   squareTexture=True),
#                                transparent=True)
#         return p3.Sprite(material=sm,
#                          position=position,
#                          scaleToTexture=True,
#                          scale=[size, size, size])

#     def generate_axis_ticks_and_labels(self):
#         """
#         Create ticklabels on outline edges
#         """
#         if self.tick_size is None:
#             self.tick_size = 0.05 * np.amin(
#                 np.diff(list(self.xminmax.values()), axis=1).ravel())
#         ticks_and_labels = p3.Group()
#         iden = np.identity(3, dtype=np.float32)
#         ticker = mpl.ticker.MaxNLocator(5)
#         offsets = {
#             'x': [0, self.xminmax['y'][0], self.xminmax['z'][0]],
#             'y': [self.xminmax['x'][0], 0, self.xminmax['z'][0]],
#             'z': [self.xminmax['x'][0], self.xminmax['y'][0], 0]
#         }

#         for axis, x in enumerate('xyz'):
#             ticks = ticker.tick_values(self.xminmax[x][0], self.xminmax[x][1])
#             for tick in ticks:
#                 if tick >= self.xminmax[x][0] and tick <= self.xminmax[x][1]:
#                     tick_pos = iden[axis] * tick + offsets[x]
#                     ticks_and_labels.add(
#                         self.make_axis_tick(string=value_to_string(
#                             tick, precision=1),
#                                             position=tick_pos.tolist(),
#                                             size=self.tick_size))
#             ticks_and_labels.add(
#                 self.make_axis_tick(
#                     string=self.axlabels[x],
#                     position=(iden[axis] * 0.5 * np.sum(self.xminmax[x]) +
#                               offsets[x]).tolist(),
#                     size=self.tick_size * 0.3 * len(self.axlabels[x])))

#         return ticks_and_labels

#     def create_colorbar(self):
#         """
#         Make image from matplotlib colorbar.
#         """
#         height_inches = config.plot.height / config.plot.dpi

#         fig = plt.figure(figsize=(height_inches * 0.2, height_inches),
#                                 dpi=config.plot.dpi)
#         # canvas = backend_agg.FigureCanvasAgg(fig)
#         ax = fig.add_axes([0.05, 0.02, 0.25, 0.96])
#         cb1 = mpl.colorbar.ColorbarBase(
#             ax,
#             # cmap=cm.get_cmap(self.engine.params["values"][self.engine.name]["cmap"]),
#             cmap=self.scalar_map.get_cmap(),
#             norm=self.scalar_map.norm)
#         cb1.set_label(name_with_unit(var=self.engine.data_arrays[self.engine.name], name=""))

#         buf = io.BytesIO()
#         fig.savefig(buf, format='png')
#         buf.seek(0)
#         self.cbar_image.value = buf.getvalue()





#         # fig = mpl.figure.Figure(figsize=(height_inches * 0.2, height_inches),
#         #                         dpi=config.plot.dpi)
#         # canvas = backend_agg.FigureCanvasAgg(fig)
#         # ax = fig.add_axes([0.05, 0.02, 0.25, 0.96])
#         # cb1 = mpl.colorbar.ColorbarBase(
#         #     ax,
#         #     # cmap=cm.get_cmap(self.engine.params["values"][self.engine.name]["cmap"]),
#         #     cmap=self.scalar_map.get_cmap(),
#         #     norm=self.scalar_map.norm)
#         # cb1.set_label(name_with_unit(var=self.engine.data_arrays[self.engine.name], name=""))
#         # canvas.draw()
#         # image = np.frombuffer(canvas.tostring_rgb(), dtype='uint8')
#         # shp = list(fig.canvas.get_width_height())[::-1] + [3]
#         # self.cbar_image.value = pil.Image.fromarray(
#         #     image.reshape(shp))._repr_png_()


#     def create_cut_surface_controls(self, show_outline):

#         # Opacity slider: top value controls opacity if no cut surface is
#         # active. If a cut curface is present, the upper slider is the opacity
#         # of the slice, while the lower slider value is the opacity of the
#         # data not in the cut surface.
#         self.opacity_slider = ipw.FloatRangeSlider(
#             min=0.0,
#             max=1.0,
#             value=[0.1, 1],
#             step=0.01,
#             description="Opacity slider: When no cut surface is active, the "
#             "max value of the range slider controls the overall opacity, "
#             "and the lower value has no effect. When a cut surface is "
#             "present, the max value is the opacity of the slice, while the "
#             "min value is the opacity of the background.",
#             continuous_update=True,
#             style={'description_width': '60px'})
#         self.opacity_slider.observe(self.engine.update_opacity, names="value")
#         self.opacity_checkbox = ipw.Checkbox(
#             value=self.opacity_slider.continuous_update,
#             description="Continuous update",
#             indent=False,
#             layout={"width": "20px"})
#         self.opacity_checkbox_link = ipw.jslink(
#             (self.opacity_checkbox, 'value'),
#             (self.opacity_slider, 'continuous_update'))

#         self.toggle_outline_button = ipw.ToggleButton(value=show_outline,
#                                                           description='',
#                                                           button_style='')
#         self.toggle_outline_button.observe(self.toggle_outline, names="value")
#         # Run a trigger to update button text
#         self.toggle_outline({"new": show_outline})

#         # Add buttons to provide a choice of different cut surfaces:
#         # - Cartesian X, Y, Z
#         # - Cylindrical X, Y, Z (cylinder major axis)
#         # - Spherical R
#         # - Value-based iso-surface
#         # Note additional spaces required in cylindrical names because
#         # options must be unique.
#         self.cut_surface_buttons = ipw.ToggleButtons(
#             options=[('X ', self.cut_options["Xplane"]),
#                      ('Y ', self.cut_options["Yplane"]),
#                      ('Z ', self.cut_options["Zplane"]),
#                      ('R ', self.cut_options["Sphere"]),
#                      (' X ', self.cut_options["Xcylinder"]),
#                      (' Y ', self.cut_options["Ycylinder"]),
#                      (' Z ', self.cut_options["Zcylinder"]),
#                      ('', self.cut_options["Value"])],
#             value=None,
#             description='Cut surface:',
#             button_style='',
#             tooltips=[
#                 'X-plane', 'Y-plane', 'Z-plane', 'Sphere', 'Cylinder-X',
#                 'Cylinder-Y', 'Cylinder-Z', 'Value'
#             ],
#             icons=(['cube'] * 3) + ['circle-o'] + (['toggle-on'] * 3) +
#             ['magic'],
#             style={"button_width": "55px"},
#             layout={'width': '350px'})
#         self.cut_surface_buttons.observe(self.engine.update_cut_surface_buttons,
#                                          names="value")
#         # Add a capture for a click event: if the active button is clicked,
#         # this resets the togglebuttons value to None and deletes the cut
#         # surface.
#         self.cut_surface_buttons.on_msg(self.engine.check_if_reset_needed)

#         # Allow to change the thickness of the cut surface
#         self.cut_surface_thickness = ipw.BoundedFloatText(
#             value=self.pixel_size * 1.1,
#             min=0,
#             layout={"width": "150px"},
#             disabled=True,
#             description="Thickness:",
#             style={'description_width': 'initial'})

#         # Add slider to control position of cut surface
#         self.cut_slider = ipw.FloatSlider(min=0,
#                                               max=1,
#                                               step=self.cut_surface_thickness.value,
#                                               description="Position:",
#                                               disabled=True,
#                                               value=0.5,
#                                               layout={"width": "350px"})
#         self.cut_checkbox = ipw.Checkbox(value=True,
#                                              description="Continuous update",
#                                              indent=False,
#                                              layout={"width": "20px"},
#                                              disabled=True)
#         self.cut_checkbox_link = ipw.jslink(
#             (self.cut_checkbox, 'value'),
#             (self.cut_slider, 'continuous_update'))
#         self.cut_slider.observe(self.engine.update_cut_surface, names="value")

#         # Allow to change the thickness of the cut surface
#         self.cut_surface_thickness = ipw.BoundedFloatText(
#             value=0.05 * self.box_size.max(),
#             min=0,
#             layout={"width": "150px"},
#             disabled=True,
#             description="Thickness:",
#             style={'description_width': 'initial'})
#         self.cut_surface_thickness.observe(self.engine.update_cut_surface,
#                                            names="value")
#         self.cut_thickness_link = ipw.jslink(
#             (self.cut_slider, 'step'), (self.cut_surface_thickness, 'value'))
#         self.cut_slider.observe(self.engine.update_cut_surface, names="value")

#         # Put widgets into boxes
#         self.cut_surface_controls = ipw.HBox([
#             self.cut_surface_buttons,
#             ipw.VBox([
#                 ipw.HBox([self.cut_slider, self.cut_checkbox]),
#                 self.cut_surface_thickness
#             ])
#         ])
#         return


#     # def update_opacity(self, change):
#     #     """
#     #     Update opacity of all points when opacity slider is changed.
#     #     Take cut surface into account if present.
#     #     """
#     #     if self.cut_surface_buttons.value is None:
#     #         arr = self.points_geometry.attributes["rgba_color"].array
#     #         arr[:, 3] = change["new"][1]
#     #         self.points_geometry.attributes["rgba_color"].array = arr
#     #         # There is a strange effect with point clouds and opacities.
#     #         # Results are best when depthTest is False, at low opacities.
#     #         # But when opacities are high, the points appear in the order
#     #         # they were drawn, and not in the order they are with respect
#     #         # to the camera position. So for high opacities, we switch to
#     #         # depthTest = True.
#     #         self.points_material.depthTest = change["new"][1] > 0.9
#     #     else:
#     #         self.update_cut_surface({"new": self.cut_slider.value})

#     # def check_if_reset_needed(self, owner, content, buffers):
#     #     if owner.value == self.current_cut_surface_value:
#     #         self.cut_surface_buttons.value = None
#     #     self.current_cut_surface_value = owner.value

#     # def update_cut_surface_buttons(self, change):
#     #     if change["new"] is None:
#     #         self.cut_slider.disabled = True
#     #         self.cut_checkbox.disabled = True
#     #         self.cut_surface_thickness.disabled = True
#     #         self.update_opacity({"new": self.opacity_slider.value})
#     #     else:
#     #         self.points_material.depthTest = False
#     #         if change["old"] is None:
#     #             self.cut_slider.disabled = False
#     #             self.cut_checkbox.disabled = False
#     #             self.cut_surface_thickness.disabled = False
#     #         self.update_cut_slider_bounds()

#     # def update_cut_slider_bounds(self):
#     #     # Cartesian X, Y, Z
#     #     if self.cut_surface_buttons.value < self.cut_options["Xcylinder"]:
#     #         minmax = self.xminmax["xyz"[self.cut_surface_buttons.value]]
#     #         if minmax[0] < self.cut_slider.max:
#     #             self.cut_slider.min = minmax[0]
#     #             self.cut_slider.max = minmax[1]
#     #         else:
#     #             self.cut_slider.max = minmax[1]
#     #             self.cut_slider.min = minmax[0]
#     #         self.cut_slider.value = 0.5 * (minmax[0] + minmax[1])
#     #     # Cylindrical X, Y, Z
#     #     elif self.cut_surface_buttons.value < self.cut_options["Sphere"]:
#     #         j = self.cut_surface_buttons.value - 3
#     #         remaining_axes = self.permutations["xyz"[j]]
#     #         self.remaining_inds = [(j + 1) % 3, (j + 2) % 3]
#     #         rmax = np.abs([
#     #             self.xminmax[remaining_axes[0]][0],
#     #             self.xminmax[remaining_axes[1]][0],
#     #             self.xminmax[remaining_axes[0]][1],
#     #             self.xminmax[remaining_axes[1]][1]
#     #         ]).max()
#     #         self.cut_slider.min = 0
#     #         self.cut_slider.max = rmax * np.sqrt(2.0)
#     #         self.cut_slider.value = 0.5 * self.cut_slider.max
#     #     # Spherical
#     #     elif self.cut_surface_buttons.value == self.cut_options["Sphere"]:
#     #         rmax = np.abs(list(self.xminmax.values())).max()
#     #         self.cut_slider.min = 0
#     #         self.cut_slider.max = rmax * np.sqrt(3.0)
#     #         self.cut_slider.value = 0.5 * self.cut_slider.max
#     #     # Value iso-surface
#     #     elif self.cut_surface_buttons.value == self.cut_options["Value"]:
#     #         self.cut_slider.min = self.vminmax[0]
#     #         self.cut_slider.max = self.vminmax[1]
#     #         self.cut_slider.value = 0.5 * (self.vminmax[0] + self.vminmax[1])

#     #     # Update slider step to avoid too fine granularity which slows down
#     #     # interaction. Slice thickness is linked to the step via jslink.
#     #     self.cut_slider.step = (self.cut_slider.max -
#     #                             self.cut_slider.min) / self.cut_slider_steps

#     # def update_cut_surface(self, change):
#     #     newc = None
#     #     target = self.cut_slider.value
#     #     # Cartesian X, Y, Z
#     #     if self.cut_surface_buttons.value < self.cut_options["Xcylinder"]:
#     #         newc = np.where(
#     #             np.abs(self.positions[:, self.cut_surface_buttons.value] -
#     #                    target) < 0.5 * self.cut_surface_thickness.value,
#     #             self.opacity_slider.upper, self.opacity_slider.lower)
#     #     # Cylindrical X, Y, Z
#     #     elif self.cut_surface_buttons.value < self.cut_options["Sphere"]:
#     #         newc = np.where(
#     #             np.abs(
#     #                 np.sqrt(self.positions[:, self.remaining_inds[0]] *
#     #                         self.positions[:, self.remaining_inds[0]] +
#     #                         self.positions[:, self.remaining_inds[1]] *
#     #                         self.positions[:, self.remaining_inds[1]]) -
#     #                 target) < 0.5 * self.cut_surface_thickness.value,
#     #             self.opacity_slider.upper, self.opacity_slider.lower)
#     #     # Spherical
#     #     elif self.cut_surface_buttons.value == self.cut_options["Sphere"]:
#     #         newc = np.where(
#     #             np.abs(
#     #                 np.sqrt(self.positions[:, 0] * self.positions[:, 0] +
#     #                         self.positions[:, 1] * self.positions[:, 1] +
#     #                         self.positions[:, 2] * self.positions[:, 2]) -
#     #                 target) < 0.5 * self.cut_surface_thickness.value,
#     #             self.opacity_slider.upper, self.opacity_slider.lower)
#     #     # Value iso-surface
#     #     elif self.cut_surface_buttons.value == self.cut_options["Value"]:
#     #         newc = np.where(
#     #             np.abs(self.engine.vslice - target) <
#     #             0.5 * self.cut_surface_thickness.value,
#     #             self.opacity_slider.upper, self.opacity_slider.lower)

#     #     # Unfortunately, one cannot edit the value of the geometry array
#     #     # in-place, as this does not trigger an update on the threejs side.
#     #     # We have to update the entire array.
#     #     c3 = self.points_geometry.attributes["rgba_color"].array
#     #     c3[:, 3] = newc
#     #     self.points_geometry.attributes["rgba_color"].array = c3

#     # def slice_data(self, change=None, autoscale_cmap=False):
#     #     """
#     #     Slice the extra dimensions down and update the slice values
#     #     """
#     #     self.vslice = self.engine.data_arrays[self.engine.name]
#     #     # Slice along dimensions with active sliders
#     #     for dim, val in self.slider.items():
#     #         if not val.disabled:
#     #             # self.lab[dim].value = self.make_slider_label(
#     #             #     self.slider_coord[self.engine.name][dim], val.value)
#     #             # self.lab[dim].value = self.make_slider_label(
#     #             #     self.vslice.coords[dim], val.value, self.slider_axformatter[self.engine.name][dim][False])
#     #             # self.vslice = self.vslice[val.dim, val.value]

#     #             deltax = self.thickness_slider[dim].value
#     #             self.vslice = self.resample_image(self.vslice,
#     #                     rebin_edges={dim: sc.Variable([dim], values=[val.value - 0.5 * deltax,
#     #                                                                  val.value + 0.5 * deltax],
#     #                                                         unit=self.vslice.coords[dim].unit)})[dim, 0]
#     #             self.vslice *= (deltax * sc.units.one)


#     #     # Handle masks
#     #     if len(self.masks[self.engine.name]) > 0:
#     #         # Use automatic broadcasting in Scipp variables
#     #         msk = sc.Variable(dims=self.vslice.dims,
#     #                           values=np.zeros(self.vslice.shape,
#     #                                           dtype=np.int32))
#     #         for m in self.masks[self.engine.name]:
#     #             if self.masks[self.engine.name][m].value:
#     #                 msk += sc.Variable(
#     #                     dims=self.vslice.masks[m].dims,
#     #                     values=self.vslice.masks[m].values.astype(np.int32))
#     #         msk = msk.values

#     #     self.vslice = self.vslice.values.flatten()
#     #     if autoscale_cmap:
#     #         self.scalar_map.set_clim(self.vslice.min(), self.vslice.max())
#     #     colors = self.scalar_map.to_rgba(self.vslice).astype(np.float32)

#     #     if len(self.masks[self.engine.name]) > 0:
#     #         masks_inds = np.where(msk.flatten())
#     #         masks_colors = self.masks_scalar_map.to_rgba(
#     #             self.vslice[masks_inds]).astype(np.float32)
#     #         colors[masks_inds] = masks_colors

#     #     return colors

#     # def update_slice(self, change=None, autoscale_cmap=False):
#     #     """
#     #     Update colors of points.
#     #     """
#     #     new_colors = self.slice_data(change=change, autoscale_cmap=autoscale_cmap)
#     #     new_colors[:,
#     #                3] = self.points_geometry.attributes["rgba_color"].array[:,
#     #                                                                         3]
#     #     self.points_geometry.attributes["rgba_color"].array = new_colors
#     #     if self.cut_surface_buttons.value == self.cut_options["Value"]:
#     #         self.update_cut_surface(None)
#     #     return

#     def toggle_mask(self, change):
#         """
#         Show/hide masks
#         """
#         self.engine.update_slice()
#         return

#     def toggle_outline(self, change):
#         self.outline.visible = change["new"]
#         self.axticks.visible = change["new"]
#         desc = "Hide" if change["new"] else "Show"
#         self.toggle_outline_button.description = desc + " outline"

#     def rescale_to_data(self, button=None):
#         # self.scalar_map.set_clim(self.vslice.min(), self.vslice.max())
#         self.engine.update_slice(autoscale_cmap=True)
#         self.create_colorbar()
