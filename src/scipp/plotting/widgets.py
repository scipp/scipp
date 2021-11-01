# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet
from functools import partial
from html import escape
from ..utils import value_to_string


class PlotWidgets:
    """
    Widgets containing a slider for each of the input's dimensions, as well as
    buttons to modify the currently displayed axes.
    It also provides buttons to hide/show masks.
    """
    def __init__(self, *, dims, formatters, ndim, dim_label_map, masks, sizes):

        import ipywidgets as ipw
        self._dims = dims
        self._labels = dim_label_map
        self._controller = None
        self._formatters = formatters
        # Dict of controls for each dim, one entry per dim of data
        self._controls = {}

        # The container list to hold all widgets
        self.container = []
        # dim_buttons: buttons to control which dimension the slider controls
        self.dim_buttons = {}
        # all_masks_button: button to hide/show all masks in a single click
        self.all_masks_button = None

        self._slider_dims = dims[:len(dims) - ndim]

        self.profile_button = ipw.Button(description="Profile",
                                         button_style="",
                                         layout={"width": "initial"})
        # TODO: hide the profile button for 3D plots. Renable this once
        # profile picking is supported on 3D plots
        if ndim == 3:
            self.profile_button.layout.display = 'none'
        if len(self._slider_dims) != 1:
            self.profile_button.layout.display = 'none'

        multid_coord = None
        for array in masks.values():
            for dim, coord in array.meta.items():
                if len(coord.dims) > 1:
                    multid_coord = dim

        for dim in dims:
            slider = ipw.IntSlider(step=1,
                                   continuous_update=True,
                                   readout=False,
                                   layout={"width": "200px"})

            continuous_update = ipw.Checkbox(value=True,
                                             description="Continuous update",
                                             indent=False,
                                             layout={"width": "20px"})
            ipw.jslink((continuous_update, 'value'), (slider, 'continuous_update'))

            thickness_slider = ipw.IntSlider(min=1,
                                             step=1,
                                             description="Thickness",
                                             continuous_update=False,
                                             readout=False,
                                             layout={'width': "180px"},
                                             style={'description_width': 'initial'})
            # If there is a multid coord, we only allow slices of thickness 1
            if multid_coord is not None:
                thickness_slider.layout.display = 'none'

            slider_readout = ipw.Label()

            unit = ipw.Label(value=self._formatters[dim]['unit'],
                             layout={"width": "60px"})

            self._controls[dim] = {
                'continuous': continuous_update,
                'slider': slider,
                'value': slider_readout,
                'unit': unit,
                'thickness': thickness_slider
            }

        first = True
        for index, dim in enumerate(self._slider_dims):
            # Add one set of buttons per dimension
            self.dim_buttons[index] = {}
            for dim_ in self._dims:
                self.dim_buttons[index][dim_] = ipw.Button(
                    description=dim_label_map[dim_],
                    button_style='info' if dim == dim_ else '',
                    disabled=((dim != dim_) and (dim_ in self._slider_dims)
                              or (dim_ == multid_coord)),
                    layout={"width": 'initial'})
                # Add observer to buttons
                self.dim_buttons[index][dim_].on_click(
                    self.update_buttons(index, dim=dim_))

            # Add the row of sliders + buttons
            row = list(self.dim_buttons[index].values()) + list(
                self._controls[dim].values())
            if first:
                first = False
                row.append(self.profile_button)
            self.container.append(ipw.HBox(row))

        self._add_masks_controls(masks)
        self.initialize(sizes=sizes)

    def _ipython_display_(self):
        """
        IPython display representation for Jupyter notebooks.
        """
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        """
        Gather all widgets in a single container box.
        """
        import ipywidgets as ipw
        return ipw.VBox(self.container)

    def _add_masks_controls(self, masks):
        """
        Add checkboxes for individual masks, as well as a global hide/show all
        toggle button.
        """
        import ipywidgets as ipw
        masks_found = False
        self.mask_checkboxes = {}
        for name in masks:
            self.mask_checkboxes[name] = {}
            if len(masks[name].masks) > 0:
                masks_found = True
                for key in masks[name].masks:
                    self.mask_checkboxes[name][key] = ipw.Checkbox(
                        value=True,
                        description="{}:{}".format(escape(name), escape(key)),
                        indent=False,
                        layout={"width": "initial"})
                    setattr(self.mask_checkboxes[name][key], "mask_group", name)
                    setattr(self.mask_checkboxes[name][key], "mask_name", key)

        if masks_found:
            self.masks_lab = ipw.Label(value="Masks:")

            # Add a master button to control all masks in one go
            self.all_masks_button = ipw.ToggleButton(value=True,
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
                ipw.HBox([self.masks_lab, self.all_masks_button, self.masks_box])
            ]

    def update_buttons(self, index, dim):
        """
        Custom update for 2D grid of toggle buttons.
        """
        def _update(owner=None):
            if owner.button_style == "info":
                return
            old_dim = self._slider_dims[index]
            self._slider_dims[index] = dim

            # Put controls for new dim into layout
            children = self.container[index].children
            pos = len(self._dims)
            children = children[:pos] + tuple(
                self._controls[dim].values()) + children[pos + 5:]
            self.container[index].children = children

            self.dim_buttons[index][old_dim].button_style = ""
            self.dim_buttons[index][dim].button_style = "info"

            for i in set(self.dim_buttons.keys()) - set([index]):
                self.dim_buttons[i][dim].disabled = True
                self.dim_buttons[i][old_dim].disabled = False

            self._controller.swap_dimensions(index, old_dim, dim)

        return _update

    def update_slider_range(self, dim, thickness, nmax, set_value=True):
        """
        When the thickness slider value is changed, we need to update the
        bounds of the slice position slider so that it does no overrun the data
        range. In short, the min and max of the position slider are
        0.5*thickness and N - 0.5*thickness, respectively, where N is the size
        of the slider dimension.
        Since we are dealing with integers, when the thickness is an even
        number, the range covered is shifted by 1 towards the right.
        """
        slider = self._controls[dim]['slider']
        sl_min = (thickness // 2) + (thickness % 2) - 1
        sl_max = nmax - (thickness // 2)
        if sl_max < slider.min:
            slider.min = sl_min
            slider.max = sl_max
        else:
            slider.max = sl_max
            slider.min = sl_min
        if set_value:
            slider.value = sl_min

    def update_thickness(self, dim):
        """
        When the slice thickness is changed, we update the slider range and
        update the data in the slice.
        """
        def _update(change=None):
            self.update_slider_range(dim,
                                     change["new"],
                                     change["owner"].max - 1,
                                     set_value=False)
            self._controller.update_data()

        return _update

    def _set_slider_defaults(self, dim, max_value):
        controls = self._controls[dim]
        controls['thickness'].max = max_value
        controls['thickness'].value = 1
        self.update_slider_range(dim, controls['thickness'].value, max_value - 1)

        # Disable slider and profile button if there is only a single bin
        # TODO did we break this for event data (previously fixed)?
        disabled = max_value == 1
        controls['slider'].disabled = disabled
        controls['continuous'].disabled = disabled
        controls['thickness'].disabled = disabled

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

    def connect(self, controller):
        """
        Connect the widget interface to the callbacks provided by the
        `PlotController`.
        """
        self._controller = controller
        self.profile_button.on_click(
            partial(controller.toggle_profile_view, dims=self._slider_dims))
        for dim in self._controls:
            self._controls[dim]['slider'].observe(self._slider_moved, names="value")
            self._controls[dim]['thickness'].observe(self.update_thickness(dim),
                                                     names="value")

        for name in self.mask_checkboxes:
            for m in self.mask_checkboxes[name]:
                self.mask_checkboxes[name][m].observe(controller.toggle_mask,
                                                      names="value")

    def initialize(self, sizes):
        """
        Initialize widget parameters once the `PlotModel`, `PlotView` and
        `PlotController` have been created, since, for instance, slider limits
        depend on the dimensions of the input data, which are not known until
        the `PlotModel` is created.
        """
        self._sizes = sizes
        for dim in self._dims:
            self._set_slider_defaults(dim, sizes[dim])

    def _slider_moved(self, _):
        self._controller.update_data(slices=self.slices)

    @property
    def slices(self):
        """
        Get the current range covered by the thick slice.
        """
        bounds = {}
        for dim in self._slider_dims:
            pos = self._controls[dim]['slider'].value
            delta = self._controls[dim]['thickness'].value
            lower = pos - (delta // 2) + ((delta + 1) % 2)
            upper = pos + (delta // 2) + 1
            if lower + 1 == upper:
                # Just slice without rebin
                bounds[dim] = lower
            else:
                bounds[dim] = [lower, upper]
        return bounds

    def clear_profile_button(self):
        """
        Reset profile button, when for example a new dimension is
        displayed along one of the figure axes.
        """
        self.profile_button.button_style = ""

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

    def update_slider_readout(self, bounds):
        """
        Update the slider readout with new slider bounds.
        """
        for dim in self._slider_dims:

            def format(val):
                form = self._formatters[dim]['linear']
                if form is None:
                    return value_to_string(val)
                # pos=None causes datetime formatter to return full string
                # rather than attempting to set a separate label and returning
                # offset
                return form(val, pos=None)

            if bounds[dim].values.ndim == 0:
                bound = f'{format(bounds[dim].value)}'
            else:
                low, high = bounds[dim].values
                bound = f'[{format(low)} {format(high)}]'
            self._controls[dim]['value'].value = bound
