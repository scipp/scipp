# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from ..config import plot as config
from .render import render_plot
from .slicer import Slicer
from .tools import axis_label, parse_colorbar

# Other imports
import numpy as np
import ipywidgets as widgets
from matplotlib import cm
# from matplotlib.colors import Normalize, LogNorm
import copy as cp

try:
    import ipyvolume as ipv
    from ipyevents import Event
except:
    ipv = None


def plot_3d(input_data=None, axes=None, cb=None, filename=None, name=None,
            figsize=None, show_variances=False):
    """
    Plot a 3-slice through a N dimensional dataset. For every dimension above
    3, a slider is created to adjust the position of the slice in that
    particular dimension. For other dimensions, the sliders are used to adjust
    the position of the slice in 3D space.
    """

    var = input_data[name]
    if axes is None:
        axes = var.dims

    # # Parse colorbar
    # cbar = parse_colorbar(cb, var)

    sv = Slicer3d(input_data=var, axes=axes, cb=cb,
                  show_variances=show_variances)

    render_plot(figure=sv.fig, widgets=sv.box, filename=filename, ipv=ipv)

    return sv.members


class Slicer3d(Slicer):

    def __init__(self, input_data=None, axes=None, cb=None,
                 show_variances=False):

        super().__init__(input_data=input_data, axes=axes, cb=cb,
                         show_variances=show_variances,
                         button_options=['X', 'Y', 'Z'])

        self.cube = None
        self.members.update({"surfaces": {}, "wireframes": {}, "outlines": {},
                             "fig": {}})

        # Initialise Figure and VBox objects
        self.fig = ipv.figure(width=config.width, height=config.height,
                              animation=0)
        # params = {"values": {"cbmin": "min", "cbmax": "max"},
        #           "variances": None}
        # if self.show_variances:
        #     params["variances"] = {"cbmin": "min_var", "cbmax": "max_var"}

        self.scalar_map = dict()

        # Set colorbar limits once to keep them constant for slicer
        # TODO: should there be auto scaling as slider value is changed?
        for key, norm in self.cb["norm"].items():
            # # if val is not None:
            #     arr = getattr(self.input_data, key)
            #     if self.cb["log"]:
            #         subset = np.where(np.isfinite(np.log10(arr)))
            #     else:
            #         subset = np.where(np.isfinite(arr))
            #     if self.cb[val["cbmin"]] is not None:
            #         vmin = self.cb[val["cbmin"]]
            #     else:
            #         vmin = np.amin(arr[subset])
            #     if self.cb[val["cbmax"]] is not None:
            #         vmax = self.cb[val["cbmax"]]
            #     else:
            #         vmax = np.amax(arr[subset])
            #     if self.cb["log"]:
            #         norm = LogNorm(vmin=vmin, vmax=vmax)
            #     else:
            #         norm = Normalize(vmin=vmin, vmax=vmax)


            self.scalar_map[key] = cm.ScalarMappable(norm=norm,
                                                     cmap=self.cb["name"])
            self.members["surfaces"][key] = {}
            self.members["wireframes"][key] = {}


        self.permutations = {"x": ["y", "z"], "y": ["x", "z"], "z": ["x", "y"]}

        # Store min/max for each dimension for invisible scatter
        self.xminmax = dict()
        for key, var in self.slider_x.items():
            self.xminmax[key] = [var.values[0], var.values[-1]]
        outl_x, outl_y, outl_z = self.get_box()

        self.outline = ipv.plot_wireframe(outl_x, outl_y, outl_z,
                                          color="black")
        wframes = self.get_outlines()
        meshes = self.get_meshes()
        self.wireframes = dict()
        self.surfaces = dict()

        # #====================================================================
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

        self.mouse_events = dict()
        self.last_changed_slider_dim = None
        for key, sl in self.slider.items():
            self.mouse_events[key] = Event(source=sl,
                                           watched_events=['mouseup'])
            self.mouse_events[key].on_dom_event(self.update_surface)

        # Call update_slice once to make the initial image
        self.update_axes()
        self.box = [ipv.gcc()] + self.vbox
        self.box = widgets.VBox(self.box)
        self.box.layout.align_items = 'center'

        self.members["outlines"]["values"] = self.outline
        self.members["fig"]["values"] = self.fig

        return

    def update_buttons(self, owner, event, dummy):
        for key, button in self.buttons.items():
            if (button.value == owner.value) and (key != owner.dim_str):
                button.value = owner.old_value
                button.old_value = button.value
        owner.old_value = owner.value
        # Show all surfaces, hide all wireframes
        for key in self.surfaces.keys():
            self.surfaces[key].visible = True
            self.wireframes[key].visible = False
        # Update the show/hide checkboxes
        for key, button in self.buttons.items():
            ax_dim = button.value
            if ax_dim is not None:
                ax_dim = ax_dim.lower()
            self.showhide[key].value = (button.value is not None)
            self.showhide[key].disabled = (button.value is None)
            self.showhide[key].description = "hide"
            if button.value is None:
                self.showhide[key].button_style = ""
            else:
                self.showhide[key].button_style = "success"
                self.button_axis_to_dim[ax_dim] = key

        self.fig.meshes = []
        # Update the outline
        outl_x, outl_y, outl_z = self.get_box()
        outl_x = outl_x.flatten()
        outl_y = outl_y.flatten()
        outl_z = outl_z.flatten()
        self.outline.x = outl_x
        self.outline.y = outl_y
        self.outline.z = outl_z
        self.fig.xlim = list(outl_x[[0, -1]])
        self.fig.ylim = list(outl_y[[0, -1]])
        self.fig.zlim = list(outl_z[[0, -1]])

        self.update_axes()
        # self.box.children = tuple([ipv.gcc()] + self.vbox)

        return

    def update_axes(self):
        # Go through the buttons and select the right coordinates for the axes
        titles = dict()
        buttons_dims = {"x": None, "y": None, "z": None}
        for key, button in self.buttons.items():
            if button.value is not None:
                titles[button.value.lower()] = axis_label(
                    self.slider_x[key], name=self.slider_labels[key])
                buttons_dims[button.value.lower()] = button.dim_str

        self.fig.xlabel = titles["x"]
        self.fig.ylabel = titles["y"]
        self.fig.zlabel = titles["z"]

        self.update_cube()

        return

    def update_cube(self, update_coordinates=True):
        # The dimensions to be sliced have been saved in slider_dims
        self.cube = self.input_data
        self.last_changed_slider_dim = None
        # Slice along dimensions with buttons who have no value, i.e. the
        # dimension is not used for any axis. This reduces the data volume to
        # a 3D cube.
        for key, val in self.slider.items():
            if self.buttons[key].value is None:
                self.lab[key].value = self.make_slider_label(
                    self.slider_x[key], val.value)
                self.cube = self.cube[val.dim, val.value]

        # The dimensions to be sliced have been saved in slider_dims
        button_dim_str = dict()
        button_dim = dict()
        vslices = dict()
        # Slice along dimensions with sliders who have a button value
        for key, val in self.slider.items():
            if self.buttons[key].value is not None:
                s = self.buttons[key].value.lower()
                button_dim_str[s] = key
                button_dim[s] = val.dim
                self.lab[key].value = self.make_slider_label(
                    self.slider_x[key], val.value)
                vslices[s] = {"slice": self.cube[val.dim, val.value],
                              "loc": self.slider_x[key].values[val.value]}

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
                self.members["wireframes"]["values"][key] = \
                    self.wireframes[key]
                self.members["surfaces"]["values"][key] = self.surfaces[key]

            self.surfaces[key].color = self.scalar_map["values"].to_rgba(
                self.check_transpose(val["slice"]).flatten())

        return

    # Define function to update wireframes
    def update_slice(self, change):
        if self.buttons[change["owner"].dim_str].value is None:
            self.update_cube(update_coordinates=False)
        else:
            # Update only one slice
            # The dimensions to be sliced have been saved in slider_dims
            slice_indices = {"x": 0, "y": 1, "z": 2}
            key = change["owner"].dim_str
            self.lab[key].value = self.make_slider_label(
                    self.slider_x[key], change["new"])

            # Now move slice
            ax_dim = self.buttons[key].value.lower()
            self.wireframes[ax_dim].visible = True
            setattr(self.wireframes[ax_dim], ax_dim,
                    getattr(self.wireframes[ax_dim], ax_dim) * 0.0 +
                    self.slider_x[key].values[change["new"]])

            self.last_changed_slider_dim = key
        return

    # Define function to update surfaces
    def update_surface(self, event):
        key = self.last_changed_slider_dim
        if key is not None:
          # Now move slice
          index = self.slider[key].value
          vslice = self.cube[self.slider_dims[key], index]
          ax_dim = self.buttons[key].value.lower()
          self.wireframes[ax_dim].visible = False

          setattr(self.surfaces[ax_dim], ax_dim,
                  getattr(self.surfaces[ax_dim], ax_dim) * 0.0 +
                  self.slider_x[key].values[index])

          self.surfaces[self.buttons[key].value.lower()].color = \
              self.scalar_map["values"].to_rgba(
                  self.check_transpose(vslice).flatten())
        return

    def check_transpose(self, vslice, variances=False):
        # Check if dimensions of arrays agree, if not, plot the transpose
        button_values = [self.buttons[str(dim)].value.lower() for dim in
                         vslice.dims]
        if variances:
            values = vslice.variances
        else:
            values = vslice.values
        if ord(button_values[0]) > ord(button_values[1]):
            values = values.T
        return values

    def update_showhide(self, owner):
        owner.value = not owner.value
        owner.description = "hide" if owner.value else "show"
        owner.button_style = "success" if owner.value else "danger"
        key = owner.dim_str
        ax_dim = self.buttons[key].value.lower()
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
                self.slider_x[self.button_axis_to_dim[val[0]]].values,
                self.slider_x[self.button_axis_to_dim[val[1]]].values,
                indexing="ij")
        return meshes

    def get_box(self):
        max_size = 0.0
        dx = {"x": 0, "y": 0, "z": 0}
        for ax in dx.keys():
            dx[ax] = (self.xminmax[self.button_axis_to_dim[ax]][1] -
                      self.xminmax[self.button_axis_to_dim[ax]][0])
        max_size = np.amax(list(dx.values()))
        arrays = dict()
        for ax, size in dx.items():
            diff = max_size - size
            arrays[ax] = [
                self.xminmax[self.button_axis_to_dim[ax]][0] - 0.5*diff,
                self.xminmax[self.button_axis_to_dim[ax]][1] + 0.5*diff]

        return np.meshgrid(arrays["x"], arrays["y"], arrays["z"],
                           indexing="ij")
