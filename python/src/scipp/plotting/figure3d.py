# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet
from .. import config
from .toolbar import PlotToolbar3d
from .tools import fig_to_pngbytes
from ..utils import value_to_string
import numpy as np
import ipywidgets as ipw
from matplotlib import cm, ticker
import matplotlib.pyplot as plt
from matplotlib.colors import Normalize, LogNorm
from matplotlib.colorbar import ColorbarBase
import pythreejs as p3
from copy import copy


class PlotFigure3d:
    """
    Class for 3 dimensional plots.

    It renders an interactive scene containing a point cloud using `pythreejs`.
    """
    def __init__(self, *, background, cmap, extend, figsize, mask_cmap, nan_color, norm,
                 pixel_size, show_outline, tick_size, xlabel, ylabel, zlabel):

        self._pixel_size = pixel_size
        if pixel_size is not None:
            self._pixel_size = pixel_size
        else:
            self._pixel_size = None

        if figsize is None:
            figsize = (config.plot.width, config.plot.height)

        # Figure toolbar
        self.toolbar = PlotToolbar3d(mpl_toolbar=self)

        # Prepare colormaps
        self.cmap = cmap
        self.cmap.set_bad(color=nan_color)
        self.scalar_map = cm.ScalarMappable(norm=norm, cmap=self.cmap)
        self.masks_scalar_map = None
        self.masks_cmap = mask_cmap
        self.masks_cmap.set_bad(color=nan_color)
        self.masks_scalar_map = cm.ScalarMappable(norm=norm, cmap=self.masks_cmap)

        self.axlabels = {"x": xlabel, "y": ylabel, "z": zlabel}
        self.tick_size = tick_size
        self.show_outline = show_outline

        # Create the colorbar image
        self.cbar_image = ipw.Image()
        self.figsize = figsize
        self.extend = extend

        # Create the point cloud material with pythreejs
        self.points_material = None
        self.points_geometry = None
        self.point_cloud = None
        self.outline = None
        self.axticks = None
        self.camera_backup = {}

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

    def initialize_toolbar(self, **kwargs):
        self.toolbar.initialize(**kwargs)

    def _ipython_display_(self):
        """
        IPython display representation for Jupyter notebooks.
        """
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        """
        Return the renderer and the colorbar into a widget box.
        """
        return ipw.HBox(
            [self.toolbar._to_widget(),
             ipw.HBox([self.renderer]), self.cbar_image])

    def savefig(self, filename=None):
        """
        Save scene to file.
        TODO: would be nice to save as static png, as well as interactive html,
        like `ipyvolume`.
        """
        raise RuntimeError("Saving figures is not yet implemented for 3D "
                           "visualization.")

    def initialize(self, *args, **kwargs):
        """
        Dummy initialize function.
        """
        return

    def connect(self, controller, event_handler):
        """
        Connect the toolbar Home button to reset the camera position.
        """
        self.toolbar.connect(controller=controller)

    def update_axes(self, scale, unit):
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

    def set_position_params(self, params):
        limits = params.limits
        center = params.center
        box_size = params.box_size

        if self._pixel_size is None:
            # Note the value of 0.05 is arbitrary here. It is a sensible
            # guess to render a plot that is not too crowded and shows
            # individual pixels.
            self._pixel_size = 0.05 * np.mean(box_size)
        box_size += self._pixel_size

        self._create_outline(limits=limits, box_size=box_size, center=center)

        self.axticks = self._generate_axis_ticks_and_labels(
            box_size=box_size, limits=limits, components=params.components)

        self._create_points_material()
        self._create_point_cloud(positions=params.positions.values)

        # Set camera controller target
        distance_from_center = 1.2
        self.camera.position = list(np.array(center) + distance_from_center * box_size)
        cam_pos_norm = np.linalg.norm(self.camera.position)
        box_mean_size = np.linalg.norm(box_size)
        self.camera.near = 0.01 * box_mean_size
        self.camera.far = 5.0 * cam_pos_norm
        self.controls.target = tuple(center)
        self.camera.lookAt(tuple(center))

        # Save camera settings
        self.camera_backup["reset"] = copy(self.camera.position)
        self.camera_backup["center"] = tuple(copy(center))
        self.camera_backup["x_normal"] = [
            center[0] - distance_from_center * box_mean_size, center[1], center[2]
        ]
        self.camera_backup["y_normal"] = [
            center[0], center[1] - distance_from_center * box_mean_size, center[2]
        ]
        self.camera_backup["z_normal"] = [
            center[0], center[1], center[2] - distance_from_center * box_mean_size
        ]

        # Rescale axes helper
        self.axes_3d.scale = [self.camera.far] * 3

        self.scene.add(self.point_cloud)
        self.scene.add(self.outline)
        self.scene.add(self.axticks)

        # Update visibility of outline according to keyword arg
        self.outline.visible = self.show_outline
        self.axticks.visible = self.show_outline

    def _create_point_cloud(self, positions):
        """
        Make a PointsGeometry using pythreejs
        """
        rgba_shape = list(positions.shape)
        rgba_shape[1] += 1
        pos_array = positions.astype('float32')
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
        Note that the value of 580 was obtained from trial and error.
        """
        if "pixel_ratio" in config.plot:
            pixel_ratio = config.plot.pixel_ratio
        else:
            pixel_ratio = 1.0
        self.points_material = p3.ShaderMaterial(vertexShader='''
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
''' % (580.0 * self._pixel_size * pixel_ratio, ),
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

    def _create_outline(self, *, limits, box_size, center):
        """
        Make a wireframe cube with tick labels
        """
        box_geometry = p3.BoxBufferGeometry(*list(box_size))
        edges = p3.EdgesGeometry(box_geometry)
        self.outline = p3.LineSegments(geometry=edges,
                                       material=p3.LineBasicMaterial(color='#000000'),
                                       position=tuple(center))

    def _make_axis_tick(self, string, position, color="black", size=1.0):
        """
        Make a text-based sprite for axis tick
        """
        sm = p3.SpriteMaterial(map=p3.TextTexture(string=string,
                                                  color=color,
                                                  size=300,
                                                  squareTexture=True),
                               transparent=True)
        return p3.Sprite(material=sm, position=position, scale=[size, size, size])

    def _generate_axis_ticks_and_labels(self, *, limits, box_size, components):
        """
        Create ticklabels on outline edges
        """
        if self.tick_size is None:
            self.tick_size = 0.05 * np.mean(box_size)
        ticks_and_labels = p3.Group()
        iden = np.identity(3, dtype=np.float32)
        ticker_ = ticker.MaxNLocator(5)
        lims = {x: limits[x] for x in "xyz"}
        offsets = {
            'x': [0, limits['y'][0], limits['z'][0]],
            'y': [limits['x'][0], 0, limits['z'][0]],
            'z': [limits['x'][0], limits['y'][0], 0]
        }

        for axis, (x, dim) in enumerate(zip('xyz', components)):
            ticks = ticker_.tick_values(lims[x][0], lims[x][1])
            for tick in ticks:
                if lims[x][0] <= tick <= lims[x][1]:
                    tick_pos = iden[axis] * tick + offsets[x]
                    ticks_and_labels.add(
                        self._make_axis_tick(string=value_to_string(tick, precision=1),
                                             position=tick_pos.tolist(),
                                             size=self.tick_size))
            coord = components[dim]
            axis_label = f'{dim} [{coord.unit}]' if self.axlabels[
                x] is None else self.axlabels[x]
            # Offset labels 5% beyond axis ticks to reduce overlap
            ticks_and_labels.add(
                self._make_axis_tick(string=axis_label,
                                     position=(iden[axis] * 0.5 * np.sum(limits[x]) +
                                               1.05 * np.array(offsets[x])).tolist(),
                                     size=self.tick_size * 0.3 * len(axis_label)))

        return ticks_and_labels

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

    def toggle_mask(self, *args, **kwargs):
        """
        Show/hide masks
        """
        return

    def update_data(self, new_values, info=None):
        """
        Update colors of points.
        """
        array = new_values['data']
        colors = self.scalar_map.to_rgba(array.values)
        self._unit = array.unit

        if 'mask' in new_values:
            # We change the colors of the points in-place where masks are True
            masks_inds = np.where(new_values['mask'].values)
            masks_colors = self.masks_scalar_map.to_rgba(array.values[masks_inds])
            colors[masks_inds] = masks_colors

        colors[:, 3] = self.points_geometry.attributes["rgba_color"].array[:, 3]
        self.points_geometry.attributes["rgba_color"].array = colors.astype(np.float32)

    def rescale_to_data(self, vmin=None, vmax=None):
        """
        Rescale the colorbar limits according to the supplied values.
        Also create a new colorbar image and update the colorbar widget
        container.
        """
        self.scalar_map.set_clim(vmin, vmax)
        self._update_colorbar()

    def _update_colorbar(self):
        """
        Create the colorbar figure and save it to png and update the image
        widget.
        Note that the figure is closed as soon as it is created to avoid it
        re-appearing further down the notebook.
        """
        height_inches = self.figsize[1] / config.plot.dpi
        cbar_fig = plt.figure(figsize=(height_inches * 0.2, height_inches),
                              dpi=config.plot.dpi)
        cbar_ax = cbar_fig.add_axes([0.05, 0.02, 0.25, 0.94])
        _ = ColorbarBase(cbar_ax,
                         cmap=self.scalar_map.get_cmap(),
                         norm=self.scalar_map.norm,
                         extend=self.extend)
        cbar_ax.set_ylabel(self._unit)
        # TODO If we set this position it is clipped somewhere. For now we
        # leave the default, which places unit to the right of the colorbar.
        # cbar_ax.yaxis.set_label_coords(-0.9, 0.5)
        self.cbar_image.value = fig_to_pngbytes(cbar_fig)

    def reset_camera(self):
        """
        Reset the camera position.
        """
        self.move_camera(position=self.camera_backup["reset"])

    def camera_x_normal(self):
        """
        View scene along the X normal.
        """
        self.camera_normal(position=self.camera_backup["x_normal"].copy(), ind=0)

    def camera_y_normal(self):
        """
        View scene along the Y normal.
        """
        self.camera_normal(position=self.camera_backup["y_normal"].copy(), ind=1)

    def camera_z_normal(self):
        """
        View scene along the Z normal.
        """
        self.camera_normal(position=self.camera_backup["z_normal"].copy(), ind=2)

    def camera_normal(self, position, ind):
        """
        Move camera to requested normal, and flip if current position is equal
        to the requested position.
        """
        if np.allclose(self.camera.position, position):
            position[ind] = 2.0 * self.camera_backup["center"][ind] - position[ind]
        self.move_camera(position=position)

    def move_camera(self, position):
        self.camera.position = position
        self.controls.target = self.camera_backup["center"]
        self.camera.lookAt(self.camera_backup["center"])

    def toggle_norm(self, norm=None, vmin=None, vmax=None):
        """
        Toggle color normalization when toolbar button is clicked.
        """
        new_norm = LogNorm(vmin=vmin, vmax=vmax) if norm == "log" else Normalize(
            vmin=vmin, vmax=vmax)
        self.scalar_map.set_norm(new_norm)
        self.masks_scalar_map.set_norm(new_norm)
        self._update_colorbar()
