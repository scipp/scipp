# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .. import config
from .toolbar import PlotToolbar
from .._utils import value_to_string
import numpy as np
import ipywidgets as ipw
from matplotlib import cm, ticker
import matplotlib.pyplot as plt
from matplotlib.colors import Normalize, LogNorm
import pythreejs as p3
from copy import copy
import io


class PlotFigure3d:
    """
    Class for 3 dimensional plots.

    It renders an interactive scene containing a point cloud using `pythreejs`.
    """
    def __init__(self,
                 cmap=None,
                 norm=None,
                 figsize=None,
                 unit=None,
                 log=None,
                 nan_color=None,
                 masks=None,
                 pixel_size=None,
                 tick_size=None,
                 background=None,
                 show_outline=True,
                 extend=None,
                 xlabel=None,
                 ylabel=None,
                 zlabel=None):

        if figsize is None:
            figsize = (config.plot.width, config.plot.height)

        # Figure toolbar
        self.toolbar = PlotToolbar(ndim=3)

        # Prepare colormaps
        self.cmap = cmap
        self.cmap.set_bad(color=nan_color)
        self.scalar_map = cm.ScalarMappable(norm=norm, cmap=self.cmap)
        self.masks_scalar_map = None
        if len(masks) > 0:
            self.masks_cmap = masks["cmap"]
            self.masks_cmap.set_bad(color=nan_color)
            self.masks_scalar_map = cm.ScalarMappable(norm=norm,
                                                      cmap=self.masks_cmap)

        self.axlabels = {"x": xlabel, "y": ylabel, "z": zlabel}
        self.positions = None
        self.pixel_size = pixel_size
        self.tick_size = tick_size
        self.show_outline = show_outline
        self.unit = unit

        # Create the colorbar image
        self.cbar_image = ipw.Image()
        self.cbar_fig, self.cbar = self._create_colorbar(figsize, extend)

        # Create the point cloud material with pythreejs
        self.points_material = self._create_points_material()
        self.points_geometry = None
        self.point_cloud = None
        self.outline = None
        self.axticks = None
        self.camera_reset = {}

        # Define camera
        self.camera = p3.PerspectiveCamera(position=[0, 0, 0],
                                           aspect=config.plot.width /
                                           config.plot.height)

        # Add red/green/blue axes helper
        self.axes_3d = p3.AxesHelper()

        # Create the pythreejs scene
        self.scene = p3.Scene(children=[self.camera, self.axes_3d],
                              background=background)

        # Add camera controller
        self.controls = p3.OrbitControls(controlling=self.camera)

        # Render the scene into a widget
        self.renderer = p3.Renderer(camera=self.camera,
                                    scene=self.scene,
                                    controls=[self.controls],
                                    width=figsize[0],
                                    height=figsize[1])

    def _ipython_display_(self):
        """
        IPython display representation for Jupyter notebooks.
        """
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        """
        Return the renderer and the colorbar into a widget box.
        """
        return ipw.HBox([
            self.toolbar._to_widget(),
            ipw.HBox([self.renderer]), self.cbar_image
        ])

    def savefig(self, filename=None):
        """
        Save scene to file.
        TODO: would be nice to save as static png, as well as interactive html,
        like `ipyvolume`.
        """
        raise RuntimeError("Saving figures is not yet implemented for 3D "
                           "visualization.")

    def initialise(self, *args, **kwargs):
        """
        Dummy initialise function.
        """
        return

    def connect(self, callbacks):
        """
        Connect the toolbar Home button to reset the camera position.
        """
        callbacks.update({"home_view": self.reset_camera})
        self.toolbar.connect(callbacks)

    def update_axes(self, axparams):
        """
        When a point cloud is created, one cannot modify the number of points.
        Hence, when axes are updated, we have to remove the point cloud from
        the scene and create a new one.
        """
        if self.point_cloud is not None:
            self.scene.remove(self.point_cloud)
        if self.outline is not None:
            self.scene.remove(self.outline)
        if self.axticks is not None:
            self.scene.remove(self.axticks)

        self._create_point_cloud(axparams["positions"])
        self._create_outline(axparams)

        # Set camera controller target
        self.camera.position = list(
            np.array(axparams["centre"]) + 1.2 * axparams["box_size"])
        self.controls.target = axparams["centre"]
        self.camera.lookAt(axparams["centre"])
        # Save camera settings for reset button
        self.camera_reset["position"] = copy(self.camera.position)
        self.camera_reset["lookat"] = copy(axparams["centre"])

        # Rescale axes helper
        self.axes_3d.scale = [5.0 * np.linalg.norm(self.camera.position)] * 3

        self.scene.add(self.point_cloud)
        self.scene.add(self.outline)
        self.scene.add(self.axticks)

        # Update visibility of outline according to keyword arg
        self.outline.visible = self.show_outline
        self.axticks.visible = self.show_outline

    def _create_point_cloud(self, pos_array):
        """
        Make a PointsGeometry using pythreejs
        """
        rgba_shape = list(pos_array.shape)
        rgba_shape[1] += 1
        self.points_geometry = p3.BufferGeometry(
            attributes={
                'position':
                p3.BufferAttribute(array=pos_array),
                'rgba_color':
                p3.BufferAttribute(array=np.ones(rgba_shape, dtype=np.float32))
            })
        self.point_cloud = p3.Points(geometry=self.points_geometry,
                                     material=self.points_material)

    def _create_points_material(self):
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
''' % (580.0 * self.pixel_size, ),  # the value of 580 is from trial and error
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

    def _create_outline(self, axparams):
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

        self.axticks = self._generate_axis_ticks_and_labels(axparams)

    def _make_axis_tick(self, string, position, color="black", size=1.0):
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

    def _generate_axis_ticks_and_labels(self, axparams):
        """
        Create ticklabels on outline edges
        """
        if self.tick_size is None:
            self.tick_size = 0.05 * np.amin([
                axparams['x']["lims"][1] - axparams['x']["lims"][0],
                axparams['y']["lims"][1] - axparams['y']["lims"][0],
                axparams['z']["lims"][1] - axparams['z']["lims"][0]
            ])
        ticks_and_labels = p3.Group()
        iden = np.identity(3, dtype=np.float32)
        ticker_ = ticker.MaxNLocator(5)
        offsets = {
            'x': [0, axparams['y']["lims"][0], axparams['z']["lims"][0]],
            'y': [axparams['x']["lims"][0], 0, axparams['z']["lims"][0]],
            'z': [axparams['x']["lims"][0], axparams['y']["lims"][0], 0]
        }

        for axis, x in enumerate('xyz'):
            ticks = ticker_.tick_values(axparams[x]["lims"][0],
                                        axparams[x]["lims"][1])
            for tick in ticks:
                if tick >= axparams[x]["lims"][0] and tick <= axparams[x][
                        "lims"][1]:
                    tick_pos = iden[axis] * tick + offsets[x]
                    ticks_and_labels.add(
                        self._make_axis_tick(string=value_to_string(
                            tick, precision=1),
                                             position=tick_pos.tolist(),
                                             size=self.tick_size))
            axis_label = axparams[x][
                "label"] if self.axlabels[x] is None else self.axlabels[x]
            ticks_and_labels.add(
                self._make_axis_tick(
                    string=axis_label,
                    position=(iden[axis] * 0.5 * np.sum(axparams[x]["lims"]) +
                              offsets[x]).tolist(),
                    size=self.tick_size * 0.3 * len(axis_label)))

        return ticks_and_labels

    def _create_colorbar(self, figsize, extend):
        """
        Make image from matplotlib colorbar.
        We need to make a dummy imshow so we can later update the limits with
        set_clim, as this method is not available on a ColorbarBase class.
        """
        height_inches = figsize[1] / config.plot.dpi

        cbar_fig, ax = plt.subplots(figsize=(height_inches * 0.2,
                                             height_inches),
                                    dpi=config.plot.dpi)
        cbar_imshow = ax.imshow(np.array([[0, 1]]),
                                cmap=self.scalar_map.get_cmap(),
                                norm=self.scalar_map.norm)
        ax.set_visible(False)
        cbar_ax = cbar_fig.add_axes([0.05, 0.02, 0.25, 0.94])
        cbar = plt.colorbar(cbar_imshow, cax=cbar_ax, extend=extend)
        cbar.set_label(self.unit)
        cbar.ax.yaxis.set_label_coords(-0.9, 0.5)
        return cbar_fig, cbar_imshow

    def update_opacity(self, alpha):
        """
        Update opacity of all points when opacity slider is changed.
        Take cut surface into account if present.
        """
        color = self.points_geometry.attributes["rgba_color"]
        # Must work with a copy and the array property setter to ensure
        # updates are triggered
        updated = color.array.copy()
        updated[:, 3] = alpha
        color.array = updated

    def update_depth_test(self, value):
        """
        Update the `depthTest` property of the point cloud. If `depthTest` is
        `True`, the distance of the point with respect to the camera is
        conserved. When it is `False`, the points simply appear in the order
        they are drawn.
        """
        self.points_material.depthTest = value

    def toggle_mask(self, change):
        """
        Show/hide masks
        """
        return

    def update_data(self, new_values, info=None):
        """
        Update colors of points.
        """
        colors = self.scalar_map.to_rgba(new_values["values"])

        if new_values["masks"] is not None:
            # In 3D, we change the colors of the points in-place where masks
            # are True, instead of having an additional point cloud per mask.
            masks_inds = np.where(new_values["masks"])
            masks_colors = self.masks_scalar_map.to_rgba(
                new_values["values"][masks_inds])
            colors[masks_inds] = masks_colors

        colors[:, 3] = self.points_geometry.attributes["rgba_color"].array[:,
                                                                           3]
        self.points_geometry.attributes["rgba_color"].array = colors.astype(
            np.float32)

    def rescale_to_data(self, vmin=None, vmax=None):
        """
        Rescale the colorbar limits according to the supplied values.
        Also create a new colorbar image and update the colorbar widget
        container.
        """
        self.scalar_map.set_clim(vmin, vmax)
        self.cbar.set_clim(vmin, vmax)
        self.update_colorbar()

    def update_colorbar(self):
        """
        Save the colorbar figure to png and update the image widget.
        """
        buf = io.BytesIO()
        self.cbar_fig.savefig(buf, format='png', bbox_inches='tight')
        buf.seek(0)
        self.cbar_image.value = buf.getvalue()

    def reset_camera(self, owner=None):
        """
        Reset the camera position.
        """
        self.camera.position = self.camera_reset["position"]
        self.controls.target = self.camera_reset["lookat"]
        self.camera.lookAt(self.camera_reset["lookat"])

    def toggle_norm(self, norm=None, vmin=None, vmax=None):
        """
        Toggle color normalization when toolbar button is clicked.
        """
        new_norm = LogNorm(
            vmin=vmin, vmax=vmax) if norm == "log" else Normalize(vmin=vmin,
                                                                  vmax=vmax)
        self.scalar_map.set_norm(new_norm)
        self.masks_scalar_map.set_norm(new_norm)
        self.cbar.set_norm(new_norm)
        self.update_colorbar()

    def update_log_axes_buttons(self, *args, **kwargs):
        """
        Update the state (value and color) of toolbar log axes buttons when
        axes or dimensions are swapped.
        """
        if self.toolbar is not None:
            self.toolbar.update_log_axes_buttons(*args, **kwargs)

    def update_norm_button(self, *args, **kwargs):
        if self.toolbar is not None:
            self.toolbar.update_norm_button(*args, **kwargs)
