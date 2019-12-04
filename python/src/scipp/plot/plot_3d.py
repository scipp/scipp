# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from ..config import plot as config
from .plot_2d import Slicer2d
from .render import render_plot
from .slicer import Slicer
from .tools import axis_label, parse_colorbar
from .._scipp import core as sc


# Other imports
import numpy as np
import ipywidgets as widgets
from matplotlib import cm
from matplotlib.colors import Normalize, LogNorm
import copy as cp
# import matplotlib.pyplot as plt


# import plotly.graph_objs as go
# from plotly.subplots import make_subplots


def plot_3d(input_data, axes=None, contours=False, cb=None, filename=None,
            name=None, figsize=None, show_variances=False, ndim=0,
            backend=None, volume=False, volume_sampling=15):
    """
    Plot a 3-slice through a N dimensional dataset. For every dimension above
    3, a slider is created to adjust the position of the slice in that
    particular dimension. For other dimensions, the sliders are used to adjust
    the position of the slice in 3D space.
    """


    var = input_data[name]
    if axes is None:
        axes = var.dims

    # Parse colorbar
    cbar = parse_colorbar(cb, plotly=True)

    # # Make title
    # title = axis_label(var=var, name=name, log=cbar["log"])

    # if figsize is None:
    #     figsize = [config.width, config.height]

    # layout = {"height": figsize[1], "width": figsize[0], "showlegend": False}
    # if var.variances is not None and show_variances:
    #     layout["height"] = 0.7 * layout["height"]

    # if ndim == 2:

    #     data = dict(x=[0], y=[0], z=[0], type="surface",
    #                 colorscale=cbar["name"],
    #                 colorbar=dict(
    #                     title=title,
    #                     titleside='right',
    #                     lenmode='fraction',
    #                     len=1.05,
    #                     thicknessmode='fraction',
    #                     thickness=0.03)
    #                 )

    #     sv = Slicer2d(data=data, layout=layout, input_data=var,
    #                   axes=axes, value_name=title, cb=cbar,
    #                   show_variances=show_variances, rasterize=False,
    #                   surface3d=True)
    # else:
    layout = None
    title=None
    sv = Slicer3d(layout=layout, input_data=var, axes=axes,
                  value_name=title, cb=cbar,
                  show_variances=show_variances, volume=volume,
                  volume_sampling=volume_sampling)

    render_plot(static_fig=sv.fig, interactive_fig=sv.vbox, backend=backend,
                filename=filename)

    return


