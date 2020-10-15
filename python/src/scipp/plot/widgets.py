# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .._utils import value_to_string
import ipywidgets as ipw


class PlotWidgets:
    def __init__(self,
                 axes=None,
                 ndim=None,
                 name=None,
                 dim_to_shape=None,
                 positions=None,
                 masks=None,
                 button_options=None):

        self.rescale_button = ipw.Button(description="Rescale")
        if ndim == len(button_options):
            self.rescale_button.layout.display = 'none'
        self.interface = {}

        # The container list to hold all widgets
        self.container = [self.rescale_button]

        # Initialise slider and label containers
        self.dim_labels = {}
        self.slider = {}
        self.slider_readout = {}
        self.thickness_slider = {}
        self.thickness_readout = {}
        self.buttons = {}
        self.profile_button = {}
        self.showhide = {}
        self.button_axis_to_dim = {}
        self.continuous_update = {}
        self.all_masks_button = None

        # Now begin loop to construct sliders

        for ax, dim in axes.items():

            string_ax = isinstance(ax, str)

            # Determine if slider should be disabled or not:
            # In the case of 3d projection, disable sliders that are for
            # dims < 3, or sliders that contain vectors.
            disabled = False
            if positions is not None:
                disabled = dim == positions
            elif string_ax:
                disabled = True

            self.dim_labels[dim] = ipw.Label(layout={"width": "100px"})

            # Add a slider to slice along additional dimensions of the array
            self.slider[dim] = ipw.IntSlider(min=0,
                                             step=1,
                                             continuous_update=True,
                                             readout=False,
                                             disabled=disabled,
                                             layout={"width": "200px"})

            self.continuous_update[dim] = ipw.Checkbox(
                value=True,
                description="Continuous update",
                indent=False,
                layout={"width": "20px"},
                disabled=disabled)
            ipw.jslink((self.continuous_update[dim], 'value'),
                       (self.slider[dim], 'continuous_update'))

            self.thickness_slider[dim] = ipw.FloatSlider(
                min=0.,
                description="Thickness",
                continuous_update=False,
                readout=False,
                disabled=disabled,
                layout={'width': "180px"})

            self.slider_readout[dim] = ipw.Label()
            self.thickness_readout[dim] = ipw.Label()

            self.profile_button[dim] = ipw.Button(description="Profile",
                                                  disabled=disabled,
                                                  button_style="",
                                                  layout={"width": "initial"})

            if ndim == len(button_options):
                self.slider[dim].layout.display = 'none'
                self.slider_readout[dim].layout.display = 'none'
                self.continuous_update[dim].layout.display = 'none'
                self.thickness_slider[dim].layout.display = 'none'
                self.thickness_readout[dim].layout.display = 'none'
                self.profile_button[dim].layout.display = 'none'

            # Add one set of buttons per dimension
            self.buttons[dim] = ipw.ToggleButtons(
                options=button_options,
                description='',
                value=ax if string_ax else None,
                disabled=False,
                button_style='',
                style={"button_width": "50px"})

            if string_ax:
                self.button_axis_to_dim[ax] = dim

            setattr(self.buttons[dim], "dim", dim)
            setattr(self.buttons[dim], "old_value", self.buttons[dim].value)
            setattr(self.slider[dim], "dim", dim)
            setattr(self.continuous_update[dim], "dim", dim)
            setattr(self.thickness_slider[dim], "dim", dim)
            setattr(self.thickness_readout[dim], "dim", dim)
            setattr(self.profile_button[dim], "dim", dim)

            # Hide buttons and labels for 1d variables
            if ndim == 1:
                self.buttons[dim].layout.display = 'none'
                self.dim_labels[dim].layout.display = 'none'
                self.thickness_slider[dim].layout.display = 'none'
                self.thickness_readout[dim].layout.display = 'none'
                self.profile_button[dim].layout.display = 'none'
                self.continuous_update[dim].layout.display = 'none'

            # Hide buttons if positions are used
            if positions is not None:
                self.buttons[dim].layout.display = 'none'
            # Disable profile picking for 3D plots for now
            if len(button_options) == 3:
                self.profile_button[dim].disabled = True
            if dim == positions:
                self.dim_labels[dim].layout.display = 'none'
                self.slider[dim].layout.display = 'none'
                self.continuous_update[dim].layout.display = 'none'
                self.profile_button[dim].layout.display = 'none'
                self.thickness_slider[dim].layout.display = 'none'
                self.thickness_readout[dim].layout.display = 'none'

            # Add observer to buttons
            self.buttons[dim].on_msg(self.update_buttons)
            # Add the row of slider + buttons
            row = [
                self.dim_labels[dim], self.slider[dim],
                self.slider_readout[dim], self.continuous_update[dim],
                self.buttons[dim], self.thickness_slider[dim],
                self.thickness_readout[dim], self.profile_button[dim]
            ]
            self.container.append(ipw.HBox(row))

        # Add controls for masks
        self._add_masks_controls(masks)

    def _ipython_display_(self):
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        return ipw.VBox(self.container)

    def _add_masks_controls(self, masks):
        """
        Add widgets for masks.
        """
        masks_found = False
        self.mask_checkboxes = {}
        for name in masks:
            self.mask_checkboxes[name] = {}
            if len(masks[name]["names"]) > 0:
                masks_found = True
                for key in masks[name]["names"]:
                    self.mask_checkboxes[name][key] = ipw.Checkbox(
                        value=masks[name]["names"][key],
                        description="{}:{}".format(name, key),
                        indent=False,
                        layout={"width": "initial"})
                    setattr(self.mask_checkboxes[name][key], "mask_group",
                            name)
                    setattr(self.mask_checkboxes[name][key], "mask_name", key)

        if masks_found:
            self.masks_lab = ipw.Label(value="Masks:")

            # Add a master button to control all masks in one go
            self.all_masks_button = ipw.ToggleButton(
                value=True,
                description="Hide all",
                disabled=False,
                button_style="",
                layout={"width": "initial"})
            self.all_masks_button.observe(self.toggle_all_masks, names="value")

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
                ipw.HBox(
                    [self.masks_lab, self.all_masks_button, self.masks_box])
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

        self.interface["update_axes"]()
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

    def connect(self, callbacks):
        self.rescale_button.on_click(callbacks["rescale_to_data"])
        for dim in self.slider:
            self.profile_button[dim].on_click(callbacks["toggle_profile_view"])
            self.slider[dim].observe(callbacks["update_data"], names="value")
            self.thickness_slider[dim].observe(callbacks["update_data"],
                                               names="value")
        self.interface["update_axes"] = callbacks["update_axes"]

        for name in self.mask_checkboxes:
            for m in self.mask_checkboxes[name]:
                self.mask_checkboxes[name][m].observe(callbacks["toggle_mask"],
                                                      names="value")

    def initialise(self, parameters, multid_coord=None):
        for dim, item in parameters.items():
            # Dimension labels
            self.dim_labels[dim].value = item["labels"]

            # Dimension slider
            size = item["slider"]
            self.slider[dim].value = size // 2
            self.slider[dim].max = size - 1

            # Thickness slider
            self.thickness_slider[
                dim].value = 0. if multid_coord is not None else item[
                    "thickness_slider"]
            self.thickness_slider[dim].max = item["thickness_slider"]
            self.thickness_slider[dim].step = item["thickness_slider"] * 0.01
            if multid_coord is not None:
                self.thickness_slider[dim].disabled = True

            # Slider readouts
            self.slider_readout[dim].value = item["slider_readout"]

    def get_active_slider_values(self):
        slider_values = {}
        for dim, sl in self.slider.items():
            if not sl.disabled:
                slider_values[dim] = sl.value
        return slider_values

    def get_non_profile_slider_values(self, profile_dim):
        slider_values = {}
        for dim, sl in self.slider.items():
            if dim != profile_dim:
                slider_values[dim] = sl.value
        return slider_values

    def get_buttons_and_disabled_sliders(self):
        buttons_and_dims = {}
        for dim, button in self.buttons.items():
            if self.slider[dim].disabled:
                buttons_and_dims[dim] = button.value
        return buttons_and_dims

    def get_masks_info(self):
        """
        Get information on masks: their names and whether they should be
        displayed.
        """
        mask_info = {}
        for name in self.mask_checkboxes:
            mask_info[name] = {
                m: chbx.value
                for m, chbx in self.mask_checkboxes[name].items()
            }
        return mask_info

    def update_slider_readout(self, dim, value):
        self.slider_readout[dim].value = value

    def update_thickness_readout(self, dim, loc):
        thickness = self.thickness_slider[dim].value
        # if thickness == 0.0:
        #     thickness_start = value_to_string(coord[dim, ind].value)
        #     thickness_end = value_to_string(coord[dim, ind + 1].value)
        # else:
        thickness_start = value_to_string(loc - 0.5 * thickness)
        thickness_end = value_to_string(loc + 0.5 * thickness)
        # return "{} - {}".format(thickness_start, thickness_end)
        self.thickness_readout[dim].value = "{} - {}".format(
            thickness_start, thickness_end)
