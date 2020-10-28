# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .._utils import value_to_string
import ipywidgets as ipw


class PlotWidgets:
    """
    Widgets containing a slider for each of the input's dimensions, as well as
    buttons to modify the currently displayed axes.
    It also provides buttons to hide/show masks.
    """
    def __init__(self,
                 axes=None,
                 ndim=None,
                 name=None,
                 dim_to_shape=None,
                 positions=None,
                 masks=None,
                 button_options=None):

        # self.rescale_button = ipw.Button(description="Rescale")
        # if ndim == len(button_options):
        #     self.rescale_button.layout.display = 'none'
        self.interface = {}

        # The container list to hold all widgets
        # self.container = [self.rescale_button]
        self.container = []

        # Initialise slider and label containers
        self.unit_labels = {}
        self.slider = {}
        self.index_to_dim = {}
        self.slider_readout = {}
        self.thickness_slider = {}
        self.dim_buttons = {}
        self.profile_button = {}
        self.showhide = {}
        self.button_axis_to_dim = {}
        self.continuous_update = {}
        self.all_masks_button = None

        slider_dims = {}
        for ax, dim in axes.items():
            if isinstance(ax, int) and dim != positions:
                slider_dims[ax] = dim
        possible_dims = list(axes.values())

        # Now begin loop to construct sliders
        # for ax, dim in axes.items():
        for index, (ax, dim) in enumerate(slider_dims.items()):

            # # Determine if slider should be disabled or not:
            # # In the case of 3d projection, disable sliders that are for
            # # dims < 3, or sliders that contain vectors.
            # disabled = False
            # if positions is not None:
            #     disabled = dim == positions
            # elif string_ax:
            #     disabled = True

            self.unit_labels[index] = ipw.Label(layout={"width": "40px"})

            # Add a slider to slice along additional dimensions of the array
            self.slider[index] = ipw.IntSlider(step=1,
                                             continuous_update=True,
                                             readout=False,
                                             layout={"width": "200px"})

            self.continuous_update[index] = ipw.Checkbox(
                value=True,
                description="Continuous update",
                indent=False,
                layout={"width": "20px"})
            ipw.jslink((self.continuous_update[index], 'value'),
                       (self.slider[index], 'continuous_update'))

            self.thickness_slider[index] = ipw.IntSlider(
                min=1,
                step=1,
                description="Thickness",
                continuous_update=False,
                readout=False,
                layout={'width': "180px"},
                style={'description_width': 'initial'})

            self.slider_readout[index] = ipw.Label()

            self.profile_button[index] = ipw.Button(description="Profile",
                                                  button_style="",
                                                  layout={"width": "initial"})

            # if ndim == len(button_options):
            #     self.slider[dim].layout.display = 'none'
            #     self.slider_readout[dim].layout.display = 'none'
            #     self.continuous_update[dim].layout.display = 'none'
            #     self.thickness_slider[dim].layout.display = 'none'
            #     self.profile_button[dim].layout.display = 'none'

            # Add one set of buttons per dimension
            self.dim_buttons[index] = {}
            for dim_ in possible_dims:
                # dstr = str(dim_)
                self.dim_buttons[index][dim_] = ipw.Button(
                description=dim_,
                # value=dim == dim_,
                button_style='info' if dim == dim_ else '',
                # style={"button_width": "initial"}
                disabled=((dim != dim_) and (dim_ in slider_dims.values())),
                layout={"width":'initial'})
                # Add observer to buttons
                # self.dim_buttons[index][dim_].on_msg(self.update_buttons)
                self.dim_buttons[index][dim_].on_click(self.update_buttons)
                setattr(self.dim_buttons[index][dim_], "index", index)

            self.index_to_dim[index] = dim
            self.index_to_dim[dim] = index

            # if string_ax:
            #     self.button_axis_to_dim[ax] = dim

            # setattr(self.dim_buttons[index], "index", index)
            # setattr(self.dim_buttons[index], "old_value", self.dim_buttons[index].value)
            setattr(self.slider[index], "dim", dim)
            setattr(self.slider[index], "index", index)
            # setattr(self.continuous_update[index], "index", index)
            setattr(self.thickness_slider[index], "dim", dim)
            setattr(self.thickness_slider[index], "index", index)
            # setattr(self.profile_button[index], "index", index)

            # # Hide buttons and labels for 1d variables
            # if ndim == 1:
            #     self.buttons[dim].layout.display = 'none'
            #     self.dim_labels[dim].layout.display = 'none'
            #     self.thickness_slider[dim].layout.display = 'none'
            #     self.profile_button[dim].layout.display = 'none'
            #     self.continuous_update[dim].layout.display = 'none'

            # # Hide buttons if positions are used
            # if positions is not None:
            #     self.buttons[dim].layout.display = 'none'
            # # Hide profile picking for 3D plots for now
            # if len(button_options) == 3:
            #     self.profile_button[dim].layout.display = 'none'
            # if dim == positions:
            #     self.dim_labels[dim].layout.display = 'none'
            #     self.slider[dim].layout.display = 'none'
            #     self.continuous_update[dim].layout.display = 'none'
            #     self.profile_button[dim].layout.display = 'none'
            #     self.thickness_slider[dim].layout.display = 'none'

            # Add observer to buttons
            # self.dim_buttons[index].on_msg(self.update_buttons)
            # Add the row of slider + buttons
            # row = [
            #     self.dim_labels[dim], self.slider[dim],
            #     self.slider_readout[dim], self.continuous_update[dim],
            #     self.buttons[dim], self.thickness_slider[dim],
            #     self.profile_button[dim]
            # ]
            row = list(self.dim_buttons[index].values()) + [
                self.continuous_update[index],
                self.slider[index],
                self.slider_readout[index],
                self.unit_labels[index],
                self.thickness_slider[index],
                self.profile_button[index]
            ]
            self.container.append(ipw.HBox(row))

        # Add controls for masks
        self._add_masks_controls(masks)

    def _ipython_display_(self):
        """
        IPython display representation for Jupyter notebooks.
        """
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        """
        Gather all widgets in a single container box.
        """
        return ipw.VBox(self.container)

    def _add_masks_controls(self, masks):
        """
        Add checkboxes for individual masks, as well as a global hide/show all
        toggle button.
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

    # def update_buttons(self, owner=None, event=None, dummy=None):
    def update_buttons(self, owner=None):
        """
        Custom update for 2D grid of toggle buttons.
        """
        if owner.button_style == "info":
            return
        # toggle_slider = False
        # if not self.slider[owner.dim].disabled:
        #     toggle_slider = True
        #     self.slider[owner.dim].disabled = True
        #     self.thickness_slider[owner.dim].disabled = True
        #     self.profile_button[owner.dim].disabled = True
        #     self.continuous_update[owner.dim].disabled = True
        new_ind = owner.index
        new_dim = owner.description

        self.index_to_dim[new_ind] = new_dim
        self.index_to_dim[new_dim] = new_ind

        old_dim = None
        for dim in self.dim_buttons[new_ind]:
            if self.dim_buttons[new_ind][dim].button_style == "info":
                old_dim = self.dim_buttons[new_ind][dim].description
            # if self.dim_buttons[owner.index][dim].description != owner.description:
            self.dim_buttons[new_ind][dim].button_style = ""
        owner.button_style = "info"

        self.slider[new_ind].dim = new_dim
        self.thickness_slider[new_ind].dim = new_dim

        # Update the slider max and value.
        self.update_thickness_slider_range({new_ind: new_dim})
        # # Before we update the value, we need
        # # to lock the data update which is linked to the slider.
        # self.interface["lock_update_data"]()
        # slider_max = self.interface["get_dim_shape"](new_dim) - 1
        # if slider_max < self.slider[new_ind].value:
        #     self.slider[new_ind].value = slider_max // 2
        #     self.slider[new_ind].max = slider_max
        # else:
        #     self.slider[new_ind].max = slider_max
        #     self.slider[new_ind].value = slider_max // 2
        # self.interface["unlock_update_data"]()
        # for dim in self.dim_buttons[owner.index]:
        #     if self.dim_buttons[owner.index][dim].description != owner.description:
        #         self.dim_buttons[owner.index][dim].button_style = ""
        for index in set(self.dim_buttons.keys()) - set([new_ind]):
            self.dim_buttons[index][new_dim].disabled = True
            self.dim_buttons[index][old_dim].disabled = False

        # Update the axes


        # for index in self.dim_buttons:
        #     for dim in self.dim_buttons[index]:

        #     if (button.value == owner.value) and (dim != owner.dim):
        #         if self.slider[dim].disabled:
        #             button.value = owner.old_value
        #         else:
        #             button.value = None
        #         button.old_value = button.value
        #         if toggle_slider:
        #             self.slider[dim].disabled = False
        #             self.thickness_slider[dim].disabled = False
        #             self.profile_button[dim].disabled = False
        #             self.continuous_update[dim].disabled = False
        # owner.old_value = owner.value
        self.unit_labels[new_ind].value = self.interface["get_coord_unit"](new_dim)

        self.interface["swap_dimensions"](new_ind, old_dim, new_dim)
        # self.interface["update_axes"]()
        return

    def update_slider_range(self, index, thickness, nmax, set_value=True):
        sl_min = (thickness // 2) + (thickness % 2) - 1
        sl_max = nmax - (thickness // 2)
        # print(sl_min, sl_mid, sl_max)
        # print(self.slider[index].min, self.slider[index].value, self.slider[index].max)
        if sl_max < self.slider[index].min:
            self.slider[index].min = sl_min
            self.slider[index].max = sl_max
        else:
            self.slider[index].max = sl_max
            self.slider[index].min = sl_min
        # self.slider[index].min = sl_min
        # self.slider[index].max = sl_max
        if set_value:
            # sl_mid = (sl_min + sl_max) // 2
            self.slider[index].value = (sl_min + sl_max) // 2

    def update_thickness(self, change=None):
        index = change["owner"].index
        self.update_slider_range(index, change["new"], change["owner"].max - 1,
            set_value=False)
        self.interface["update_data"](change)


    def update_thickness_slider_range(self, inds_and_dims):
        # Update the slider max and value. Before we update the value, we need
        # to lock the data update which is linked to the slider.
        self.interface["lock_update_data"]()
        for ind, dim in inds_and_dims.items():
            slider_max = self.interface["get_dim_shape"](dim)
            self.thickness_slider[ind].max = slider_max
            self.thickness_slider[ind].value = slider_max

            # sl_min = (slider_max // 2) + (slider_max % 2) - 1
            # sl_max = 8 - (j // 2)

            # if slider_max < self.slider[ind].value:
            #     self.slider[ind].value = slider_max // 2
            #     self.slider[ind].max = slider_max
            # else:
            #     self.slider[ind].max = slider_max
            #     self.slider[ind].value = slider_max // 2
            self.update_slider_range(ind, slider_max, slider_max - 1)

            # # Thickness slider
            # self.thickness_slider[dim].max = item["thickness_slider"]
            # self.thickness_slider[dim].value = item["thickness_slider"]
            # # Only index slicing is allowed when ragged coords are present
            # if multid_coord is not None:
            #     self.thickness_slider[dim].value = 0.
            #     self.thickness_slider[dim].layout.display = 'none'
            # self.thickness_slider[dim].step = item["thickness_slider"] * 0.01
        self.interface["unlock_update_data"]()

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
        """
        Connect the widget interface to the callbacks provided by the
        `PlotController`.
        """
        # self.rescale_button.on_click(callbacks["rescale_to_data"])
        for index in self.slider:
            self.profile_button[index].on_click(callbacks["toggle_profile_view"])
            self.slider[index].observe(callbacks["update_data"], names="value")
            # self.thickness_slider[dim].observe(callbacks["update_data"],
            #                                    names="value")
            self.thickness_slider[index].observe(self.update_thickness,
                                               names="value")
        # self.interface["update_axes"] = callbacks["update_axes"]
        self.interface["update_data"] = callbacks["update_data"]
        self.interface["get_dim_shape"] = callbacks["get_dim_shape"]
        self.interface["lock_update_data"] = callbacks["lock_update_data"]
        self.interface["unlock_update_data"] = callbacks["unlock_update_data"]
        self.interface["swap_dimensions"] = callbacks["swap_dimensions"]
        self.interface["get_coord_unit"] = callbacks["get_coord_unit"]

        for name in self.mask_checkboxes:
            for m in self.mask_checkboxes[name]:
                self.mask_checkboxes[name][m].observe(callbacks["toggle_mask"],
                                                      names="value")

    # def initialise(self, parameters, multid_coord=None):
    def initialise(self, dim_to_shape, xlims, coord_units):
        """
        Initialise widget parameters once the `PlotModel`, `PlotView` and
        `PlotController` have been created, since, for instance, slider limits
        depend on the dimensions of the input data, which are not known until
        the `PlotModel` is created.
        """
        for index in self.thickness_slider:
            dim = self.index_to_dim[index]
            nmax = dim_to_shape[dim]
            self.thickness_slider[index].max = nmax
            self.thickness_slider[index].value = nmax
            self.update_slider_range(index, nmax, nmax - 1)

            lims = xlims[dim].values
            self.update_slider_readout(
                              index,
                              lims[0],
                              lims[1],
                              [0, nmax - 1])

            self.unit_labels[index].value = coord_units[dim]
        # for dim, item in parameters.items():
        #     # TODO: for now we prevent a ragged coord from being along a slider
        #     # dimension, as it causes conceptual issues with respect to
        #     # resampling a displayed image.
        #     if dim == multid_coord:
        #         if not self.slider[dim].disabled:
        #             raise RuntimeError("A ragged coordinate cannot lie along "
        #                                "a slider dimension, it must be one of "
        #                                "the displayed dimensions.")
        #         self.buttons[dim].disabled = True
        #     # # Dimension labels
        #     # self.dim_labels[dim].value = item["labels"]

        #     # Dimension slider
        #     size = item["slider"]
        #     # Caution: we need to update max first because it is set to 100 by
        #     # default in ipywidgets, thus preventing a value to be set above
        #     # 100.
        #     self.slider[dim].max = size - 1
        #     self.slider[dim].value = size // 2

        #     # Thickness slider
        #     self.thickness_slider[dim].max = item["thickness_slider"]
        #     self.thickness_slider[dim].value = item["thickness_slider"]
        #     # Only index slicing is allowed when ragged coords are present
        #     if multid_coord is not None:
        #         self.thickness_slider[dim].value = 0.
        #         self.thickness_slider[dim].layout.display = 'none'
        #     self.thickness_slider[dim].step = item["thickness_slider"] * 0.01

        #     # Slider readouts
        #     self.update_slider_readout(*item["slider_readout"])

    def get_slider_bounds(self):
        bounds = {}
        for index, sl in self.slider.items():
            pos = sl.value
            delta = self.thickness_slider[index].value
            lower = pos - (delta // 2) + ((delta + 1) % 2)
            upper = pos + (delta // 2) + 1
            bounds[self.index_to_dim[index]] = [lower, upper]
        return bounds

    def get_slider_value(self, key):
        """
        Return value of the slider corresponding to the requested dimension.
        """
        return self.slider[key].value

    def get_thickness_slider_value(self, key):
        """
        Return value of the thickness slider corresponding to the requested
        dimension.
        """
        return self.thickness_slider[key].value

    def get_slider_dims_and_values(self):
        """
        Return the values of all sliders that are not disabled.
        """
        # slider_values = {}
        # for index, sl in self.slider.items():
        #     # if not sl.disabled:
        #     slider_values[self.index_to_dim[index]] = sl.value
        # return slider_values
        return {self.index_to_dim[index]: [index, sl.value] for index, sl in self.slider.items()}

    def get_non_profile_slider_values(self, profile_dim):
        """
        Return the values of all sliders that do not correspond to the profile
        dimension.
        """
        slider_values = {}
        for dim, sl in self.slider.items():
            if dim != profile_dim:
                slider_values[dim] = sl.value
        return slider_values

    def get_buttons_and_disabled_sliders(self):
        """
        Get the values and dimensions of the buttons for dimensions that have
        active sliders.
        """
        buttons_and_dims = {}
        for dim, button in self.buttons.items():
            if self.slider[dim].disabled:
                buttons_and_dims[dim] = button.value
        return buttons_and_dims

    def clear_profile_buttons(self, profile_dim=None):
        """
        Reset all profile buttons, when for example a new dimension is
        displayed along one of the figure axes.
        """
        for dim, but in self.profile_button.items():
            if dim != profile_dim:
                but.button_style = ""

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

    # def get_slice_bounds(self, key, left, centre, right):
    #     """
    #     Get the bounds of the slice for a given dimension, computed from the
    #     slider position and the thickness of the slice. Return as floats.
    #     """
    #     thickness = self.thickness_slider[key].value
    #     if thickness == 0.0:
    #         lower = left
    #         upper = right
    #     else:
    #         lower = centre - 0.5 * thickness
    #         upper = centre + 0.5 * thickness
    #     return lower, upper

    def get_slice_bounds(self,
                                   key,
                                   lower,
                                   upper,
                                   indices,
                                   is_multid=False):
        """
        Get the bounds of the slice for a given dimension as a single string.
        """
        if is_multid:
            return "i{}:i{}".format(indices[0], indices[1])
        else:
            return "{}:{}".format(value_to_string(lower),
                                  value_to_string(upper))

    def update_slider_readout(self,
                              key,
                              lower,
                              upper,
                              indices,
                              is_multid=False):
        """
        Update the slider readout with new slider bounds.
        """
        self.slider_readout[key].value = self.get_slice_bounds(
            key, lower, upper, indices, is_multid)
