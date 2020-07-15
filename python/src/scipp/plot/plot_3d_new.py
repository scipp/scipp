# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import config
from .render import render_plot
from ..plot.sciplot import SciPlot
from .slicer import Slicer
from .._utils import name_with_unit, value_to_string
from .._scipp import core as sc


# Other imports
import numpy as np
import ipywidgets as widgets
from matplotlib import cm
import matplotlib as mpl
import matplotlib.pyplot as plt
# from matplotlib import ticker
from matplotlib.backends import backend_agg
import PIL as pil
import pythreejs as p3



# try:
#     import ipyvolume as ipv
#     from ipyevents import Event
# except ImportError:
#     ipv = None


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
            background="#f0f0f0"):
    """
    Plot a 3-slice through a N dimensional dataset. For every dimension above
    3, a slider is created to adjust the position of the slice in that
    particular dimension. For other dimensions, the sliders are used to adjust
    the position of the slice in 3D space.
    """

    # Protect against unloaded module
    # if ipv is None:
    #     raise RuntimeError("Three-dimensional projections require ipyvolume "
    #                        "and ipyevents to be installed. Use conda/pip "
    #                        "install ipyvolume ipyevents.")

    sv = Slicer3d(scipp_obj_dict=scipp_obj_dict,
                  axes=axes,
                  masks=masks,
                  cmap=cmap,
                  log=log,
                  vmin=vmin,
                  vmax=vmax,
                  color=color,
                  aspect=aspect,
                  background=background)

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
                 background=None):

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

        self.cube = None
        self.members.update({"surfaces": {}, "wireframes": {}})

        # # Initialise Figure and VBox objects
        # self.fig = ipv.figure(width=config.plot.width,
        #                       height=config.plot.height,
        #                       animation=0)

        self.scalar_map = cm.ScalarMappable(
            norm=self.params["values"][self.name]["norm"],
            cmap=self.params["values"][self.name]["cmap"])

        self.permutations = {"x": ["y", "z"], "y": ["x", "z"], "z": ["x", "y"]}
        self.remaining_axes = ["x", "y"]
        self.remaining_inds = [0, 1]

        # # Store min/max for each dimension for invisible scatter
        # self.xminmax = {}
        # self.center_of_mass = [0, 0, 0]
        # for dim, var in self.slider_x[self.name].items():
        #     self.xminmax[dim] = [var.values[0], var.values[-1]]
        # self.set_axes_range()

        # self.wireframes = dict()
        # self.surfaces = dict()

        # coords = list(self.slider_x[self.name].values())
        self.positions = None
        self.pixel_size = 1.0
        print(self.slider_x)
        for dim, coord in self.data_array.coords.items():
            # if dim 
            print(coord.dtype, sc.dtype.vector_3_float64)
            if coord.dtype == sc.dtype.vector_3_float64:
                # positions_found = dim
                self.positions = np.array(coord.values)
                break
        # if positions_found is not None:
        #     self.positions = np.array(self.slider_x[self.name][positions_found].values)
        # else:
        if self.positions is None:
            coords = []
            for dim, val in self.slider.items():
                if val.disabled:
                    coords.append(self.slider_x[self.name][dim].values)
            x, y, z = np.meshgrid(*coords, indexing='ij')
            self.positions = np.array([x.ravel(), y.ravel(), z.ravel()]).T
            self.pixel_size = coords[0][1] - coords[0][0]
        print('self.pixel_size', self.pixel_size)
        print(coords[0][1] , coords[0][0])

        # #====================================================================
        # wframes = self.get_outlines()
        # meshes = self.get_meshes()
        # surf_args = dict.fromkeys(self.permutations)
        # wfrm_args = dict.fromkeys(self.permutations)
        # # print(wframes)
        # for xyz, perm in self.permutations.items():
        #     print(xyz, perm)
        #     key = self.button_axis_to_dim[xyz]

        #     wfrm_args[xyz] = np.ones_like(wframes[xyz][perm[0]]) * \
        #         self.slider_x[key].values[self.slider[key].value]
        #     surf_args[xyz] = np.ones_like(meshes[xyz][perm[0]]) * \
        #         self.slider_x[key].values[self.slider[key].value]
        #     for p in perm:
        #         wfrm_args[p] = wframes[xyz][p]
        #         surf_args[p] = meshes[xyz][p]

        #     self.wireframes[xyz] = ipv.plot_wireframe(**wfrm_args,
        #                                               color="red")
        #     self.surfaces[xyz] = ipv.plot_surface(**surf_args, color="red")
        # #====================================================================

        # self.mouse_events = dict()
        # self.last_changed_slider_dim = None
        # for dim, sl in self.slider.items():
        #     self.mouse_events[dim] = Event(source=sl,
        #                                    watched_events=['mouseup'])
        #     self.mouse_events[dim].on_dom_event(self.update_surface)

        # # Call update_slice once to make the initial image
        # self.update_axes()
        # self.box = [ipv.gcc()] + self.vbox
        # self.box = widgets.VBox(self.box)
        # self.box.layout.align_items = 'center'

        # self.members["fig"] = self.fig

        self.xminmax, self.center_of_mass = self.get_spatial_extents()

        # self.points_geometry, self.points_material, self.points = 
        self.create_points_geometry()

        self.outline, self.axticks = self.create_outline()

        box_size = np.diff(list(self.xminmax.values()), axis=1).ravel()

        # Define camera
        camera_lookat = self.center_of_mass
        # print(self.center_of_mass)
        # print(np.diff(list(self.xminmax.values()), axis=1))
        camera_pos = np.array(self.center_of_mass) + 1.2 * box_size
        # print(camera_pos)
        # print(list(camera_pos))
        self.camera = p3.PerspectiveCamera(position=list(camera_pos),
                                           lookAt=camera_lookat,
                                                aspect=config.plot.width /
                                                config.plot.height)

        self.axes_3d = p3.AxesHelper(10.0 * np.linalg.norm(camera_pos))
        # Create the threejs scene
        self.scene = p3.Scene(
            children=[self.camera, self.axes_3d, self.points, self.outline, self.axticks],
            background=background)

        # # Call the rendering which will create the mesh and points objects
        # self.change_rendering({"new": self.select_rendering.value})

        # # Add camera controller
        # if self._look_at is not None:
        #     self.controller = self.p3.OrbitControls(controlling=self.camera,
        #                                             target=self._look_at)
        #     self.camera.lookAt(self._look_at)
        # else:
        self.controller = p3.OrbitControls(controlling=self.camera)

        # Render the scene into a widget
        self.renderer = p3.Renderer(camera=self.camera,
                                         scene=self.scene,
                                         controls=[self.controller],
                                         width=config.plot.width,
                                         height=config.plot.height)
                                         # antialias=True)



        # Controls

        self.opacity_slider = widgets.FloatRangeSlider(min=0.0,
                                                       max=1.0,
                                                       value=[0.1, 1],
                                                       step=0.01,
                                                       description="Opacity",
                                                       continuous_update=False)
        self.opacity_slider.observe(self.update_opacity, names="value")
        self.opacity_checkbox = widgets.Checkbox(
            value=self.opacity_slider.continuous_update,
            tooltip="Continuous update",
            indent=False,
            layout={"width": "initial"})
        self.opacity_checkbox_link = widgets.jslink((self.opacity_checkbox, 'value'), (self.opacity_slider, 'continuous_update'))
        # self.opacity_lower_value = widgets.FloatText(value=0, layout={"width": "50px"})
        # self.opacity_upper_value = widgets.FloatText(value=1, layout={"width": "50px"})
        # self.opacity_lower_link = widgets.link((self.opacity_slider, 'lower'), (self.opacity_lower_value, 'value'))
        # self.opacity_upper_link = widgets.link((self.opacity_slider, 'upper'), (self.opacity_upper_value, 'value'))


        self.cut_surface_buttons = widgets.ToggleButtons(
            options=[('X ', 0), ('Y ', 1), ('Z ', 2),
                     (' X ', 3), (' Y ', 4), (' Z ', 5),
                     ('R ', 6)],
            value=None,
            description='Cut surface:',
            button_style='', # 'success', 'info', 'warning', 'danger' or ''
            tooltips=['X-plane', 'Y-plane', 'Z-plane', 'Cylinder-X',
                      'Cylinder-Y', 'Cylinder-Z', 'Sphere'],
            icons=(['cube'] * 3) + (['toggle-on'] * 3) + ['circle-o'],
            style={"button_width": "55px"}
        )
        self.cut_surface_buttons.observe(self.update_cut_surface_buttons, names="value")

        # self.add_cut_planes = {}
        # self.add_cut_planes['x'] = widgets.Button(
        #     description='X',
        #     disabled=False,
        #     button_style='', # 'success', 'info', 'warning', 'danger' or ''
        #     icon='fa-cube',
        #     tooltip="X-plane",
        #     layout={'width': "50px"})
        # self.add_cut_planes['y'] = widgets.Button(
        #     description='Y',
        #     disabled=False,
        #     button_style='', # 'success', 'info', 'warning', 'danger' or ''
        #     icon='fa-cube',
        #     tooltip="Y-plane",
        #     layout={'width': "50px"})
        # self.add_cut_planes['z'] = widgets.Button(
        #     description='Z',
        #     disabled=False,
        #     button_style='', # 'success', 'info', 'warning', 'danger' or ''
        #     icon='fa-cube',
        #     tooltip="Z-plane",
        #     layout={'width': "50px"})
        # self.add_cut_planes['cx'] = widgets.Button(
        #     description='X',
        #     disabled=False,
        #     button_style='', # 'success', 'info', 'warning', 'danger' or ''
        #     icon='fa-toggle-on',
        #     tooltip="Cylinder-X",
        #     layout={'width': "50px"})
        # self.add_cut_planes['cy'] = widgets.Button(
        #     description='Y',
        #     disabled=False,
        #     button_style='', # 'success', 'info', 'warning', 'danger' or ''
        #     icon='fa-toggle-on',
        #     tooltip="Cylinder-Y",
        #     layout={'width': "50px"})
        # self.add_cut_planes['cz'] = widgets.Button(
        #     description='Z',
        #     disabled=False,
        #     button_style='', # 'success', 'info', 'warning', 'danger' or ''
        #     icon='fa-toggle-on',
        #     tooltip="Cylinder-Z",
        #     layout={'width': "50px"})
        # self.add_cut_planes['r'] = widgets.Button(
        #     description='R',
        #     disabled=False,
        #     button_style='', # 'success', 'info', 'warning', 'danger' or ''
        #     icon='fa-circle-o',
        #     tooltip="Sphere",
        #     layout={'width': "50px"})
        self.cut_slider = widgets.FloatSlider(min=0, max=1, disabled=True,
            layout={"width": "200px"})
        self.cut_checkbox = widgets.Checkbox(
            value=True,
            tooltip="Continuous update",
            indent=False,
            layout={"width": "initial"},
            disabled=True)
        self.cut_checkbox_link = widgets.jslink((self.cut_checkbox, 'value'), (self.cut_slider, 'continuous_update'))
        self.cut_slider.observe(self.update_cut_surface,
                                       names="value")

        self.cut_surface_thickness = widgets.FloatText(value=0.05 * box_size.max(),
            layout={"width": "50px"})
        # 
        self.cut_surface_controls = widgets.HBox([self.cut_surface_buttons, self.cut_slider,
            self.cut_checkbox, self.cut_surface_thickness])



        self.box = widgets.VBox([self.renderer, widgets.VBox(self.vbox),
            widgets.HBox([self.opacity_slider, self.opacity_checkbox]),
            self.cut_surface_controls])

        return

    def create_points_geometry(self):
        """
        Make a simple PointsGeometry for the 'Fast' rendering mode
        """
        self.points_geometry = p3.BufferGeometry(
            attributes={
                'position':
                p3.BufferAttribute(array=self.positions),
                'rgba_color':
                p3.BufferAttribute(array=self.slice_data(None))
                # 'color':
                # p3.BufferAttribute(array=self.scalar_map.to_rgba(
                #     self.data_array.values.flatten())[:, :3])
                    # dtype=np.float32))
            })
        # self.points_material = p3.PointsMaterial(vertexColors='VertexColors',
        #                                          size=self.pixel_size,
        #                                          transparent=True)
        self.points_material = self.create_points_material()
        self.points = p3.Points(geometry=self.points_geometry,
                                material=self.points_material)
        # return points_geometry, points_material, points

    def create_points_material(self):
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
//    vColor = vec4(color, 1.0);
    projected = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
    gl_Position = projected;
    // use the delta between the point position and camera position to size point
    xDelta = pow(position[0] - cameraPosition[0], 2.0);
    yDelta = pow(position[1] - cameraPosition[1], 2.0);
    zDelta = pow(position[2] - cameraPosition[2], 2.0);
    delta = pow(xDelta + yDelta + zDelta, 0.5);
    gl_PointSize = 300.0 / delta;
}
''',
    fragmentShader='''
