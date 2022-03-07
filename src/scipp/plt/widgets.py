# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

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

        # self.profile_button = ipw.Button(description="Profile",
        #                                  button_style="",
        #                                  layout={"width": "initial"})
        # # TODO: hide the profile button for 3D plots. Renable this once
        # # profile picking is supported on 3D plots
        # if ndim == 3:
        #     self.profile_button.layout.display = 'none'
        # if len(self._slider_dims) != 1:
        #     self.profile_button.layout.display = 'none'

        multid_coord = None
        for array in masks.values():
            for dim, coord in array.meta.items():
                if len(coord.dims) > 1:
                    multid_coord = dim

        for dim in dims:
            slider = ipw.IntSlider(step=1,
                                   description=dim,
                                   min=0,
                                   max=sizes[dim],
                                   continuous_update=True,
                                   readout=True,
                                   layout={"width": "400px"})

            continuous_update = ipw.Checkbox(value=True,
                                             description="Continuous update",
                                             indent=False,
                                             layout={"width": "20px"})
            ipw.jslink((continuous_update, 'value'), (slider, 'continuous_update'))

            slider_readout = ipw.Label()

            # unit = ipw.Label(value=self._formatters[dim]['unit'],
            #                  layout={"width": "60px"})
            unit = ipw.Label(value='unit', layout={"width": "60px"})

            self._controls[dim] = {
                'continuous': continuous_update,
                'slider': slider,
                'value': slider_readout,
                'unit': unit
            }

        first = True
        for index, dim in enumerate(self._slider_dims):
            row = list(self._controls[dim].values())
            self.container.append(ipw.HBox(row))

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
        for dim in self._controls:
            self._controls[dim]['slider'].observe(self._slider_moved, names="value")

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
        return {dim: self._controls[dim]['slider'].value for dim in self._slider_dims}

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
