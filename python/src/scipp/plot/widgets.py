import ipywidgets as ipw


class PlotWidgets:

    def __init__(self, parent, engine, positions=None):

        # Initialise list for VBox container
        self.rescale_button = ipw.Button(description="Rescale")
        self.rescale_button.on_click(self.rescale_to_data)
        self.vbox = [self.rescale_button]

        # Initialise slider and label containers
        self.lab = dict()
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
        os.write(1, "Slicer 5\n".encode())

        # Additional condition if positions kwarg set
        # positions_dim = None
        if positions is not None:
            if scipp_obj_dict[self.name].coords[
                    positions].dtype != sc.dtype.vector_3_float64:
                raise RuntimeError(
                    "Supplied positions coordinate does not contain vectors.")
        # if len(button_options) == 3 and positions is not None:
        #     if scipp_obj_dict[self.name].coords[
        #             positions].dtype == sc.dtype.vector_3_float64:
        #         positions_dim = self.data_array.coords[positions].dims[-1]
        #     else:
        #         raise RuntimeError(
        #             "Supplied positions coordinate does not contain vectors.")

        # Now begin loop to construct sliders
        button_values = [None] * (self.ndim - len(button_options)) + \
            button_options[::-1]
        # for i, dim in enumerate(self.slider_coord[self.name]):
        # for i, dim in enumerate(self.data_arrays[self.name].coords):
        os.write(1, "Slicer 5.1\n".encode())
        for i, dim in enumerate(self.parent.engine.axes):
            # dim_str = self.slider_label[self.name][dim]["name"]
            dim_str = str(dim)
            # Determine if slider should be disabled or not:
            # In the case of 3d projection, disable sliders that are for
            # dims < 3, or sliders that contain vectors.
            disabled = False
            if positions is not None:
                disabled = dim == positions
            elif i >= self.ndim - len(button_options):
                disabled = True
            os.write(1, "Slicer 5.2\n".encode())

            # Add an IntSlider to slide along the z dimension of the array
            dim_xlims = self.slider_xlims[self.name][dim].values
            dx = dim_xlims[1] - dim_xlims[0]
            self.slider[dim] = ipw.FloatSlider(
                value=0.5 * np.sum(dim_xlims),
                min=dim_xlims[0],
                # max=self.slider_shape[self.name][dim][dim] - 1 -
                # self.histograms[name][dim][dim],
                # max=self.dim_to_shape[self.name][dim] - 1,
                max=dim_xlims[1],
                step=0.01 * dx,
                description=dim_str,
                continuous_update=True,
                readout=True,
                disabled=disabled)
            # labvalue = self.make_slider_label(
            #     self.slider_label[self.name][dim]["coord"], indx)
            self.continuous_update[dim] = ipw.Checkbox(
                value=True,
                description="Continuous update",
                indent=False,
                layout={"width": "20px"})
            ipw.jslink((self.continuous_update[dim], 'value'),
                           (self.slider[dim], 'continuous_update'))
            os.write(1, "Slicer 5.3\n".encode())

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
            os.write(1, "Slicer 5.4\n".encode())

            # labvalue = self.make_slider_label(
            #         self.data_arrays[self.name].coords[dim], indx,
            #         self.slider_axformatter[self.name][dim][False])
            labvalue = "[{}]".format(self.data_arrays[self.name].coords[dim].unit)
            if self.ndim == len(button_options):
                self.slider[dim].layout.display = 'none'
                self.continuous_update[dim].layout.display = 'none'
                # This is a trick to turn the label into the coordinate name
                # because when we hide the slider, the slider description is
                # also hidden
                labvalue = dim_str
            os.write(1, "Slicer 5.5\n".encode())

            # Add a label widget to display the value of the z coordinate
            self.lab[dim] = ipw.Label(value=labvalue)
            # Add one set of buttons per dimension
            self.buttons[dim] = ipw.ToggleButtons(
                options=button_options,
                description='',
                value=button_values[i],
                disabled=False,
                button_style='',
                style={"button_width": "50px"})
            os.write(1, "Slicer 5.6\n".encode())

            self.profile_button[dim] = ipw.ToggleButton(
                value=False,
                description="Profile",
                disabled=False,
                button_style="",
                layout={"width": "initial"})
            self.profile_button[dim].observe(self.toggle_profile_view, names="value")

            os.write(1, "Slicer 5.7\n".encode())

            if button_values[i] is not None:
                self.button_axis_to_dim[button_values[i].lower()] = dim
            setattr(self.buttons[dim], "dim", dim)
            setattr(self.buttons[dim], "old_value", self.buttons[dim].value)
            setattr(self.slider[dim], "dim", dim)
            setattr(self.continuous_update[dim], "dim", dim)
            setattr(self.thickness_slider[dim], "dim", dim)
            setattr(self.profile_button[dim], "dim", dim)
            os.write(1, "Slicer 5.8\n".encode())

            # Hide buttons and labels for 1d variables
            if self.ndim == 1:
                self.buttons[dim].layout.display = 'none'
                self.lab[dim].layout.display = 'none'

            # Hide buttons and inactive sliders for 3d projection
            if len(button_options) == 3:
                self.buttons[dim].layout.display = 'none'
                if self.slider[dim].disabled:
                    self.slider[dim].layout.display = 'none'
                    self.continuous_update[dim].layout.display = 'none'
                    self.lab[dim].layout.display = 'none'
                    self.thickness_slider[dim].layout.display = 'none'
            os.write(1, "Slicer 5.9\n".encode())

            # Add observer to buttons
            self.buttons[dim].on_msg(self.parent.engine.update_buttons)
            # Add an observer to the sliders
            self.slider[dim].observe(self.parent.engine.update_slice, names="value")
            self.thickness_slider[dim].observe(self.parent.engine.update_slice, names="value")
            # Add the row of slider + buttons
            row = [
                self.slider[dim], self.lab[dim], self.continuous_update[dim],
                self.buttons[dim], self.thickness_slider[dim],
                self.profile_button[dim]
            ]
            self.vbox.append(ipw.HBox(row))
            os.write(1, "Slicer 5.10\n".encode())

        #     # Construct members object
        #     self.members["widgets"]["sliders"][dim_str] = self.slider[dim]
        #     self.members["widgets"]["togglebuttons"][dim_str] = self.buttons[
        #         dim]
        #     self.members["widgets"]["labels"][dim_str] = self.lab[dim]
        # os.write(1, "Slicer 6\n".encode())

        # Add controls for masks
        self.add_masks_controls()
        os.write(1, "Slicer 7\n".encode())

        self.vbox = 

        return

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
        for name, array in self.parent.engine.data_arrays.items():
            self.masks[name] = {}
            if len(array.masks) > 0:
                masks_found = True
                for key in array.masks:
                    self.masks[name][key] = ipw.Checkbox(
                        value=self.params["masks"][name]["show"],
                        description="{}:{}".format(name, key),
                        indent=False,
                        layout={"width": "initial"})
                    setattr(self.masks[name][key], "masks_group", name)
                    setattr(self.masks[name][key], "masks_name", key)
                    self.masks[name][key].observe(self.parent.engine.toggle_mask,
                                                  names="value")

        if masks_found:
            self.masks_box = []
            for name in self.masks:
                mask_list = []
                for cbox in self.masks[name].values():
                    mask_list.append(cbox)
                self.masks_box.append(ipw.HBox(mask_list))
            # Add a master button to control all masks in one go
            self.masks_button = ipw.ToggleButton(
                value=self.params["masks"][self.name]["show"],
                description="Hide all masks" if
                self.params["masks"][self.name]["show"] else "Show all masks",
                disabled=False,
                button_style="")
            self.masks_button.observe(self.parent.engine.toggle_all_masks, names="value")
            self.vbox += [self.masks_button, ipw.VBox(self.masks_box)]
            # self.members["widgets"]["togglebutton"]["masks"] = \
            #     self.masks_button