class Slicer3d(Slicer):

    def __init__(self, layout, input_data, axes, value_name, cb,
                 show_variances, volume, volume_sampling):

        super().__init__(input_data, axes, value_name, cb, show_variances,
                         button_options=['X', 'Y', 'Z'], volume=volume)

        import ipyvolume as ipv
        from ipyevents import Event

        self.cube = None
        self.volume = volume

        # Initialise Figure and VBox objects
        self.fig = ipv.figure()
        self.fig.animation = 0
        params = {"values": {"cbmin": "min", "cbmax": "max"},
                  "variances": None}
        if self.show_variances:
            params["variances"] = {"cbmin": "min_var", "cbmax": "max_var"}

        self.scalar_map = dict()

        # Set colorbar limits once to keep them constant for slicer
        # TODO: should there be auto scaling as slider value is changed?
        for i, (key, val) in enumerate(sorted(params.items())):
            if val is not None:
                # arr = getattr(self.input_data, key)
                # if self.cb[val["cbmin"]] is not None:
                #     val["cmin"] = self.cb[val["cbmin"]]
                # else:
                #     val["cmin"] = np.amin(arr[np.where(np.isfinite(arr))])
                # if self.cb[val["cbmax"]] is not None:
                #     val["cmax"] = self.cb[val["cbmax"]]
                # else:
                #     val["cmax"] = np.amax(arr[np.where(np.isfinite(arr))])
                # if self.cb["log"]:
                #     val["norm"] = LogNorm(vmin=self.cb[val["cbmin"]], vmax=self.cb[val["cbmax"]])
                # else:
                #     val["norm"] = Normalize(vmin=self.cb[val["cbmin"]], vmax=self.cb[val["cbmin"]])
                if self.cb["log"]:
                    norm = LogNorm(vmin=self.cb[val["cbmin"]], vmax=self.cb[val["cbmax"]])
                else:
                    norm = Normalize(vmin=self.cb[val["cbmin"]], vmax=self.cb[val["cbmin"]])


                self.scalar_map[key] = cm.ScalarMappable(norm=norm,
                                                         cmap=self.cb["name"])

        colorbars = [{"x": 1.0, "title": value_name,
                      "thicknessmode": 'fraction', "thickness": 0.02}]
        self.permutations = {"x": ["y", "z"], "y": ["x", "z"], "z": ["x", "y"]}

        # Store min/max for each dimension for invisible scatter
        self.xminmax = dict()
        for key, var in self.slider_x.items():
            self.xminmax[key] = [var.values[0], var.values[-1]]
        outl_x, outl_y, outl_z = self.get_box()
        # outl_x, outl_y, outl_z = np.meshgrid([-10, 60], [-10, 60], [-10, 60])

        self.outline = ipv.plot_wireframe(outl_x, outl_y, outl_z, color="green")
        wframes = self.get_outlines()
        meshes = self.get_meshes()
        self.wireframes = dict()
        self.surfaces = dict()
        # for xyz in "xyz":
        #     print(wframes[xyz][0])
        #     print(wframes[xyz][1])
        # print("surface coordinates")

        surf_args = dict.fromkeys(self.permutations)
        wfrm_args = dict.fromkeys(self.permutations)
        print(wframes)
        for xyz, perm in self.permutations.items():
            print(xyz, perm)
            key = self.button_axis_to_dim[xyz]

            wfrm_args[xyz] = np.ones_like(wframes[xyz][perm[0]]) * self.slider_x[key].values[self.slider[key].value]
            surf_args[xyz] = np.ones_like(meshes[xyz][perm[0]]) * self.slider_x[key].values[self.slider[key].value]
            for p in perm:
                wfrm_args[p] = wframes[xyz][p]
                surf_args[p] = meshes[xyz][p]

            self.wireframes[xyz] = ipv.plot_wireframe(**wfrm_args, color="red")
            self.surfaces[xyz] = ipv.plot_surface(**surf_args, color="red")

            # self.wireframes[xyz] = ipv.plot_wireframe(
            #     np.ones_like(wframes[xyz][perm[0]]) * self.slider_x[key].values[self.slider[key].value],
            #     wframes[xyz][perm[0]], wframes[xyz][perm[1]], color="red")
            # self.surfaces[xyz] = ipv.plot_surface(
            #     np.ones_like(meshes[xyz][perm[0]]) * self.slider_x[key].values[self.slider[key].value],
            #     meshes[xyz][perm[0]], meshes[xyz][perm[1]], color="red")

        # key = self.button_axis_to_dim["x"]
        # self.wireframes["x"] = ipv.plot_wireframe(
        #     np.ones_like(wframes["x"]["y"]) * self.slider_x[key].values[self.slider[key].value],
        #     wframes["x"]["y"], wframes["x"]["z"], color="red")
        # self.surfaces["x"] = ipv.plot_surface(
        #     np.ones_like(meshes["x"]["y"]) * self.slider_x[key].values[self.slider[key].value],
        #     meshes["x"][0], meshes["x"][1], color="red")
        # # print("x:")
        # # print(np.shape(self.surfaces["x"].x), self.surfaces["x"].x)
        # # print(np.shape(self.surfaces["x"].y), self.surfaces["x"].y)
        # # print(np.shape(self.surfaces["x"].z), self.surfaces["x"].z)
        # key = self.button_axis_to_dim["y"]
        # self.wireframes["y"] = ipv.plot_wireframe(wframes["y"][0],
        #     np.ones_like(wframes["y"][0]) * self.slider_x[key].values[self.slider[key].value],
        #     wframes["y"][1], color="red")
        # self.surfaces["y"] = ipv.plot_surface(meshes["y"][0],
        #     np.ones_like(meshes["y"][0]) * self.slider_x[key].values[self.slider[key].value],
        #     meshes["y"][1], color="green")
        # # print("y:")
        # # print(np.shape(self.surfaces["y"].x), self.surfaces["y"].x)
        # # print(np.shape(self.surfaces["y"].y), self.surfaces["y"].y)
        # # print(np.shape(self.surfaces["y"].z), self.surfaces["y"].z)

        # key = self.button_axis_to_dim["z"]
        # self.wireframes["z"] = ipv.plot_wireframe(
        #     wframes["z"][0], wframes["z"][1],
        #     np.ones_like(wframes["z"][0]) * self.slider_x[key].values[self.slider[key].value], color="red")
        # self.surfaces["z"] = ipv.plot_surface(
        #     meshes["z"][0], meshes["z"][1],
        #     np.ones_like(meshes["z"][0]) * self.slider_x[key].values[self.slider[key].value], color="blue")
        # print("z:")
        # print(np.shape(self.surfaces["z"].x), self.surfaces["z"].x)
        # # print(np.shape(self.surfaces["z"].y), self.surfaces["z"].y)
        # # print(np.shape(self.surfaces["z"].z), self.surfaces["z"].z)
        # for k, v in self.wireframes.items():
        #     v.visible = False


        self.mouse_events = dict()
        # self.update_functions = dict()
        self.last_changed_slider_dim = None
        for key, sl in self.slider.items():
            self.mouse_events[key] = Event(source=sl, watched_events=['mouseup'])
            self.mouse_events[key].on_dom_event(self.update_surface)
            # wkey = key + "_wireframe"
            # self.mouse_events[wkey] = Event(source=sl, watched_events=['mousedown'])
            # self.mouse_events[wkey].on_dom_event(self.show_wireframe)
            # Use lambda wrapper to also send the dim to the update function
            # in addition to the mouse event
            # key_copy = cp.copy(key)
            # setattr(self, "update_{}surface".format(key), lambda event: self.update_surface(event, key_copy))
            # self.update_functions[key] = lambda event: self.update_surface(event, key_copy)
            # # self.update_functions[key] = def foo(x): print("The value of x is", x)
            # self.mouse_events[key].on_dom_event(self.update_functions[key])
            # self.mouse_events[key].on_dom_event(self.update_surface)
            # self.mouse_events[key].on_dom_event(getattr(self, "update_{}surface".format(key)))
        # self.container = widgets.HTML('Event info')
        # print(self.update_functions)





        # ipv.show()
        # return




        # # Make a generic volume trace
        # if self.volume:
        #     vol_trace = go.Volume(x=[0], y=[0], z=[0], value=[0], opacity=0.1,
        #                           surface_count=volume_sampling,
        #                           colorscale=self.cb["name"], showscale=True)

        # xyz = "xyz"
        # if self.show_variances:
        #     self.fig = go.FigureWidget(
        #         make_subplots(rows=1, cols=2, horizontal_spacing=0.16,
        #                       specs=[[{"type": "scene"}, {"type": "scene"}]]))

        #     colorbars.append({"x": 1.0, "title": "Variances",
        #                       "thicknessmode": 'fraction', "thickness": 0.02})
        #     colorbars[0]["x"] = -0.1

        #     for i, (key, val) in enumerate(sorted(params.items())):
        #         if self.volume:
        #             vol_trace["isomin"] = val["cmin"]
        #             vol_trace["isomax"] = val["cmax"]
        #             vol_trace["meta"] = key
        #             vol_trace["colorbar"] = colorbars[i]
        #             self.fig.add_trace(vol_trace, row=1, col=i+1)
        #         else:
        #             for j in range(3):
        #                 self.fig.add_trace(
        #                     go.Surface(cmin=val["cmin"],
        #                                cmax=val["cmax"],
        #                                showscale=False,
        #                                colorscale=self.cb["name"],
        #                                colorbar=colorbars[i],
        #                                meta=key,
        #                                name="slice_{}".format(xyz[j])),
        #                     row=1, col=i+1)
        #             self.fig.add_trace(
        #                 go.Scatter3d(x=scatter_x,
        #                              y=scatter_y,
        #                              z=scatter_z,
        #                              marker=dict(cmin=val["cmin"],
        #                                          cmax=val["cmax"],
        #                                          color=np.linspace(
        #                                              val["cmin"],
        #                                              val["cmax"], 8),
        #                                          colorbar=colorbars[i],
        #                                          colorscale=self.cb["name"],
        #                                          showscale=True,
        #                                          opacity=1.0e-6),
        #                              mode="markers",
        #                              hoverinfo="none",
        #                              meta=key,
        #                              name="scatter"),
        #                 row=1, col=i+1)
        #     self.fig.update_layout(**layout)
        # else:
        #     if self.volume:
        #         vol_trace["isomin"] = params["values"]["cmin"]
        #         vol_trace["isomax"] = params["values"]["cmax"]
        #         vol_trace["meta"] = "values"
        #         vol_trace["colorbar"] = colorbars[0]
        #         data = [vol_trace]
        #     else:
        #         data = [go.Surface(cmin=params["values"]["cmin"],
        #                            cmax=params["values"]["cmax"],
        #                            colorscale=self.cb["name"],
        #                            colorbar=colorbars[0],
        #                            showscale=False,
        #                            meta="values",
        #                            name="slice_{}".format(xyz[j]))
        #                 for j in range(3)]

        #         data += [go.Scatter3d(
        #                      x=scatter_x,
        #                      y=scatter_y,
        #                      z=scatter_z,
        #                      marker=dict(cmin=params["values"]["cmin"],
        #                                  cmax=params["values"]["cmax"],
        #                                  color=np.linspace(
        #                                      params["values"]["cmin"],
        #                                      params["values"]["cmax"], 8),
        #                                  colorbar=colorbars[0],
        #                                  colorscale=self.cb["name"],
        #                                  showscale=True,
        #                                  opacity=1.0e-6),
        #                      mode="markers",
        #                      hoverinfo="none",
        #                      meta="values",
        #                      name="scatter")]
        #     self.fig = go.FigureWidget(data=data, layout=layout)

        # Call update_slice once to make the initial image
        self.update_axes()
        self.vbox = [ipv.gcc()] + self.vbox
        self.vbox = widgets.VBox(self.vbox)
        self.vbox.layout.align_items = 'center'

        return

    def update_buttons(self, owner, event, dummy):
        for key, button in self.buttons.items():
            if (button.value == owner.value) and (key != owner.dim_str):
                button.value = owner.old_value
                button.old_value = button.value
        owner.old_value = owner.value
        # if not self.volume:
        # Show all surfaces, hide all wireframes
        for key in self.surfaces.keys():
            self.surfaces[key].visible = True
            self.wireframes[key].visible = True
        # Update the show/hide checkboxes
        for key, button in self.buttons.items():
            ax_dim = button.value
            if ax_dim is not None:
                ax_dim = ax_dim.lower()
            # self.fig.update_traces(visible=True)
            self.showhide[key].value = (button.value is not None)
            self.showhide[key].disabled = (button.value is None)
            self.showhide[key].description = "hide"
            if button.value is None:
                self.showhide[key].button_style = ""
            else:
                self.showhide[key].button_style = "success"
                self.button_axis_to_dim[ax_dim] = key
        # Update the scatter
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

        # self.fig.xlim(outl_x[0], outl_x[-1])
        # self.fig.ylim(outl_y[0], outl_y[-1])
        # self.fig.zlim(outl_z[0], outl_z[-1])
        
        # scatter_x, scatter_y, scatter_z = self.get_outline_as_scatter()
        # self.fig.update_traces(x=scatter_x, y=scatter_y, z=scatter_z,
        #                        selector={"name": "scatter"})
        # self.wireframes["x"].x = outl_x
        self.update_axes()

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

        # axes_dict = dict(xaxis_title=titles["x"],
        #                  yaxis_title=titles["y"],
        #                  zaxis_title=titles["z"])

        # if self.show_variances:
        #     self.fig.layout.scene1 = axes_dict
        #     self.fig.layout.scene2 = axes_dict
        # else:
        #     self.fig.update_layout(scene=axes_dict)
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
        # fig, ax = plt.subplots(3, 1)
        # i = 0
        # Slice along dimensions with sliders who have a button value
        for key, val in self.slider.items():
            if self.buttons[key].value is not None:
                s = self.buttons[key].value.lower()
                button_dim_str[s] = key
                button_dim[s] = val.dim
                if not self.volume:
                    self.lab[key].value = self.make_slider_label(
                        self.slider_x[key], val.value)
                    # print(s, key)
                    vslices[s] = {"slice": self.cube[val.dim, val.value],
                                  "loc": self.slider_x[key].values[val.value]}
                    # print(np.shape(vslices[s]["slice"].values))
                    # ax[i].imshow(vslices[s]["slice"].values)
                    # i += 1

        # if self.volume:

        #     xyz = []
        #     for w in ["x", "y", "z"]:
        #         xyz.append(self.slider_x[button_dim_str[w]].values)
        #     xyz_arrays = np.meshgrid(*xyz, indexing='ij')
        #     base_dims = [button_dim["x"], button_dim["y"], button_dim["z"]]
        #     transpose = [str(dim) for dim in self.cube.dims] != \
        #         [button_dim_str["x"], button_dim_str["y"], button_dim_str["z"]]
        #     if transpose:
        #         # Use scipp's automatic transpose capabilities
        #         base_var = sc.Variable(base_dims,
        #                                values=np.ones_like(xyz_arrays[0]))
        #         self.cube = base_var * self.cube.data
        #     self.fig.update_traces(x=xyz_arrays[0].flatten(),
        #                            y=xyz_arrays[1].flatten(),
        #                            z=xyz_arrays[2].flatten(),
        #                            value=self.cube.values.flatten(),
        #                            selector={"meta": "values"})
        #     if self.show_variances:
        #         self.fig.update_traces(x=xyz_arrays[0].flatten(),
        #                                y=xyz_arrays[1].flatten(),
        #                                z=xyz_arrays[2].flatten(),
        #                                value=self.cube.variances.flatten(),
        #                                selector={"meta": "variances"})

        # else:

        # Now make 3 slices
        # permutations = {"x": ["y", "z"], "y": ["x", "z"], "z": ["x", "y"]}

        wframes = None
        meshes = None
        if update_coordinates:
            wframes = self.get_outlines()
            meshes = self.get_meshes()
        surf_args = dict.fromkeys(self.permutations)
        wfrm_args = dict.fromkeys(self.permutations)

        for key, val in sorted(vslices.items()):
            # print("================")
            # print(key)
            if update_coordinates:
                perm = self.permutations[key]
                # xx, yy = np.meshgrid(
                #     self.slider_x[
                #         button_dim_str[self.permutations[key][0]]].values,
                #     self.slider_x[
                #         button_dim_str[self.permutations[key][1]]].values)
                # args = {key: np.ones_like(xx) * val["loc"],
                #         self.permutations[key][0]: xx,
                #         self.permutations[key][1]: yy}
                surf_args[key] = np.ones_like(meshes[key][perm[0]]) * val["loc"]
                wfrm_args[key] = np.ones_like(wframes[key][perm[0]]) * val["loc"]
                for p in perm:
                    surf_args[p] = meshes[key][p]
                    wfrm_args[p] = wframes[key][p]




