import ipywidgets as ipw
import numpy as np


class PlotWidgets:

    def __init__(self, controller, positions=None,
                         button_options=None):

        # Initialise list for VBox container
        self.controller = controller
        self.rescale_button = ipw.Button(description="Rescale")
        self.rescale_button.on_click(self.controller.rescale_to_data)
        self.container = [self.rescale_button]

        # Initialise slider and label containers
        # self.lab = dict()
        self.slider = dict()
        # self.slider = dict()
        self.thickness_slider = dict()
        self.buttons = dict()
        self.profile_button = dict()
        self.showhide = dict()
        self.button_axis_to_dim = dict()
        self.continuous_update = dict()
        # Default starting index for slider
        indx = 0
        # os.write(1, "Slicer 5\n".encode())

        # # Additional condition if positions kwarg set
        # # positions_dim = None
        # if positions is not None:
        #     if scipp_obj_dict[self.controller.engine.name].coords[
        #             positions].dtype != sc.dtype.vector_3_float64:
        #         raise RuntimeError(
        #             "Supplied positions coordinate does not contain vectors.")
        # if len(button_options) == 3 and positions is not None:
        #     if scipp_obj_dict[self.name].coords[
        #             positions].dtype == sc.dtype.vector_3_float64:
        #         positions_dim = self.data_array.coords[positions].dims[-1]
        #     else:
        #         raise RuntimeError(
        #             "Supplied positions coordinate does not contain vectors.")

        # Now begin loop to construct sliders
        button_values = [None] * (self.controller.ndim - len(button_options)) + \
            button_options[::-1]
        # for i, dim in enumerate(self.slider_coord[self.name]):
        # for i, dim in enumerate(self.data_arrays[self.name].coords):
        # os.write(1, "Slicer 5.1\n".encode())
        for i, dim in enumerate(self.controller.axes):
            # dim_str = self.slider_label[self.name][dim]["name"]
            # dim_str = str(dim)
            # Determine if slider should be disabled or not:
            # In the case of 3d projection, disable sliders that are for
            # dims < 3, or sliders that contain vectors.
            disabled = False
            if positions is not None:
                disabled = dim == positions
            elif i >= self.controller.ndim - len(button_options):
                disabled = True
            # os.write(1, "Slicer 5.2\n".encode())

            # Add an FloatSlider to slide along the z dimension of the array
            dim_xlims = self.controller.xlims[self.controller.name][dim].values
            dx = dim_xlims[1] - dim_xlims[0]
            self.slider[dim] = ipw.FloatSlider(
                value=0.5 * np.sum(dim_xlims),
                min=dim_xlims[0],
                # max=self.slider_shape[self.name][dim][dim] - 1 -
                # self.histograms[name][dim][dim],
                # max=self.dim_to_shape[self.name][dim] - 1,
                max=dim_xlims[1],
                step=0.01 * dx,
                description=self.controller.labels[self.controller.name][dim],
                continuous_update=True,
                readout=True,
                disabled=disabled,
                layout={"width": "350px"},
                style={'description_width': '100px'})
            # labvalue = self.make_slider_label(
            #     self.slider_label[self.name][dim]["coord"], indx)
            self.continuous_update[dim] = ipw.Checkbox(
                value=True,
                description="Continuous update",
                indent=False,
                layout={"width": "20px"},
                disabled=disabled)
            ipw.jslink((self.continuous_update[dim], 'value'),
                           (self.slider[dim], 'continuous_update'))
            # os.write(1, "Slicer 5.3\n".encode())

            self.thickness_slider[dim] = ipw.FloatSlider(
                value=dx,
                min=0.01 * dx,
                max=dx,
                step=0.01 * dx,
                description="Thickness",
                continuous_update=False,
                readout=True,
                disabled=disabled,
                layout={'width': "270px"})
            # os.write(1, "Slicer 5.4\n".encode())

            self.profile_button[dim] = ipw.Button(
                description="Profile",
                disabled=disabled,
                button_style="",
                layout={"width": "initial"})
            # self.profile_button[dim].observe(self.controller.toggle_profile_view, names="value")
            self.profile_button[dim].on_click(self.controller.toggle_profile_view)


            # labvalue = self.make_slider_label(
            #         self.data_arrays[self.name].coords[dim], indx,
            #         self.slider_axformatter[self.name][dim][False])
            # labvalue = "[{}]".format(self.controller.coords[self.controller.name][dim].unit)
            if self.controller.ndim == len(button_options):
                self.slider[dim].layout.display = 'none'
                self.continuous_update[dim].layout.display = 'none'
                self.thickness_slider[dim].layout.display = 'none'
                self.profile_button[dim].layout.display = 'none'
                # This is a trick to turn the label into the coordinate name
                # because when we hide the slider, the slider description is
                # also hidden
                # labvalue = dim_str
            # os.write(1, "Slicer 5.5\n".encode())

            # Add a label widget to display the value of the z coordinate
            # self.lab[dim] = ipw.Label(value=labvalue)
            # Add one set of buttons per dimension
            self.buttons[dim] = ipw.ToggleButtons(
                options=button_options,
                description='',
                value=button_values[i],
                disabled=False,
                button_style='',
                style={"button_width": "50px"})
            # os.write(1, "Slicer 5.6\n".encode())

            # self.profile_button[dim] = ipw.ToggleButton(
            #     value=False,
            #     description="Profile",
            #     disabled=False,
            #     button_style="",
            #     layout={"width": "initial"})
            # self.profile_button[dim].observe(self.controller.engine.toggle_profile_view, names="value")

            # os.write(1, "Slicer 5.7\n".encode())

            if button_values[i] is not None:
                self.button_axis_to_dim[button_values[i].lower()] = dim
            setattr(self.buttons[dim], "dim", dim)
            setattr(self.buttons[dim], "old_value", self.buttons[dim].value)
            setattr(self.slider[dim], "dim", dim)
            setattr(self.continuous_update[dim], "dim", dim)
            setattr(self.thickness_slider[dim], "dim", dim)
            setattr(self.profile_button[dim], "dim", dim)
            # os.write(1, "Slicer 5.8\n".encode())

            # Hide buttons and labels for 1d variables
            if self.controller.ndim == 1:
                self.buttons[dim].layout.display = 'none'
                # self.lab[dim].layout.display = 'none'
                self.thickness_slider[dim].layout.display = 'none'
                self.profile_button[dim].layout.display = 'none'
                self.continuous_update[dim].layout.display = 'none'

            # # Hide buttons and inactive sliders for 3d projection
            if positions is not None and dim == positions:
                self.buttons[dim].layout.display = 'none'
                self.slider[dim].layout.display = 'none'
                self.continuous_update[dim].layout.display = 'none'
                # self.lab[dim].layout.display = 'none'
                self.thickness_slider[dim].layout.display = 'none'
            # # os.write(1, "Slicer 5.9\n".encode())

            # Add observer to buttons
            self.buttons[dim].on_msg(self.update_buttons)
            # Add an observer to the sliders
            self.slider[dim].observe(self.controller.update_data, names="value")
            self.thickness_slider[dim].observe(self.controller.update_data, names="value")
            # Add the row of slider + buttons
            row = [
                self.slider[dim], self.continuous_update[dim],
                self.buttons[dim], self.thickness_slider[dim],
                self.profile_button[dim]
            ]
            self.container.append(ipw.HBox(row))
            # os.write(1, "Slicer 5.10\n".encode())

        #     # Construct members object
        #     self.members["widgets"]["sliders"][dim_str] = self.slider[dim]
        #     self.members["widgets"]["togglebuttons"][dim_str] = self.buttons[
        #         dim]
        #     self.members["widgets"]["labels"][dim_str] = self.lab[dim]
        # os.write(1, "Slicer 6\n".encode())

        # Add controls for masks
        self.add_masks_controls()
        # os.write(1, "Slicer 7\n".encode())

        # self.container = ipw.VBox(self.container)

        return

    def _to_widget(self):
        return ipw.VBox(self.container)

    # def make_data_array_with_bin_edges(self, array):
    #     da_with_edges = sc.DataArray(
    #         data=sc.Variable(dims=array.dims,
    #                          unit=array.unit,
    #                          values=array.values,
    #                          variances=array.variances,
    #                          dtype=sc.dtype.float64))
    #     for dim, coord in arraycoord[self.name].items():
    #         if self.histograms[self.name][dim][dim]:
    #             da_with_edges.coords[dim] = coord
    #         else:
    #             da_with_edges.coords[dim] = to_bin_edges(coord, dim)
    #     if len(self.masks[self.name]) > 0:
    #         for m in self.masks[self.name]:
    #             da_with_edges.masks[m] = self.data_array.masks[m]
    #     return da_with_edges

    def add_masks_controls(self):
        # Add controls for masks
        masks_found = False
        self.mask_checkboxes = {}
        for name in self.controller.mask_names:
            self.mask_checkboxes[name] = {}
            if len(self.controller.mask_names[name]) > 0:
                masks_found = True
                for key in self.controller.mask_names[name]:
                    self.mask_checkboxes[name][key] = ipw.Checkbox(
                        value=self.controller.params["masks"][name]["show"],
                        description="{}:{}".format(name, key),
                        indent=False,
                        layout={"width": "initial"})
                    setattr(self.mask_checkboxes[name][key], "mask_group", name)
                    setattr(self.mask_checkboxes[name][key], "mask_name", key)
                    self.mask_checkboxes[name][key].observe(self.controller.toggle_mask,
                                                  names="value")

        if masks_found:
            self.masks_lab = ipw.Label(value="Masks:")

            # Add a master button to control all masks in one go
            self.masks_button = ipw.ToggleButton(
                value=self.controller.params["masks"][self.controller.name]["show"],
                description="Hide all" if
                self.controller.params["masks"][self.controller.name]["show"] else "Show all",
                disabled=False,
                button_style="",
                layout={"width": "initial"})
            self.masks_button.observe(self.toggle_all_masks, names="value")
            # self.masks_box.append(self.masks_button)

            box_layout = ipw.Layout(display='flex',
                                flex_flow='row wrap',
                                align_items='stretch',
                                width='70%')
            mask_list = []
            for name in self.mask_checkboxes:
                for cbox in self.mask_checkboxes[name].values():
                    mask_list.append(cbox)

            self.masks_box = ipw.Box(children=mask_list, layout=box_layout)

            self.container += [ipw.HBox([self.masks_lab, self.masks_button, self.masks_box])]
            # self.members["widgets"]["togglebutton"]["masks"] = \
            #     self.masks_button

    def update_buttons(self, owner, event, dummy):
        toggle_slider = False
        if not self.slider[owner.dim].disabled:
            toggle_slider = True
            self.slider[owner.dim].disabled = True
            self.thickness_slider[owner.dim].disabled = True
            self.profile_button[owner.dim].disabled = True
            self.continuous_update[owner.dim].disabled = True

        for dim, button in self.buttons.items():
            if (button.value == owner.value) and (dim != owner.dim):
                if self.slider[dim].disabled:
                    button.value = owner.old_value
                else:
                    button.value = None
                button.old_value = button.value
                if toggle_slider:
                    self.slider[dim].disabled = False
                    self.thickness_slider[dim].disabled = False
                    self.profile_button[dim].disabled = False
                    self.continuous_update[dim].disabled = False
        owner.old_value = owner.value
        self.controller.update_axes()
        return

    def toggle_all_masks(self, change):
        for name in self.mask_checkboxes:
            for key in self.mask_checkboxes[name]:
                self.mask_checkboxes[name][key].value = change["new"]
        change["owner"].description = "Hide all" if change["new"] else \
            "Show all"
        return