# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import config
# from .render import render_plot
from .model3d import PlotModel3d
from .tools import to_bin_centers
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







class PlotView3d:
    def __init__(self,
                 # scipp_obj_dict=None,
                 # axes=None,
                 # masks=None,
                 controller=None,
                 cmap=None,
                 norm=None,
                 title=None,
                 unit=None,
                 log=None,
                 vmin=None,
                 vmax=None,
                 nan_color=None,
                 mask_cmap=None,
                 mask_names=None):

        self.controller = controller




        self.cbar_image = ipw.Image()
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
        self.cmap = copy(cm.get_cmap(cmap))
        self.cmap.set_bad(color=nan_color)
        self.scalar_map = cm.ScalarMappable(
            norm=norm, cmap=self.cmap)
        self.masks_scalar_map = None
        if len(mask_names) > 0:
            self.masks_cmap = copy(
                cm.get_cmap(self.engine.params["masks"][self.engine.name]["cmap"]))
            self.masks_cmap.set_bad(color=nan_color)
            self.masks_scalar_map = cm.ScalarMappable(
                norm=self.engine.params["values"][self.engine.name]["norm"],
                cmap=self.masks_cmap)

        # # Generate the colorbar image
        # self.create_colorbar()

        # # Useful variables
        # self.permutations = {"x": ["y", "z"], "y": ["x", "z"], "z": ["x", "y"]}
        # self.remaining_inds = [0, 1]

        # Search the coordinates to see if one contains vectors. If so, it will
        # be used as position vectors.
        self.axlabels = {"x": "", "y": "", "z": ""}
        self.positions = None
        self.pixel_size = pixel_size
        self.tick_size = tick_size
        # if positions is not None:
        #     coord = scipp_obj_dict[self.engine.name].coords[positions]
        #     self.positions = np.array(coord.values, dtype=np.float32)
        #     self.axlabels.update({
        #         "x": name_with_unit(coord, name="X"),
        #         "y": name_with_unit(coord, name="Y"),
        #         "z": name_with_unit(coord, name="Z")
        #     })
        # else:
        #     # If no positions are supplied, create a meshgrid from coordinate
        #     # axes.
        #     coords = []
        #     labels = []
        #     # for dim, val in self.slider.items():
        #     for dim in self.engine.data_arrays[self.engine.name].dims:
        #         # print("in here", dim)
        #         if self.widgets.slider[dim].disabled:
        #             # arr = self.slider_coord[self.engine.name][dim]
        #             # if self.histograms[self.engine.name][dim][dim]:
        #             #     arr = to_bin_centers(arr, dim)
        #             coord = self.engine.data_arrays[self.engine.name].coords[dim]
        #             coords.append(to_bin_centers(coord, dim).values)
        #             labels.append(name_with_unit(coord))
        #     # z, y, x = np.meshgrid(*coords, indexing='ij')
        #     x, y, z = np.meshgrid(*coords, indexing='ij')
        #     self.positions = np.array(
        #         [x.ravel(), y.ravel(), z.ravel()], dtype=np.float32).T
        #     if self.pixel_size is None:
        #         self.pixel_size = coords[0][1] - coords[0][0]
        #     self.axlabels.update({
        #         "z": labels[0],
        #         "y": labels[1],
        #         "x": labels[2]
        #     })

        # # Find spatial and value limits
        # self.xminmax, self.center_of_mass = self.get_spatial_extents()
        # self.vminmax = [
        #     sc.min(self.engine.data_arrays[self.engine.name].data).value,
        #     sc.max(self.engine.data_arrays[self.engine.name].data).value
        # ]

        # Create the point cloud material with pythreejs
        self.points_material = self.create_points_material()
        self.points_geometry = None
        self.point_cloud = None
        self.outline = None
        self.axticks = None



        # self.points_geometry, self.points_material, self.points = \
        #     self.create_points_geometry()

        # # Create outline around point positions
        # self.outline, self.axticks = self.create_outline()

        # # Save the size of the outline box for later
        # self.box_size = np.diff(list(self.xminmax.values()), axis=1).ravel()

        # Define camera
        # camera_lookat = self.center_of_mass
        # camera_pos = np.array(self.center_of_mass) + 1.2 * self.box_size

        self.camera = p3.PerspectiveCamera(position=[0, 0, 0],
                                           aspect=config.plot.width /
                                           config.plot.height)

        # Add red/green/blue axes helper
        # self.axes_3d = p3.AxesHelper(10.0 * np.linalg.norm(camera_pos))
        self.axes_3d = p3.AxesHelper()

        # Create the pythreejs scene
        self.scene = p3.Scene(children=[self.camera, self.axes_3d],
                              background=background)

        # Add camera controller
        self.controls = p3.OrbitControls(controlling=self.camera)
                                           # target=camera_lookat)
        # self.camera.lookAt(camera_lookat)

        # Render the scene into a widget
        self.renderer = p3.Renderer(camera=self.camera,
                                    scene=self.scene,
                                    controls=[self.controls],
                                    width=config.plot.width,
                                    height=config.plot.height)

        # Update visibility of outline according to keyword arg
        self.outline.visible = show_outline
        self.axticks.visible = show_outline

        self.create_cut_surface_controls(show_outline)

        # Generate the colorbar image
        self.create_colorbar()

        self.figure = ipw.HBox([self.renderer, self.cbar_image])

        self.additional_widgets = ipw.HBox([
                self.opacity_slider, self.opacity_checkbox,
                self.toggle_outline_button
            ])

        # self.box = ipw.VBox([
        #     ipw.HBox([self.renderer, self.cbar_image]),
        #     ipw.VBox(self.vbox),
        #     ipw.HBox([
        #         self.opacity_slider, self.opacity_checkbox,
        #         self.toggle_outline_button
        #     ]), self.cut_surface_controls
        # ])

        # # Update list of members to be returned in the SciPlot object
        # self.members.update({
        #     "camera": self.camera,
        #     "scene": self.scene,
        #     "renderer": self.renderer
        # })

        return

    def _ipython_display_(self):
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        # widgets_ = [self.figure, self.widgets]
        # if self.overview["additional_widgets"] is not None:
        #     wdgts.append(self.overview["additional_widgets"])
        return ipw.VBox([self.figure, self.widgets.container, self.additional_widgets,
            self.cut_surface_controls])




    def update_axes(self, axparams, axformatter=None, axlocator=None, logx=None, logy=None):
        self.scene.remove(self.point_cloud)
        self.scene.remove(self.outline)
        self.scene.remove(self.axticks)

        self.create_point_cloud(axparams["pos"])
        self.create_outline(axparams)
        # Define camera: look at the centre of mass of the points
        # camera_lookat = self.center_of_mass

        # box_size = axparams['x']["lims"][1] - axparams['x']["lims"][0],
        #     axparams['y']["lims"][1] - axparams['y']["lims"][0],
        #     axparams['z']["lims"][1] - axparams['z']["lims"][0]
        self.camera.position = np.array(axparams["centre"]) + 1.2 * axparams["box_size"]
        # Set camera controller target
        self.controls.target = axparams["centre"]
        self.camera.lookAt(axparams["centre"])
        self.axes_3d.size = 10.0 * np.linalg.norm(self.camera.position)

        self.scene.add(self.point_cloud)
        self.scene.add(self.outline)
        self.scene.add(self.axticks)



    # def create_points_geometry(self, pos_array):
    #     """
    #     Make a PointsGeometry using pythreejs
    #     """
    #     rgba_shape = list(pos_array.shape)
    #     rgba_shape[1] += 1
    #     self.points_geometry = p3.BufferGeometry(
    #         attributes={
    #             'position': p3.BufferAttribute(array=pos_array),
    #             # 'rgba_color': p3.BufferAttribute(array=self.engine.slice_data(change=None, autoscale_cmap=True))
    #             'rgba_color': p3.BufferAttribute(array=np.ones(rgba_shape))
    #         })

    def create_point_cloud(self, pos_array):
        """
        Make a PointsGeometry using pythreejs
        """
        rgba_shape = list(pos_array.shape)
        rgba_shape[1] += 1
        self.points_geometry = p3.BufferGeometry(
            attributes={
                'position': p3.BufferAttribute(array=pos_array),
                # 'rgba_color': p3.BufferAttribute(array=self.engine.slice_data(change=None, autoscale_cmap=True))
                'rgba_color': p3.BufferAttribute(array=np.ones(rgba_shape))
            })
        # points_material = self.create_points_material()
        self.point_cloud = p3.Points(geometry=self.points_geometry, material=self.points_material)
        # return points_geometry, points_material, points

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

    # def get_spatial_extents(self):
    #     """
    #     Find extents of points in 3D
    #     """
    #     xminmax = {}
    #     for i, x in enumerate('xyz'):
    #         xminmax[x] = [
    #             np.amin(self.positions[:, i]) - 0.5 * self.pixel_size,
    #             np.amax(self.positions[:, i]) + 0.5 * self.pixel_size
    #         ]
    #     center_of_mass = [
    #         0.5 * np.sum(xminmax['x']), 0.5 * np.sum(xminmax['y']),
    #         0.5 * np.sum(xminmax['z'])
    #     ]
    #     return xminmax, center_of_mass

    def create_outline(self, axparams):
        """
        Make a wireframe cube with tick labels
        """

        box_geometry = p3.BoxBufferGeometry(
            axparams['x']["lims"][1] - axparams['x']["lims"][0],
            axparams['y']["lims"][1] - axparams['y']["lims"][0],
            axparams['z']["lims"][1] - axparams['z']["lims"][0])
        edges = p3.EdgesGeometry(box_geometry)
        self.outline = p3.LineSegments(
            geometry=edges,
            material=p3.LineBasicMaterial(color='#000000'),
            position=axparams["centre"])
            # position=[
            #     0.5 * np.sum(self.xminmax['x']),
            #     0.5 * np.sum(self.xminmax['y']),
            #     0.5 * np.sum(self.xminmax['z'])
            # ])

        self.axticks = self.generate_axis_ticks_and_labels(axparams)

        # return outline, ticks_and_labels

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

    def generate_axis_ticks_and_labels(self, axparams):
        """
        Create ticklabels on outline edges
        """
        if self.tick_size is None:
            self.tick_size = 0.05 * np.amin([
                axparams['x']["lims"][1] - axparams['x']["lims"][0],
                axparams['y']["lims"][1] - axparams['y']["lims"][0],
                axparams['z']["lims"][1] - axparams['z']["lims"][0]])
                # np.diff(list(self.xminmax.values()), axis=1).ravel())
        ticks_and_labels = p3.Group()
        iden = np.identity(3, dtype=np.float32)
        ticker = mpl.ticker.MaxNLocator(5)
        offsets = {
            'x': [0, axparams['y']["lims"][0], axparams['z']["lims"][0]],
            'y': [axparams['x']["lims"][0], 0, axparams['z']["lims"][0]],
            'z': [axparams['x']["lims"][0], axparams['y']["lims"][0], 0]
        }

        for axis, x in enumerate('xyz'):
            ticks = ticker.tick_values(axparams[x]["lims"][0], axparams[x]["lims"][1])
            for tick in ticks:
                if tick >= axparams[x]["lims"][0] and tick <= axparams[x]["lims"][1]:
                    tick_pos = iden[axis] * tick + offsets[x]
                    ticks_and_labels.add(
                        self.make_axis_tick(string=value_to_string(
                            tick, precision=1),
                                            position=tick_pos.tolist(),
                                            size=self.tick_size))
            ticks_and_labels.add(
                self.make_axis_tick(
                    string=self.axlabels[x],
                    position=(iden[axis] * 0.5 * np.sum(axparams[x]["lims"]) +
                              offsets[x]).tolist(),
                    size=self.tick_size * 0.3 * len(self.axlabels[x])))

        return ticks_and_labels

    def create_colorbar(self):
        """
        Make image from matplotlib colorbar.
        """
        height_inches = config.plot.height / config.plot.dpi

        fig = plt.figure(figsize=(height_inches * 0.2, height_inches),
                                dpi=config.plot.dpi)
        # canvas = backend_agg.FigureCanvasAgg(fig)
        ax = fig.add_axes([0.05, 0.02, 0.25, 0.96])
        cb1 = mpl.colorbar.ColorbarBase(
            ax,
            # cmap=cm.get_cmap(self.engine.params["values"][self.engine.name]["cmap"]),
            cmap=self.scalar_map.get_cmap(),
            norm=self.scalar_map.norm)
        cb1.set_label(name_with_unit(var=self.engine.data_arrays[self.engine.name], name=""))

        buf = io.BytesIO()
        fig.savefig(buf, format='png')
        buf.seek(0)
        self.cbar_image.value = buf.getvalue()





        # fig = mpl.figure.Figure(figsize=(height_inches * 0.2, height_inches),
        #                         dpi=config.plot.dpi)
        # canvas = backend_agg.FigureCanvasAgg(fig)
        # ax = fig.add_axes([0.05, 0.02, 0.25, 0.96])
        # cb1 = mpl.colorbar.ColorbarBase(
        #     ax,
        #     # cmap=cm.get_cmap(self.engine.params["values"][self.engine.name]["cmap"]),
        #     cmap=self.scalar_map.get_cmap(),
        #     norm=self.scalar_map.norm)
        # cb1.set_label(name_with_unit(var=self.engine.data_arrays[self.engine.name], name=""))
        # canvas.draw()
        # image = np.frombuffer(canvas.tostring_rgb(), dtype='uint8')
        # shp = list(fig.canvas.get_width_height())[::-1] + [3]
        # self.cbar_image.value = pil.Image.fromarray(
        #     image.reshape(shp))._repr_png_()




    # def update_opacity(self, change):
    #     """
    #     Update opacity of all points when opacity slider is changed.
    #     Take cut surface into account if present.
    #     """
    #     if self.cut_surface_buttons.value is None:
    #         arr = self.points_geometry.attributes["rgba_color"].array
    #         arr[:, 3] = change["new"][1]
    #         self.points_geometry.attributes["rgba_color"].array = arr
    #         # There is a strange effect with point clouds and opacities.
    #         # Results are best when depthTest is False, at low opacities.
    #         # But when opacities are high, the points appear in the order
    #         # they were drawn, and not in the order they are with respect
    #         # to the camera position. So for high opacities, we switch to
    #         # depthTest = True.
    #         self.points_material.depthTest = change["new"][1] > 0.9
    #     else:
    #         self.update_cut_surface({"new": self.cut_slider.value})

    # def check_if_reset_needed(self, owner, content, buffers):
    #     if owner.value == self.current_cut_surface_value:
    #         self.cut_surface_buttons.value = None
    #     self.current_cut_surface_value = owner.value

    # def update_cut_surface_buttons(self, change):
    #     if change["new"] is None:
    #         self.cut_slider.disabled = True
    #         self.cut_checkbox.disabled = True
    #         self.cut_surface_thickness.disabled = True
    #         self.update_opacity({"new": self.opacity_slider.value})
    #     else:
    #         self.points_material.depthTest = False
    #         if change["old"] is None:
    #             self.cut_slider.disabled = False
    #             self.cut_checkbox.disabled = False
    #             self.cut_surface_thickness.disabled = False
    #         self.update_cut_slider_bounds()

    # def update_cut_slider_bounds(self):
    #     # Cartesian X, Y, Z
    #     if self.cut_surface_buttons.value < self.cut_options["Xcylinder"]:
    #         minmax = self.xminmax["xyz"[self.cut_surface_buttons.value]]
    #         if minmax[0] < self.cut_slider.max:
    #             self.cut_slider.min = minmax[0]
    #             self.cut_slider.max = minmax[1]
    #         else:
    #             self.cut_slider.max = minmax[1]
    #             self.cut_slider.min = minmax[0]
    #         self.cut_slider.value = 0.5 * (minmax[0] + minmax[1])
    #     # Cylindrical X, Y, Z
    #     elif self.cut_surface_buttons.value < self.cut_options["Sphere"]:
    #         j = self.cut_surface_buttons.value - 3
    #         remaining_axes = self.permutations["xyz"[j]]
    #         self.remaining_inds = [(j + 1) % 3, (j + 2) % 3]
    #         rmax = np.abs([
    #             self.xminmax[remaining_axes[0]][0],
    #             self.xminmax[remaining_axes[1]][0],
    #             self.xminmax[remaining_axes[0]][1],
    #             self.xminmax[remaining_axes[1]][1]
    #         ]).max()
    #         self.cut_slider.min = 0
    #         self.cut_slider.max = rmax * np.sqrt(2.0)
    #         self.cut_slider.value = 0.5 * self.cut_slider.max
    #     # Spherical
    #     elif self.cut_surface_buttons.value == self.cut_options["Sphere"]:
    #         rmax = np.abs(list(self.xminmax.values())).max()
    #         self.cut_slider.min = 0
    #         self.cut_slider.max = rmax * np.sqrt(3.0)
    #         self.cut_slider.value = 0.5 * self.cut_slider.max
    #     # Value iso-surface
    #     elif self.cut_surface_buttons.value == self.cut_options["Value"]:
    #         self.cut_slider.min = self.vminmax[0]
    #         self.cut_slider.max = self.vminmax[1]
    #         self.cut_slider.value = 0.5 * (self.vminmax[0] + self.vminmax[1])

    #     # Update slider step to avoid too fine granularity which slows down
    #     # interaction. Slice thickness is linked to the step via jslink.
    #     self.cut_slider.step = (self.cut_slider.max -
    #                             self.cut_slider.min) / self.cut_slider_steps

    # def update_cut_surface(self, change):
    #     newc = None
    #     target = self.cut_slider.value
    #     # Cartesian X, Y, Z
    #     if self.cut_surface_buttons.value < self.cut_options["Xcylinder"]:
    #         newc = np.where(
    #             np.abs(self.positions[:, self.cut_surface_buttons.value] -
    #                    target) < 0.5 * self.cut_surface_thickness.value,
    #             self.opacity_slider.upper, self.opacity_slider.lower)
    #     # Cylindrical X, Y, Z
    #     elif self.cut_surface_buttons.value < self.cut_options["Sphere"]:
    #         newc = np.where(
    #             np.abs(
    #                 np.sqrt(self.positions[:, self.remaining_inds[0]] *
    #                         self.positions[:, self.remaining_inds[0]] +
    #                         self.positions[:, self.remaining_inds[1]] *
    #                         self.positions[:, self.remaining_inds[1]]) -
    #                 target) < 0.5 * self.cut_surface_thickness.value,
    #             self.opacity_slider.upper, self.opacity_slider.lower)
    #     # Spherical
    #     elif self.cut_surface_buttons.value == self.cut_options["Sphere"]:
    #         newc = np.where(
    #             np.abs(
    #                 np.sqrt(self.positions[:, 0] * self.positions[:, 0] +
    #                         self.positions[:, 1] * self.positions[:, 1] +
    #                         self.positions[:, 2] * self.positions[:, 2]) -
    #                 target) < 0.5 * self.cut_surface_thickness.value,
    #             self.opacity_slider.upper, self.opacity_slider.lower)
    #     # Value iso-surface
    #     elif self.cut_surface_buttons.value == self.cut_options["Value"]:
    #         newc = np.where(
    #             np.abs(self.engine.vslice - target) <
    #             0.5 * self.cut_surface_thickness.value,
    #             self.opacity_slider.upper, self.opacity_slider.lower)

    #     # Unfortunately, one cannot edit the value of the geometry array
    #     # in-place, as this does not trigger an update on the threejs side.
    #     # We have to update the entire array.
    #     c3 = self.points_geometry.attributes["rgba_color"].array
    #     c3[:, 3] = newc
    #     self.points_geometry.attributes["rgba_color"].array = c3

    # def slice_data(self, change=None, autoscale_cmap=False):
    #     """
    #     Slice the extra dimensions down and update the slice values
    #     """
    #     self.vslice = self.engine.data_arrays[self.engine.name]
    #     # Slice along dimensions with active sliders
    #     for dim, val in self.slider.items():
    #         if not val.disabled:
    #             # self.lab[dim].value = self.make_slider_label(
    #             #     self.slider_coord[self.engine.name][dim], val.value)
    #             # self.lab[dim].value = self.make_slider_label(
    #             #     self.vslice.coords[dim], val.value, self.slider_axformatter[self.engine.name][dim][False])
    #             # self.vslice = self.vslice[val.dim, val.value]

    #             deltax = self.thickness_slider[dim].value
    #             self.vslice = self.resample_image(self.vslice,
    #                     rebin_edges={dim: sc.Variable([dim], values=[val.value - 0.5 * deltax,
    #                                                                  val.value + 0.5 * deltax],
    #                                                         unit=self.vslice.coords[dim].unit)})[dim, 0]
    #             self.vslice *= (deltax * sc.units.one)


    #     # Handle masks
    #     if len(self.masks[self.engine.name]) > 0:
    #         # Use automatic broadcasting in Scipp variables
    #         msk = sc.Variable(dims=self.vslice.dims,
    #                           values=np.zeros(self.vslice.shape,
    #                                           dtype=np.int32))
    #         for m in self.masks[self.engine.name]:
    #             if self.masks[self.engine.name][m].value:
    #                 msk += sc.Variable(
    #                     dims=self.vslice.masks[m].dims,
    #                     values=self.vslice.masks[m].values.astype(np.int32))
    #         msk = msk.values

    #     self.vslice = self.vslice.values.flatten()
    #     if autoscale_cmap:
    #         self.scalar_map.set_clim(self.vslice.min(), self.vslice.max())
    #     colors = self.scalar_map.to_rgba(self.vslice).astype(np.float32)

    #     if len(self.masks[self.engine.name]) > 0:
    #         masks_inds = np.where(msk.flatten())
    #         masks_colors = self.masks_scalar_map.to_rgba(
    #             self.vslice[masks_inds]).astype(np.float32)
    #         colors[masks_inds] = masks_colors

    #     return colors

    # def update_slice(self, change=None, autoscale_cmap=False):
    #     """
    #     Update colors of points.
    #     """
    #     new_colors = self.slice_data(change=change, autoscale_cmap=autoscale_cmap)
    #     new_colors[:,
    #                3] = self.points_geometry.attributes["rgba_color"].array[:,
    #                                                                         3]
    #     self.points_geometry.attributes["rgba_color"].array = new_colors
    #     if self.cut_surface_buttons.value == self.cut_options["Value"]:
    #         self.update_cut_surface(None)
    #     return

    def toggle_mask(self, change):
        """
        Show/hide masks
        """
        self.engine.update_slice()
        return

    # def toggle_outline(self, change):
    #     self.outline.visible = change["new"]
    #     self.axticks.visible = change["new"]
    #     desc = "Hide" if change["new"] else "Show"
    #     self.toggle_outline_button.description = desc + " outline"

    def rescale_to_data(self, button=None):
        # self.scalar_map.set_clim(self.vslice.min(), self.vslice.max())
        self.engine.update_slice(autoscale_cmap=True)
        self.create_colorbar()
