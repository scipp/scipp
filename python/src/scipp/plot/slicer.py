# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .tools import axis_to_dim_label


class Slicer:

    def __init__(self, input_data, axes, value_name, cb, show_variances,
                 button_options):

        import ipywidgets as widgets

        self.input_data = input_data
        self.show_variances = ((self.input_data.variances is not None) and
                               show_variances)
        self.cb = cb
        self.value_name = value_name

        # Get the dimensions of the image to be displayed
        self.coords = self.input_data.coords
        self.labels = self.input_data.labels
        self.shapes = dict(zip(self.input_data.dims, self.input_data.shape))

        # Size of the slider coordinate arrays
        self.slider_nx = dict()
        # Save dimensions tags for sliders, e.g. Dim.X
        self.slider_dims = []
        # Store coordinates of dimensions that will be in sliders
        self.slider_x = dict()
        # Store labels for sliders if any
        self.slider_labels = dict()
        # Protect against duplicate entries in axes
        if len(axes) != len(set(axes)):
            raise RuntimeError("Duplicate entry in axes: {}".format(axes))
        # Iterate through axes and collect dimensions
        for ax in axes:
            dim, lab, var = axis_to_dim_label(self.input_data, ax)
            if (lab is not None) and (dim in axes):
                raise RuntimeError("The dimension of the labels cannot also "
                                   "be specified as another axis.")
            self.slider_dims.append(dim)
            key = str(dim)
            self.slider_labels[key] = lab
            self.slider_x[key] = var
            self.slider_nx[key] = self.shapes[dim]
        self.ndim = len(self.slider_dims)

        # Initialise list for VBox container
        self.vbox = []

        # Initialise slider and label containers
        self.lab = dict()
        self.slider = dict()
        self.buttons = dict()
        self.showhide = dict()
        # Default starting index for slider
        indx = 0

        # Now begin loop to construct sliders
        button_values = [None] * (self.ndim - len(button_options)) + \
            button_options[::-1]
        for i, dim in enumerate(self.slider_dims):
            key = str(dim)
            # If this is a 3d projection, place slices half-way
            if len(button_options) == 3:
                indx = (self.slider_nx[key] - 1) // 2
            if self.slider_labels[key] is not None:
                descr = self.slider_labels[key]
            else:
                descr = key
            # Add an IntSlider to slide along the z dimension of the array
            self.slider[key] = widgets.IntSlider(
                value=indx,
                min=0,
                max=self.slider_nx[key] - 1,
                step=1,
                description=descr,
                continuous_update=True,
                readout=False, disabled=((i >= self.ndim-2) and
                                         len(button_options) == 2)
            )
            labvalue = str(self.slider_x[key].values[indx])
            if self.ndim == 2:
                self.slider[key].layout.display = 'none'
                labvalue = descr
            # Add a label widget to display the value of the z coordinate
            self.lab[key] = widgets.Label(value=labvalue)
            # Add one set of buttons per dimension
            self.buttons[key] = widgets.ToggleButtons(
                options=button_options, description='',
                value=button_values[i],
                disabled=False,
                button_style='',
                style={"button_width": "70px"})
            view_desc = "&#128065;"
            if button_values[i] is None:
                view_desc = "<span style='opacity:0.3;'>{}</span>".format(
                    view_desc)
            # self.showhide[key] = widgets.Checkbox(
            #     value=(button_values[i] is not None),
            #     description=view_desc,
            #     disabled=(button_values[i] is None),
            #     indent=False
            # )
            self.showhide[key] = widgets.Button(
                # value=(button_values[i] is not None),
                description="hide", #view_desc,
                disabled=(button_values[i] is None),
                button_style='', # 'success', 'info', 'warning', 'danger' or ''
                # tooltip='Description',
                # icon='eye',
                # style={"button_width": "70px"}
            )
            self.showhide[key].layout.width = "70px"
            self.showhide[key].style.button_color = 'gray'
            setattr(self.buttons[key], "dim_str", key)
            setattr(self.buttons[key], "dim", dim)
            setattr(self.buttons[key], "old_value", self.buttons[key].value)
            setattr(self.slider[key], "dim_str", key)
            setattr(self.slider[key], "dim", dim)
            setattr(self.showhide[key], "dim_str", key)
            setattr(self.showhide[key], "value", (button_values[i] is not None))
            # Add observer to buttons
            self.buttons[key].on_msg(self.update_buttons)
            # Add an observer to the slider
            self.slider[key].observe(self.update_slice, names="value")
            # Add observer to checkboxes
            # self.showhide[key].observe(self.update_showhide, names="value")
            self.showhide[key].on_click(self.update_showhide)
            # Add the row of slider + buttons
            row = [self.slider[key], self.lab[key], self.buttons[key]]
            if len(button_options) == 3:
                row += [widgets.HTML(value="&nbsp;&nbsp;&nbsp;&nbsp;"), self.showhide[key]]
            self.vbox.append(widgets.HBox(row))
        return