# #=============================================
#         surf_args = dict.fromkeys(self.permutations)
#         wfrm_args = dict.fromkeys(self.permutations)
#         print(wframes)
#         for xyz, perm in self.permutations.items():
#             print(xyz, perm)
#             key = self.button_axis_to_dim[xyz]

#             wfrm_args[xyz] = np.ones_like(wframes[xyz][perm[0]]) * self.slider_x[key].values[self.slider[key].value]
#             surf_args[xyz] = np.ones_like(meshes[xyz][perm[0]]) * self.slider_x[key].values[self.slider[key].value]
#             for p in perm:
#                 wfrm_args[p] = wframes[xyz][p]
#                 surf_args[p] = meshes[xyz][p]

#             self.wireframes[xyz] = ipv.plot_wireframe(**wfrm_args, color="red")
#             self.surfaces[xyz] = ipv.plot_surface(**surf_args, color="red")
# #=============================================


                #              self.permutations[key][1]: meshes[key][1]}
                # wfrm_args = {key: np.ones_like(wframes[key][0]) * val["loc"],
                #              self.permutations[key][0]: wframes[key][0],
                #              self.permutations[key][1]: wframes[key][1]}


                # surf_args = {key: np.ones_like(meshes[key][0]) * val["loc"],
                #              self.permutations[key][0]: meshes[key][0],
                #              self.permutations[key][1]: meshes[key][1]}
                # wfrm_args = {key: np.ones_like(wframes[key][0]) * val["loc"],
                #              self.permutations[key][0]: wframes[key][0],
                #              self.permutations[key][1]: wframes[key][1]}

                # setattr(self.surfaces[key], key, (np.ones_like(meshes[self.permutations[key][0]]) * val["loc"]).flatten())
                # setattr(self.surfaces[key], self.permutations[key][0], (np.ones_like(meshes[self.permutations[key][0]]) * val["loc"]).flatten())
                # setattr(self.surfaces[key], key, (np.ones_like(meshes[self.permutations[key][0]]) * val["loc"]).flatten())
                


                print(key, "STARTING HERE +++++++++++++++++++")
                print(surf_args)
                print(wfrm_args)
                for xyz in surf_args.keys():
                    print(xyz, np.shape(surf_args[xyz]), surf_args[xyz])
                    setattr(self.surfaces[key], xyz, surf_args[xyz].flatten())
                    setattr(self.wireframes[key], xyz, wfrm_args[xyz].flatten())
                    # print("end for this key")
                # import ipyvolume as ipv
                # ipv.plot_wireframe(**surf_args, color="blue")



        # self.wireframes["x"] = ipv.plot_wireframe(
        #     np.ones_like(wframes["x"][0]) * self.slider_x[key].values[self.slider[key].value],
        #     wframes["x"][0], wframes["x"][1], color="red")


            #     self.surfaces["x"] = ipv.plot_surface(
            # np.ones_like(meshes["x"][0]) * self.slider_x[key].values[self.slider[key].value],
            # meshes["x"][0], meshes["x"][1], color="red")


            #     # self.fig.update_traces(
            #     #     **args, selector={"name": "slice_{}".format(key)})
            # print(np.shape(self.check_transpose(val["slice"])))
            # colors = self.scalar_map["values"].to_rgba(self.check_transpose(val["slice"]).flatten())
            # print(key)
            # # print(colors)
            # print(np.shape(colors))
            # # if key == "z":
            # #   break

            print("shape of values", np.shape(val["slice"].values), np.shape(self.check_transpose(val["slice"])))
            self.surfaces[key].color = self.scalar_map["values"].to_rgba(self.check_transpose(val["slice"]).flatten())
            # self.surfaces[key].color = "red"
            # self.fig.update_traces(
            #     surfacecolor=self.check_transpose(val["slice"]),
            #     selector={"name": "slice_{}".format(key),
            #               "meta": "values"})
            # if self.show_variances:
            #     self.fig.update_traces(
            #         surfacecolor=self.check_transpose(val["slice"],
            #                                           variances=True),
            #         selector={"name": "slice_{}".format(key),
            #                   "meta": "variances"})

        return

    # Define function to update slices
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
            # vslice = self.cube[change["owner"].dim, change["new"]]

            # Now move slice
            ax_dim = self.buttons[key].value.lower()
            self.wireframes[ax_dim].visible = True
            setattr(self.wireframes[ax_dim], ax_dim,
                    getattr(self.wireframes[ax_dim], ax_dim) * 0.0 +
                    self.slider_x[key].values[change["new"]])

            # # TODO: is using a combination of getattr and setattr slower?
            # # Code would be more compact but i'm not clear if the operation
            # # would still be in-place
            # if ax_dim == "x":
            #     self.wireframes[ax_dim].x[...] = self.slider_x[key].values[change["new"]]
            # elif ax_dim == "y":
            #     self.wireframes[ax_dim].y[...] = self.slider_x[key].values[change["new"]]
            # elif ax_dim == "z":
            #     self.wireframes[ax_dim].z[...] = self.slider_x[key].values[change["new"]]




            #  = ipv.plot_wireframe(
            # np.ones_like(wframes["x"][0]) * self.slider_x[key].values[self.slider[key].value],
            # wframes["x"][0], wframes["x"][1], color="red")
            # xy = {ax_dim: np.ones_like(
            #     self.fig.data[slice_indices[ax_dim]][ax_dim]) *
            #         self.slider_x[key].values[change["new"]]}

            

            # self.fig.update_traces(
            #     **xy, selector={"name": "slice_{}".format(ax_dim)})
            # self.fig.update_traces(surfacecolor=self.check_transpose(vslice),
            #                        selector={"name": "slice_{}".format(ax_dim),
            #                                  "meta": "values"})
            # if self.show_variances:
            #     self.fig.update_traces(
            #         surfacecolor=self.check_transpose(vslice, variances=True),
            #         selector={"name": "slice_{}".format(ax_dim),
            #                   "meta": "variances"})
            # print(self.update_functions)
            self.last_changed_slider_dim = key
        return

    # Define function to update slices
    def update_surface(self, event):
        # print(event)
        key = self.last_changed_slider_dim
        if key is not None:
          # lines = ['{}: {}'.format(k, v) for k, v in event.items()]
          # content = '<br>'.join(lines)
          # content = ""
          # content += "<br>" + key
          # self.container.value  = content

          # Now move slice
          index = self.slider[key].value
          vslice = self.cube[self.slider_dims[key], index]
          ax_dim = self.buttons[key].value.lower()
          self.wireframes[ax_dim].visible = False

          setattr(self.surfaces[ax_dim], ax_dim,
                  getattr(self.surfaces[ax_dim], ax_dim) * 0.0 +
                  self.slider_x[key].values[index])

          # self.container.value  = content

          # colors = self.scalar_map["values"].to_rgba(self.check_transpose(vslice).flatten())
          # content += "<br>" + str(colors)
          # self.container.value  = content
          # print(colors)
          self.surfaces[self.buttons[key].value.lower()].color = self.scalar_map["values"].to_rgba(self.check_transpose(vslice).flatten())
          # self.container.value  = content



        return
        # if self.buttons[change["owner"].dim_str].value is None:
        #     self.update_cube(update_coordinates=False)
        # else:
        #     # Update only one slice
        #     # The dimensions to be sliced have been saved in slider_dims
        #     slice_indices = {"x": 0, "y": 1, "z": 2}
        #     key = change["owner"].dim_str
        #     self.lab[key].value = self.make_slider_label(
        #             self.slider_x[key], change["new"])
        #     vslice = self.cube[change["owner"].dim, change["new"]]



        #     self.surfaces["x"] = ipv.plot_surface(
        #     np.ones_like(meshes["x"][0]) * self.slider_x[key].values[self.slider[key].value],
        #     meshes["x"][0], meshes["x"][1], color="blue")



        #     # Now move slice
        #     ax_dim = self.buttons[key].value.lower()
        #     xy = {ax_dim: np.ones_like(
        #         self.fig.data[slice_indices[ax_dim]][ax_dim]) *
        #             self.slider_x[key].values[change["new"]]}

        #     self.fig.update_traces(
        #         **xy, selector={"name": "slice_{}".format(ax_dim)})
        #     self.fig.update_traces(surfacecolor=self.check_transpose(vslice),
        #                            selector={"name": "slice_{}".format(ax_dim),
        #                                      "meta": "values"})
        #     if self.show_variances:
        #         self.fig.update_traces(
        #             surfacecolor=self.check_transpose(vslice, variances=True),
        #             selector={"name": "slice_{}".format(ax_dim),
        #                       "meta": "variances"})
        # return

    # def show_wireframe(self, event):
    #     # print(event)
    #     key = self.last_changed_slider_dim
    #     if key is not None:
    #       ax_dim = self.buttons[key].value.lower()
    #       self.wireframes[ax_dim].visible = True
    #     return

    def check_transpose(self, vslice, variances=False):
        # Check if dimensions of arrays agree, if not, plot the transpose
        button_values = [self.buttons[str(dim)].value.lower() for dim in
                         vslice.dims]
        if variances:
            values = vslice.variances
        else:
            values = vslice.values
        print(button_values)
        print(ord(button_values[0]), ord(button_values[1]))
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
        # self.fig.update_traces(visible=owner.value,
        #                        selector={"name": "slice_{}".format(ax_dim)})
        return

    def get_outlines(self):
        outlines = dict()
        for key, val in self.permutations.items():
            outlines[key] = dict()
            outlines[key][val[0]], outlines[key][val[1]] = np.meshgrid(
                self.xminmax[self.button_axis_to_dim[val[0]]],
                self.xminmax[self.button_axis_to_dim[val[1]]], indexing="ij")
        # outlines["y"] =  np.meshgrid(
        #     self.xminmax[self.button_axis_to_dim["x"]],
        #     self.xminmax[self.button_axis_to_dim["z"]])
        # outlines["x"] =  np.meshgrid(
        #     self.xminmax[self.button_axis_to_dim["y"]],
        #     self.xminmax[self.button_axis_to_dim["z"]])
        return outlines

    def get_meshes(self):
        meshes = dict()
        for key, val in self.permutations.items():
            meshes[key] = dict()
            meshes[key][val[0]], meshes[key][val[1]] = np.meshgrid(
                self.slider_x[self.button_axis_to_dim[val[0]]].values,
                self.slider_x[self.button_axis_to_dim[val[1]]].values, indexing="ij")
        # meshes["y"] =  np.meshgrid(
        #     self.slider_x[self.button_axis_to_dim["x"]].values,
        #     self.slider_x[self.button_axis_to_dim["z"]].values)
        # meshes["x"] =  np.meshgrid(
        #     self.slider_x[self.button_axis_to_dim["y"]].values,
        #     self.slider_x[self.button_axis_to_dim["z"]].values)
        return meshes


    def get_box(self):
        # xyz = "xyz"
        max_size = 0.0
        dx = {"x": 0, "y": 0, "z": 0}
        for ax in dx.keys():
            dx[ax] = (self.xminmax[self.button_axis_to_dim[ax]][1] -
                      self.xminmax[self.button_axis_to_dim[ax]][0])
            # print(ax, size, 
        max_size = np.amax(list(dx.values()))
        print(max_size)
        arrays = dict()
        for ax, size in dx.items():
            diff = max_size - size
            arrays[ax] = [self.xminmax[self.button_axis_to_dim[ax]][0] - 0.5*diff,
                          self.xminmax[self.button_axis_to_dim[ax]][1] + 0.5*diff]
        print(dx)
        print(arrays)

        return np.meshgrid(arrays["x"], arrays["y"], arrays["z"], indexing="ij")
                # self.xminmax[self.button_axis_to_dim["x"]],
                # self.xminmax[self.button_axis_to_dim["y"]],
                # self.xminmax[self.button_axis_to_dim["z"]])


    # def get_box(self):

    #     f
    #     return np.meshgrid(
    #             self.xminmax[self.button_axis_to_dim["x"]],
    #             self.xminmax[self.button_axis_to_dim["y"]],
    #             self.xminmax[self.button_axis_to_dim["z"]])