precision highp float;
varying vec4 vColor;
void main() {
    gl_FragColor = vColor;
}
''',
    vertexColors='VertexColors',
    transparent=True,
    depthTest=False,
    alphaTest=0.5,
#     polygonOffset = True,
#     polygonOffsetFactor = -4
)

    def get_spatial_extents(self):
        # Find extents of points in 3D
        xminmax = {}
        for i, x in enumerate('xyz'):
            xminmax[x] = [np.amin(self.positions[:, i]) - 0.5*self.pixel_size, np.amax(self.positions[:, i]) + 0.5*self.pixel_size]
        center_of_mass = [
                0.5*np.sum(xminmax['x']),
                0.5*np.sum(xminmax['y']),
                0.5*np.sum(xminmax['z'])]
        return xminmax, center_of_mass


    def create_outline(self):
        """
        Make a wireframe cube with tick labels
        """

        # Find extents of points in 3D
        for i, x in enumerate('xyz'):
            self.xminmax[x] = [np.amin(self.positions[:, i]) - 0.5*self.pixel_size, np.amax(self.positions[:, i]) + 0.5*self.pixel_size]

        box_geometry = p3.BoxBufferGeometry(
            self.xminmax['x'][1] - self.xminmax['x'][0],
            self.xminmax['y'][1] - self.xminmax['y'][0],
            self.xminmax['z'][1] - self.xminmax['z'][0])
        edges = p3.EdgesGeometry( box_geometry )
        outline = p3.LineSegments(geometry=edges, material=p3.LineBasicMaterial(color='#000000'),
            position=[
                0.5*np.sum(self.xminmax['x']),
                0.5*np.sum(self.xminmax['y']),
                0.5*np.sum(self.xminmax['z'])])

        axticks = self.generate_axis_ticks()

        return outline, axticks


    def make_axis_tick(self, string, position, color="black", size=1.0):
        sm = p3.SpriteMaterial(map=p3.TextTexture(
            string=string, color=color, size=300, squareTexture=True),
                                    transparent=True)
        return p3.Sprite(material=sm,
                              position=position,
                              scaleToTexture=True,
                              scale=[size, size, size])


    def generate_axis_ticks(self):
                            # group=None,
                            # xmin=0,
                            # xmax=1,
                            # axis=0,
                            # size=1,
                            # offset=None,
                            # nmax=20,
                            # range_start=0):

        max_extent = np.amax(np.diff(list(self.xminmax.values()), axis=1).ravel())
        print(max_extent)
        # return
        tick_size = 0.1 * max_extent
        axticks = p3.Group()
        iden = np.identity(3, dtype=np.float32)
        ticker = mpl.ticker.MaxNLocator(5)
        # axticks.add(self.make_axis_tick("0", [0, 0, 0], size=tick_size))
        offsets = {'x': [0, self.xminmax['y'][0], self.xminmax['z'][0]],
                   'y': [self.xminmax['x'][0], 0, self.xminmax['z'][0]],
                   'z': [self.xminmax['x'][0], self.xminmax['y'][0], 0]}

        for axis, x in enumerate('xyz'):
            # ticker = self.mpl_ticker.MaxNLocator(10)
            ticks = ticker.tick_values(self.xminmax[x][0], self.xminmax[x][1])
            # iden = np.identity(3, dtype=np.float32)
            for i in range(2, len(ticks)):
            # for tick in ticks:
                tick_pos = iden[axis] * ticks[i] + offsets[x]#s[i]
                # if offset is not None:
                #     tick_pos += offset
                axticks.add(
                    self.make_axis_tick(string=value_to_string(ticks[i], precision=1),
                                        position=tick_pos.tolist(),
                                        size=tick_size))
        # Add bottom corner
        tick_pos = iden[0] * ticks[1] + offsets['x']
        axticks.add(
            self.make_axis_tick(string=value_to_string(ticks[1], precision=1),
                                position=tick_pos.tolist(),
                                size=tick_size))

        return axticks

    # def update_opacity(self, change):
    #     self.points_material.opacity = change["new"][1]
    def update_opacity(self, change):
        # self.points_material.opacity = change["new"][1]
        if self.cut_surface_buttons.value is None:
            arr = self.points_geometry.attributes["rgba_color"].array
            arr[:, 3] = change["new"][1]
            self.points_geometry.attributes["rgba_color"].array = arr
        else:
            self.update_cut_surface({"new": self.cut_slider.value})

    # def generate_3d_axes_ticks(self):
    #     tick_size = 10.0 * self._pixel_size
    #     axticks = self.p3.Group()
    #     axticks.add(self.make_axis_tick("0", [0, 0, 0], size=tick_size))
    #     for i in range(3):
    #         self.generate_axis_ticks(group=axticks,
    #                                  xmin=self.camera_pos * 0.1,
    #                                  xmax=self.camera_pos * 10.0,
    #                                  axis=i,
    #                                  size=tick_size,
    #                                  range_start=1)

    # def toggle_cut_continuous_update(self, change):
    #     self.cut_slider.continuous_update = change["new"]
    #     return

    def update_cut_surface_buttons(self, change):
        if change["new"] is None:
            self.cut_slider.disabled = True
            self.cut_checkbox.disabled = True
        else:
            if change["old"] is None:
                self.cut_slider.disabled = False
                self.cut_checkbox.disabled = False
            self.update_cut_slider_bounds()
            # self.update_cut_surface({"new"})

    def update_cut_slider_bounds(self):
        if self.cut_surface_buttons.value < 3:
            minmax = self.xminmax["xyz"[self.cut_surface_buttons.value]]
            self.cut_slider.min = minmax[0]
            self.cut_slider.max = minmax[1]
            self.cut_slider.value = 0.5 * (minmax[0] + minmax[1])
        elif self.cut_surface_buttons.value < 6:
            j = self.cut_surface_buttons.value - 3
            remaining_axes = self.permutations["xyz"[j]]
            # self.remaining_axes.remove(self.cut_surface_buttons.value - 3)
            self.remaining_inds = [(j + 1) % 2, (j + 2) % 2]
            # self.remaining_inds.remove(self.cut_surface_buttons.value - 3)
            # minmax = self.xminmax["xyz"[self.cut_surface_buttons.value]]
            # minxy = np.amin([self.xminmax[remains[0]][0], self.xminmax[remains[1]][0]])
            # maxxy = np.amax([self.xminmax[remains[0]][1], self.xminmax[remains[1]][1]])
            rmax = np.abs([self.xminmax[remaining_axes[0]][0],
                           self.xminmax[remaining_axes[1]][0],
                           self.xminmax[remaining_axes[0]][1],
                           self.xminmax[remaining_axes[1]][1]]).max()
            self.cut_slider.min = 0
            self.cut_slider.max = rmax * np.sqrt(2.0)
            self.cut_slider.value = 0.5 * self.cut_slider.max



    def update_cut_surface(self, change):
        newc = None
        if self.cut_surface_buttons.value < 3:
            newc = np.where(np.abs(
                self.positions[:, self.cut_surface_buttons.value] - change["new"]) < self.cut_surface_thickness.value, self.opacity_slider.upper, self.opacity_slider.lower)
        elif self.cut_surface_buttons.value < 6:
            newc = np.where(np.abs(np.sqrt(
                self.positions[:, self.remaining_inds[0]] * self.positions[:, self.remaining_inds[0]] +
                self.positions[:, self.remaining_inds[1]] * self.positions[:, self.remaining_inds[1]]) - change["new"]) < self.cut_surface_thickness.value, self.opacity_slider.upper, self.opacity_slider.lower)
        c3 = self.points_geometry.attributes["rgba_color"].array
        # print(np.shape(c3))
        # print(np.shape(newc))
        c3[:, 3] = newc
        self.points_geometry.attributes["rgba_color"].array = c3



    def update_buttons(self, owner, event, dummy):
        for dim, button in self.buttons.items():
            if (button.value == owner.value) and (dim != owner.dim):
                button.value = owner.old_value
                button.old_value = button.value
        owner.old_value = owner.value
        # Show all surfaces, hide all wireframes
        for key in self.surfaces.keys():
            self.surfaces[key].visible = True
            self.wireframes[key].visible = False
        # Update the show/hide checkboxes
        for dim, button in self.buttons.items():
            ax_dim = button.value
            if ax_dim is not None:
                ax_dim = ax_dim.lower()
            self.showhide[dim].value = (button.value is not None)
            self.showhide[dim].disabled = (button.value is None)
            self.showhide[dim].description = "hide"
            if button.value is None:
                self.showhide[dim].button_style = ""
            else:
                self.showhide[dim].button_style = "success"
                self.button_axis_to_dim[ax_dim] = dim

        self.fig.meshes = []
        self.set_axes_range()
        self.update_axes()
        # self.box.children = tuple([ipv.gcc()] + self.vbox)

        return

    def update_axes(self):
        # Go through the buttons and select the right coordinates for the axes
        titles = dict()
        buttons_dims = {"x": None, "y": None, "z": None}
        for dim, button in self.buttons.items():
            if button.value is not None:
                titles[button.value.lower()] = name_with_unit(
                    self.slider_x[self.name][dim], name=str(dim))
                # buttons_dims[button.value.lower()] = button.dim
                buttons_dims[button.value.lower()] = dim

        self.fig.xlabel = titles["x"]
        self.fig.ylabel = titles["y"]
        self.fig.zlabel = titles["z"]

        self.update_cube()

        return

    def update_cube(self, update_coordinates=True):
        # The dimensions to be sliced have been saved in slider_dims
        self.cube = self.data_array
        self.last_changed_slider_dim = None
        # Slice along dimensions with buttons who have no value, i.e. the
        # dimension is not used for any axis. This reduces the data volume to
        # a 3D cube.
        for dim, val in self.slider.items():
            if self.buttons[dim].value is None:
                self.lab[dim].value = self.make_slider_label(
                    self.slider_x[self.name][dim], val.value)
                self.cube = self.cube[dim, val.value]

        # The dimensions to be sliced have been saved in slider_dims
        button_dim = dict()
        vslices = dict()
        # Slice along dimensions with sliders who have a button value
        for dim, val in self.slider.items():
            if self.buttons[dim].value is not None:
                s = self.buttons[dim].value.lower()
                button_dim[s] = dim
                self.lab[dim].value = self.make_slider_label(
                    self.slider_x[self.name][dim], val.value)
                vslices[s] = {
                    "slice": self.cube[dim, val.value],
                    "loc": self.slider_x[self.name][dim].values[val.value]
                }

        # Now make 3 slices
        wframes = None
        meshes = None
        if update_coordinates:
            wframes = self.get_outlines()
            meshes = self.get_meshes()
        surf_args = dict.fromkeys(self.permutations)
        wfrm_args = dict.fromkeys(self.permutations)

        for key, val in sorted(vslices.items()):
            if update_coordinates:
                perm = self.permutations[key]
                surf_args[key] = np.ones_like(meshes[key][perm[0]]) * \
                    val["loc"]
                wfrm_args[key] = np.ones_like(wframes[key][perm[0]]) * \
                    val["loc"]
                for p in perm:
                    surf_args[p] = meshes[key][p]
                    wfrm_args[p] = wframes[key][p]

                self.wireframes[key] = ipv.plot_wireframe(**wfrm_args,
                                                          color="red")
                self.wireframes[key].visible = False
                self.surfaces[key] = ipv.plot_surface(**surf_args)
                self.members["wireframes"][key] = \
                    self.wireframes[key]
                self.members["surfaces"][key] = self.surfaces[key]

            self.surfaces[key].color = self.scalar_map.to_rgba(
                self.check_transpose(val["slice"]).flatten())

        return

    def slice_data(self, change):

        # if self.buttons[change["owner"].dim].value is None:
        #     self.update_cube(update_coordinates=False)
        # else:
        #     # Update only one slice
        #     # The dimensions to be sliced have been saved in slider_dims
        #     # slice_indices = {"x": 0, "y": 1, "z": 2}
        #     dim = change["owner"].dim
        #     self.lab[dim].value = self.make_slider_label(
        #         self.slider_x[self.name][dim], change["new"])

        #     # Now move slice
        #     ax_dim = self.buttons[dim].value.lower()
        #     self.wireframes[ax_dim].visible = True
        #     setattr(
        #         self.wireframes[ax_dim], ax_dim,
        #         getattr(self.wireframes[ax_dim], ax_dim) * 0.0 +
        #         self.slider_x[self.name][dim].values[change["new"]])

        #     self.last_changed_slider_dim = dim

        vslice = self.data_array
        # if self.params["masks"][self.name]["show"]:
        #     mslice = self.masks
        # Slice along dimensions with active sliders
        button_dims = [None, None, None]
        for dim, val in self.slider.items():
            if not val.disabled:
                self.lab[dim].value = self.make_slider_label(
                    self.slider_x[self.name][dim], val.value)
                vslice = vslice[val.dim, val.value]
                # # At this point, after masks were combined, all their
                # # dimensions should be contained in the data_array.dims.
                # if self.params["masks"][
                #         self.name]["show"] and dim in mslice.dims:
                #     mslice = mslice[val.dim, val.value]
            else:
                # Get the dimensions of the dimension-coordinates, since
                # buttons can contain non-dimension coordinates
                button_dims[self.buttons[dim].value.lower() ==
                            "x"] = self.slider_x[self.name][val.dim].dims[0]

        # arr = self.hist_data_array[self.key][self.slider_dim,
        #                                      change["new"]].values
        # if self.select_rendering.value == "Full":
        #     arr = np.repeat(arr, self.nverts, axis=0)
        # colors = self.scalar_map[self.key].to_rgba(arr).astype(np.float32)
        return self.scalar_map.to_rgba(vslice.values.flatten()).astype(np.float32)
        # if self.key in self.masks_variables and self.masks_params[
        #         self.key]["show"]:
        #     msk = self.masks_variables[self.key].values
        #     if self.select_rendering.value == "Full":
        #         msk = np.repeat(msk, self.nverts, axis=0)
        #     masks_inds = np.where(msk)
        #     masks_colors = self.masks_scalar_map.to_rgba(
        #         arr[masks_inds]).astype(np.float32)
        #     colors[masks_inds] = masks_colors

    # Define function to update wireframes
    def update_slice(self, change):


        new_colors = self.slice_data(change)
        new_colors[:, 3] = self.points_geometry.attributes["rgba_color"].array[:, 3]
        self.points_geometry.attributes["rgba_color"].array = new_colors

        # self.label.value = name_with_unit(
        #     var=self.hist_data_array[self.key].coords[self.slider_dim],
        #     name=value_to_string(self.hist_data_array[self.key].coords[
        #         self.slider_dim].values[change["new"]]))

        return

    # Define function to update surfaces
    def update_surface(self, event):
        dim = self.last_changed_slider_dim
        if dim is not None:
            # Now move slice
            index = self.slider[dim].value
            vslice = self.cube[dim, index]
            ax_dim = self.buttons[dim].value.lower()
            self.wireframes[ax_dim].visible = False

            setattr(
                self.surfaces[ax_dim], ax_dim,
                getattr(self.surfaces[ax_dim], ax_dim) * 0.0 +
                self.slider_x[self.name][dim].values[index])

            self.surfaces[self.buttons[dim].value.lower()].color = \
                self.scalar_map.to_rgba(
                    self.check_transpose(vslice).flatten())
        return

    def check_transpose(self, vslice):
        # Check if dimensions of arrays agree, if not, plot the transpose
        button_values = [
            self.buttons[dim].value.lower() for dim in vslice.dims
        ]
        values = vslice.values
        if ord(button_values[0]) > ord(button_values[1]):
            values = values.T
        return values

    def update_showhide(self, owner):
        owner.value = not owner.value
        owner.description = "hide" if owner.value else "show"
        owner.button_style = "success" if owner.value else "danger"
        ax_dim = self.buttons[owner.dim].value.lower()
        self.surfaces[ax_dim].visible = owner.value
        return

    def get_outlines(self):
        outlines = dict()
        for key, val in self.permutations.items():
            outlines[key] = dict()
            outlines[key][val[0]], outlines[key][val[1]] = np.meshgrid(
                self.xminmax[self.button_axis_to_dim[val[0]]],
                self.xminmax[self.button_axis_to_dim[val[1]]],
                indexing="ij")
        return outlines

    def get_meshes(self):
        meshes = dict()
        for key, val in self.permutations.items():
            meshes[key] = dict()
            meshes[key][val[0]], meshes[key][val[1]] = np.meshgrid(
                self.slider_x[self.name][self.button_axis_to_dim[
                    val[0]]].values,
                self.slider_x[self.name][self.button_axis_to_dim[
                    val[1]]].values,
                indexing="ij")
        return meshes

    def set_axes_range(self):
        if self.aspect == "equal":
            max_size = 0.0
            dx = {"x": 0, "y": 0, "z": 0}
            for ax in dx.keys():
                dx[ax] = np.ediff1d(self.xminmax[self.button_axis_to_dim[ax]])
            max_size = np.amax(list(dx.values()))
            arrays = dict()
            for ax, size in dx.items():
                diff = max_size - size
                arrays[ax] = [
                    self.xminmax[self.button_axis_to_dim[ax]][0] - 0.5 * diff,
                    self.xminmax[self.button_axis_to_dim[ax]][1] + 0.5 * diff
                ]

            outl_x, outl_y, outl_z = np.meshgrid(arrays["x"],
                                                 arrays["y"],
                                                 arrays["z"],
                                                 indexing="ij")
        elif self.aspect == "auto":
            outl_x, outl_y, outl_z = np.meshgrid(
                self.xminmax[self.button_axis_to_dim["x"]],
                self.xminmax[self.button_axis_to_dim["y"]],
                self.xminmax[self.button_axis_to_dim["z"]],
                indexing="ij")
        else:
            raise RuntimeError("Unknown aspect ratio: {}".format(self.aspect))

        self.fig.xlim = list(outl_x.flatten()[[0, -1]])
        self.fig.ylim = list(outl_y.flatten()[[0, -1]])
        self.fig.zlim = list(outl_z.flatten()[[0, -1]])
        return
