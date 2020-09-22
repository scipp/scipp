# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

import ipywidgets as ipw
import numpy as np


class PlotWidgets:
    def __init__(self, controller, positions=None, button_options=None):

        self.controller = controller
        self.rescale_button = ipw.Button(description="Rescale")
        self.rescale_button.on_click(self.controller.rescale_to_data)
        # The container list to hold all widgets
        self.container = [self.rescale_button]

        # Initialise slider and label containers
        self.dim_labels = {}
        self.slider = {}
        self.thickness_slider = {}
        self.buttons = {}
        self.profile_button = {}
        self.showhide = {}
        self.button_axis_to_dim = {}
        self.continuous_update = {}

        # Now begin loop to construct sliders
        button_values = [None] * (self.controller.ndim -
                                  len(button_options)) + button_options[::-1]

        for i, dim in enumerate(self.controller.axes):

            # Determine if slider should be disabled or not:
            # In the case of 3d projection, disable sliders that are for
            # dims < 3, or sliders that contain vectors.
            disabled = False
            if positions is not None:
                disabled = dim == positions
            elif i >= self.controller.ndim - len(button_options):
                disabled = True

            self.dim_labels[dim] = ipw.Label(
                value=self.controller.labels[self.controller.name][dim],
                layout={"width": "100px"})

            # Add an FloatSlider to slide along the z dimension of the array
            dim_xlims = self.controller.xlims[self.controller.name][dim].values
            dx = dim_xlims[1] - dim_xlims[0]
            self.slider[dim] = ipw.FloatSlider(value=0.5 * np.sum(dim_xlims),
                                               min=dim_xlims[0],
                                               max=dim_xlims[1],
                                               step=0.01 * dx,
                                               continuous_update=True,
                                               readout=True,
                                               disabled=disabled,
                                               layout={"width": "250px"})

            self.continuous_update[dim] = ipw.Checkbox(
                value=True,
                description="Continuous update",
                indent=False,
                layout={"width": "20px"},
                disabled=disabled)
            ipw.jslink((self.continuous_update[dim], 'value'),
                       (self.slider[dim], 'continuous_update'))

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

            self.profile_button[dim] = ipw.Button(description="Profile",
                                                  disabled=disabled,
                                                  button_style="",
                                                  layout={"width": "initial"})

            self.profile_button[dim].on_click(
                self.controller._toggle_profile_view)

            if self.controller.ndim == len(button_options):
                self.slider[dim].layout.display = 'none'
                self.continuous_update[dim].layout.display = 'none'
                self.thickness_slider[dim].layout.display = 'none'
                self.profile_button[dim].layout.display = 'none'

            # Add one set of buttons per dimension
            self.buttons[dim] = ipw.ToggleButtons(
                options=button_options,
                description='',
                value=button_values[i],
                disabled=False,
                button_style='',
                style={"button_width": "50px"})

            if button_values[i] is not None:
                self.button_axis_to_dim[button_values[i].lower()] = dim

            setattr(self.buttons[dim], "dim", dim)
            setattr(self.buttons[dim], "old_value", self.buttons[dim].value)
            setattr(self.slider[dim], "dim", dim)
            setattr(self.continuous_update[dim], "dim", dim)
            setattr(self.thickness_slider[dim], "dim", dim)
            setattr(self.profile_button[dim], "dim", dim)

            # Hide buttons and labels for 1d variables
            if self.controller.ndim == 1:
                self.buttons[dim].layout.display = 'none'
                self.dim_labels[dim].layout.display = 'none'
                self.thickness_slider[dim].layout.display = 'none'
                self.profile_button[dim].layout.display = 'none'
                self.continuous_update[dim].layout.display = 'none'

            # Hide buttons and inactive sliders for 3d projection
            if positions is not None:
                self.buttons[dim].layout.display = 'none'
                self.profile_button[dim].disabled = True
            if dim == positions:
                self.dim_labels[dim].layout.display = 'none'
                self.slider[dim].layout.display = 'none'
                self.continuous_update[dim].layout.display = 'none'
                self.profile_button[dim].layout.display = 'none'
                self.thickness_slider[dim].layout.display = 'none'

            # Add observer to buttons
            self.buttons[dim].on_msg(self.update_buttons)
            # Add an observer to the sliders
            self.slider[dim].observe(self.controller.update_data,
                                     names="value")
            self.thickness_slider[dim].observe(self.controller.update_data,
                                               names="value")
            # Add the row of slider + buttons
            row = [
                self.dim_labels[dim], self.slider[dim],
                self.continuous_update[dim], self.buttons[dim],
                self.thickness_slider[dim], self.profile_button[dim]
            ]
            self.container.append(ipw.HBox(row))

        # Add controls for masks
        self._add_masks_controls()

        return

    def _to_widget(self):
        return ipw.VBox(self.container)

    def _add_masks_controls(self):
        """
        Add widgets for masks.
        """
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
                    setattr(self.mask_checkboxes[name][key], "mask_group",
                            name)
                    setattr(self.mask_checkboxes[name][key], "mask_name", key)
                    self.mask_checkboxes[name][key].observe(
                        self.controller.toggle_mask, names="value")

        if masks_found:
            self.masks_lab = ipw.Label(value="Masks:")

            # Add a master button to control all masks in one go
            self.masks_button = ipw.ToggleButton(
                value=self.controller.params["masks"][
                    self.controller.name]["show"],
                description="Hide all" if
                self.controller.params["masks"][self.controller.name]["show"]
                else "Show all",
                disabled=False,
                button_style="",
                layout={"width": "initial"})
            self.masks_button.observe(self.toggle_all_masks, names="value")

            box_layout = ipw.Layout(display='flex',
                                    flex_flow='row wrap',
                                    align_items='stretch',
                                    width='70%')
            mask_list = []
            for name in self.mask_checkboxes:
                for cbox in self.mask_checkboxes[name].values():
                    mask_list.append(cbox)

            self.masks_box = ipw.Box(children=mask_list, layout=box_layout)

            self.container += [
                ipw.HBox([self.masks_lab, self.masks_button, self.masks_box])
            ]

    def update_buttons(self, owner, event, dummy):
        """
        Custom update for 2D grid ot toggle buttons.
        """
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
        """
        A main button to hide or show all masks at once.
        """
        for name in self.mask_checkboxes:
            for key in self.mask_checkboxes[name]:
                self.mask_checkboxes[name][key].value = change["new"]
        change["owner"].description = "Hide all" if change["new"] else \
            "Show all"
        return
