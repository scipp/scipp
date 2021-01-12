# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
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
                 dim_label_map=None,
                 positions=None,
                 masks=None,
                 multid_coord=None):

        self.multid_coord = multid_coord

        # Dict of callbacks passed on from the `PlotController`
        self.interface = {}
        # The container list to hold all widgets
        self.container = []
        # unit_labels: label to display slider coordinate unit
        self.unit_labels = {}
        # slider: slider to slice along additional dimensions of the array
        self.slider = {}
        # index_to_dim: map slider index to coordinate dim
        self.index_to_dim = {}
        # slider_readout: label to display range currently covered by slider
        self.slider_readout = {}
        # thickness_slider: slider to control thickness of slice
        self.thickness_slider = {}
        # dim_buttons: buttons to control which dimension the slider controls
        self.dim_buttons = {}
        # profile_button: button to show/hide a profile plot along selected dim
        self.profile_button = {}
        # continuous_update: checkbox to turn on/off slider continuous update
        self.continuous_update = {}
        # all_masks_button: button to hide/show all masks in a single click
        self.all_masks_button = None

        pos_dim = dim_label_map[
            positions] if positions in dim_label_map else positions

        slider_dims = {}
        for ax, dim in axes.items():
            if isinstance(ax, int) and dim != pos_dim:
                slider_dims[ax] = dim
        possible_dims = set(axes.values()) - set([pos_dim])

        # Now begin loop to construct sliders
        for index, (ax, dim) in enumerate(slider_dims.items()):

            self.unit_labels[index] = ipw.Label(layout={"width": "60px"})

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
            # If there is a multid coord, we only allow slices of thickness 1
            if self.multid_coord is not None:
                self.thickness_slider[index].layout.display = 'none'

            self.slider_readout[index] = ipw.Label()

            self.profile_button[index] = ipw.Button(
                description="Profile",
                button_style="",
                layout={"width": "initial"})

            # TODO: hide the profile button for 3D plots. Renable this once
            # profile picking is supported on 3D plots
            if ndim == 3:
                self.profile_button[index].layout.display = 'none'

            # Add one set of buttons per dimension
            self.dim_buttons[index] = {}
            for dim_ in possible_dims:
                self.dim_buttons[index][dim_] = ipw.Button(
                    description=dim_label_map[dim_]
                    if dim_ in dim_label_map else dim_,
                    button_style='info' if dim == dim_ else '',
                    disabled=((dim != dim_) and (dim_ in slider_dims.values())
                              or (dim_ == self.multid_coord)),
                    layout={"width": 'initial'})
                # Add observer to buttons
                self.dim_buttons[index][dim_].on_click(self.update_buttons)
                setattr(self.dim_buttons[index][dim_], "index", index)
                setattr(self.dim_buttons[index][dim_], "dim", dim_)

            self.index_to_dim[index] = dim
            self.index_to_dim[dim] = index

            setattr(self.slider[index], "index", index)
            setattr(self.thickness_slider[index], "index", index)
            setattr(self.profile_button[index], "index", index)

            # Add the row of sliders + buttons
            row = list(self.dim_buttons[index].values()) + [
                self.continuous_update[index], self.slider[index],
                self.slider_readout[index], self.unit_labels[index],
                self.thickness_slider[index], self.profile_button[index]
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

    def update_buttons(self, owner=None):
        """
        Custom update for 2D grid of toggle buttons.
        """
        if owner.button_style == "info":
            return
        new_ind = owner.index
        new_dim = owner.dim

        self.index_to_dim[new_ind] = new_dim
        self.index_to_dim[new_dim] = new_ind

        old_dim = None
        for dim in self.dim_buttons[new_ind]:
            if self.dim_buttons[new_ind][dim].button_style == "info":
                old_dim = self.dim_buttons[new_ind][dim].dim
            self.dim_buttons[new_ind][dim].button_style = ""
        owner.button_style = "info"

        # Update the slider max and value.
        # Note that updating the slider readout has to be done by the
        # controller in "swap_dimensions" because the widgets don't have access
        # to the model which holds the coordinate values.
        self.update_thickness_slider_range(new_ind, new_dim)
        for index in set(self.dim_buttons.keys()) - set([new_ind]):
            self.dim_buttons[index][new_dim].disabled = True
            self.dim_buttons[index][old_dim].disabled = False

        self.unit_labels[new_ind].value = self.interface["get_coord_unit"](
            new_dim)
        self.interface["swap_dimensions"](new_ind, old_dim, new_dim)

    def get_index_dim(self, index):
        """
        Get the dimension corresponding to the supplied index.
        """
        return self.index_to_dim[index]

    def update_slider_range(self, index, thickness, nmax, set_value=True):
        """
        When the thickness slider value is changed, we need to update the
        bounds of the slice position slider so that it does no overrun the data
        range. In short, the min and max of the position slider are
        0.5*thickness and N - 0.5*thickness, respectively, where N is the size
        of the slider dimension.
        Since we are dealing with integers, when the thickness is an even
        number, the range covered is shifted by 1 towards the right.
        """
        sl_min = (thickness // 2) + (thickness % 2) - 1
        sl_max = nmax - (thickness // 2)
        if sl_max < self.slider[index].min:
            self.slider[index].min = sl_min
            self.slider[index].max = sl_max
        else:
            self.slider[index].max = sl_max
            self.slider[index].min = sl_min
        if set_value:
            self.slider[index].value = sl_min

    def update_thickness(self, change=None):
        """
        When the slice thickness is changed, we update the slider range and
        update the data in the slice.
        """
        self.update_slider_range(change["owner"].index,
                                 change["new"],
                                 change["owner"].max - 1,
                                 set_value=False)
        self.interface["update_data"](change)

    def update_thickness_slider_range(self, ind, dim):
        """
        Update the slider max and values. Before we update the value, we need
        to lock the data update which is linked to the slider.
        """
        self.interface["lock_update_data"]()
        self._set_slider_defaults(ind, self.interface["get_dim_shape"](dim))
        self.interface["unlock_update_data"]()

    def _set_slider_defaults(self, index, max_value):
        """
        On axes change, set the thickness to 1, its max range to the shape of
        the dim, and set the position slider to accordingly.
        """
        self.thickness_slider[
            index].max = 1 if self.multid_coord is not None else max_value
        self.thickness_slider[index].value = 1
        self.update_slider_range(index, self.thickness_slider[index].value,
                                 max_value - 1)

        # Disable slider and profile button if there is only a single bin
        disabled = max_value == 1
        self.slider[index].disabled = disabled
        self.continuous_update[index].disabled = disabled
        self.thickness_slider[index].disabled = disabled
        self.profile_button[index].disabled = disabled

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
        for index in self.slider:
            self.profile_button[index].on_click(
                callbacks["toggle_profile_view"])
            self.slider[index].observe(callbacks["update_data"], names="value")
            self.thickness_slider[index].observe(self.update_thickness,
                                                 names="value")
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

    def initialise(self, dim_to_shape, ranges, coord_units):
        """
        Initialise widget parameters once the `PlotModel`, `PlotView` and
        `PlotController` have been created, since, for instance, slider limits
        depend on the dimensions of the input data, which are not known until
        the `PlotModel` is created.
        """
        for index in self.thickness_slider:
            dim = self.index_to_dim[index]
            self._set_slider_defaults(index, dim_to_shape[dim])
            lims = ranges[dim]
            val = self.slider[index].value
            self.update_slider_readout(index, lims[0], lims[1], [val, val + 1])
            self.unit_labels[index].value = coord_units[dim]

    def get_slider_bounds(self, exclude=None):
        """
        Get the current range covered by the thick slice (in integers).
        """
        bounds = {}
        for index, sl in self.slider.items():
            dim = self.index_to_dim[index]
            if dim != exclude:
                pos = sl.value
                delta = self.thickness_slider[index].value
                lower = pos - (delta // 2) + ((delta + 1) % 2)
                upper = pos + (delta // 2) + 1
                bounds[dim] = [lower, upper]
        return bounds

    def clear_profile_buttons(self, exclude=None):
        """
        Reset all profile buttons, when for example a new dimension is
        displayed along one of the figure axes.
        """
        for index, but in self.profile_button.items():
            if index != exclude:
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

    def get_slice_extent(self,
                         lower,
                         upper,
                         indices,
                         is_multid=False,
                         precision=1):
        """
        Get the bounds of the slice for a given dimension as a single string.
        """
        if is_multid:
            return "i{}:i{}".format(indices[0], indices[1])
        else:
            return "{}:{}".format(value_to_string(lower, precision=precision),
                                  value_to_string(upper, precision=precision))

    def update_slider_readout(self,
                              key,
                              lower,
                              upper,
                              indices,
                              is_multid=False):
        """
        Update the slider readout with new slider bounds.
        """
        self.slider_readout[key].value = self.get_slice_extent(
            lower, upper, indices, is_multid)
