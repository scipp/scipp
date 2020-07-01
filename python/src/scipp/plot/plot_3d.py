# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import config
from .render import render_plot
from .slicer import Slicer
from .._utils import name_with_unit

# Other imports
import numpy as np
import ipywidgets as widgets
from matplotlib import cm

try:
    import ipyvolume as ipv
    from ipyevents import Event
except ImportError:
    ipv = None


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
            color=None):
    """
    Plot a 3-slice through a N dimensional dataset. For every dimension above
    3, a slider is created to adjust the position of the slice in that
    particular dimension. For other dimensions, the sliders are used to adjust
    the position of the slice in 3D space.
    """

    # Protect against unloaded module
    if ipv is None:
        raise RuntimeError("Three-dimensional projections require ipyvolume "
                           "and ipyevents to be installed. Use conda/pip "
                           "install ipyvolume ipyevents.")

    sv = Slicer3d(scipp_obj_dict=scipp_obj_dict,
                  axes=axes,
                  masks=masks,
                  cmap=cmap,
                  log=log,
                  vmin=vmin,
                  vmax=vmax,
                  color=color,
                  aspect=aspect)

    render_plot(widgets=sv.box, filename=filename, ipv=ipv)

    return sv.members


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
                 aspect=None):

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

        # Initialise Figure and VBox objects
        self.fig = ipv.figure(width=config.plot.width,
                              height=config.plot.height,
                              animation=0)

        self.scalar_map = cm.ScalarMappable(
            norm=self.params["values"][self.name]["norm"],
            cmap=self.params["values"][self.name]["cmap"])

        self.permutations = {"x": ["y", "z"], "y": ["x", "z"], "z": ["x", "y"]}

        # Store min/max for each dimension for invisible scatter
        self.xminmax = dict()
        for dim, var in self.slider_x[self.name].items():
            self.xminmax[dim] = [var.values[0], var.values[-1]]
        self.set_axes_range()

        self.wireframes = dict()
        self.surfaces = dict()

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

        self.mouse_events = dict()
        self.last_changed_slider_dim = None
        for dim, sl in self.slider.items():
            self.mouse_events[dim] = Event(source=sl,
                                           watched_events=['mouseup'])
            self.mouse_events[dim].on_dom_event(self.update_surface)

        # Call update_slice once to make the initial image
        self.update_axes()
        self.box = [ipv.gcc()] + self.vbox
        self.box = widgets.VBox(self.box)
        self.box.layout.align_items = 'center'

        self.members["fig"] = self.fig

        return

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

    # Define function to update wireframes
    def update_slice(self, change):
        if self.buttons[change["owner"].dim].value is None:
            self.update_cube(update_coordinates=False)
        else:
            # Update only one slice
            # The dimensions to be sliced have been saved in slider_dims
            # slice_indices = {"x": 0, "y": 1, "z": 2}
            dim = change["owner"].dim
            self.lab[dim].value = self.make_slider_label(
                self.slider_x[self.name][dim], change["new"])

            # Now move slice
            ax_dim = self.buttons[dim].value.lower()
            self.wireframes[ax_dim].visible = True
            setattr(
                self.wireframes[ax_dim], ax_dim,
                getattr(self.wireframes[ax_dim], ax_dim) * 0.0 +
                self.slider_x[self.name][dim].values[change["new"]])

            self.last_changed_slider_dim = dim
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
