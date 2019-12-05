# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from ..config import plot as config
from .render import render_plot
from .slicer import Slicer
from .tools import axis_label, parse_colorbar


# Other imports
import numpy as np
import ipywidgets as widgets
from matplotlib import cm
from matplotlib.colors import Normalize, LogNorm
import matplotlib.pyplot as plt


def plot_2d(input_data=None, axes=None, cb=None, filename=None, name=None,
            figsize=None, show_variances=False):
    """
    Plot a 2D slice through a N dimensional dataset. For every dimension above
    2, a slider is created to adjust the position of the slice in that
    particular dimension.
    """

    var = input_data[name]
    if axes is None:
        axes = var.dims

    # Parse colorbar
    cbar = parse_colorbar(cb)

    sv = Slicer2d(input_data=var, axes=axes, cb=cbar,
                  show_variances=show_variances)

    render_plot(figure=sv.fig, widgets=sv.vbox, filename=filename)

    return sv.members


class Slicer2d(Slicer):

    def __init__(self, input_data=None, axes=None, cb=None,
                 show_variances=False):

        super().__init__(input_data=input_data, axes=axes, cb=cb,
                         show_variances=show_variances,
                         button_options=['X', 'Y'])

        self.members.update({"images": {}, "colorbars": {}})

        self.params = {"values": {"name": self.input_data.name,
                                  "cbmin": "min", "cbmax": "max"},
                       "variances": None}
        if self.show_variances:
            self.params["variances"] = {"name": "variances",
                                        "cbmin": "min_var", "cbmax": "max_var"}
        self.extent = {"x": [0, 1], "y": [0, 1]}

        self.fig, ax = plt.subplots(
            1, 1 + self.show_variances,
            figsize=(config.width/config.dpi,
                     config.height/(1.0+self.show_variances)/config.dpi),
            dpi=config.dpi,
            sharex=True, sharey=True)
        self.ax = dict()
        self.im = dict()
        self.cbar = dict()
        if self.show_variances:
            self.ax.update({"values": ax[0], "variances": ax[1]})
        else:
            self.ax["values"] = ax

        # Set colorbar limits once to keep them constant for slicer
        # TODO: should there be auto scaling as slider value is changed?
        for key, val in sorted(self.params.items()):
            if val is not None:
                arr = getattr(self.input_data, key)
                v = arr[np.where(np.isfinite(arr))]
                vmin = np.amin(v)
                vmax = np.amax(v)
                if self.cb["log"]:
                    norm = LogNorm(vmin=self.cb[val["cbmin"]],
                                   vmax=self.cb[val["cbmax"]])
                else:
                    norm = Normalize(vmin=self.cb[val["cbmin"]],
                                     vmax=self.cb[val["cbmin"]])
                self.im[key] = self.ax[key].imshow(
                    [[vmin, vmax],[vmin, vmax]], norm=norm,
                    extent=np.array(list(self.extent.values())).flatten(),
                    origin="lower", interpolation="none", cmap=self.cb["name"])
                self.cbar[key] = plt.colorbar(self.im[key], ax=self.ax[key])
                self.ax[key].set_title(val["name"])
                self.cbar[key].ax.set_ylabel(axis_label(var=self.input_data,
                                                        name=""))
                self.cbar[key].ax.yaxis.set_label_coords(-1.1, 0.5)
                self.members["images"][key] = self.im[key]
                self.members["colorbars"][key] = self.cbar[key]

        # Call update_slice once to make the initial image
        self.update_axes()
        self.update_slice(None)
        self.vbox = widgets.VBox(self.vbox)
        self.vbox.layout.align_items = 'center'
        self.members["fig"] = self.fig
        self.members["ax"] = self.ax

        return

    def update_buttons(self, owner, event, dummy):
        toggle_slider = False
        if not self.slider[owner.dim_str].disabled:
            toggle_slider = True
            self.slider[owner.dim_str].disabled = True
        for key, button in self.buttons.items():
            if (button.value == owner.value) and (key != owner.dim_str):
                if self.slider[key].disabled:
                    button.value = owner.old_value
                else:
                    button.value = None
                button.old_value = button.value
                if toggle_slider:
                    self.slider[key].disabled = False
        owner.old_value = owner.value
        self.update_axes()
        self.update_slice(None)

        return

    def update_axes(self):
        # Go through the buttons and select the right coordinates for the axes
        axlabels = {"x": None, "y": None}
        for key, button in self.buttons.items():
            if self.slider[key].disabled:
                but_val = button.value.lower()
                if not self.histograms[key]:
                    xc = self.slider_x[key].values
                    xmin = 1.5 * xc[0] - 0.5 * xc[1]
                    xmax = 1.5 * xc[-1] - 0.5 * xc[-2]
                    self.extent[but_val] = [xmin, xmax]
                else:
                    self.extent[but_val] = self.slider_x[key].values[[0, -1]]

                axlabels[but_val] = axis_label(
                            self.slider_x[key],
                            name=self.slider_labels[key])

        for key in self.ax.keys():
            self.im[key].set_extent(
                np.array(list(self.extent.values())).flatten())
            self.ax[key].set_xlabel(axlabels["x"])
            self.ax[key].set_ylabel(axlabels["y"])
            self.ax[key].set_xlim(self.extent["x"])
            self.ax[key].set_ylim(self.extent["y"])

        return

    # Define function to update slices
    def update_slice(self, change):
        # The dimensions to be sliced have been saved in slider_dims
        vslice = self.input_data
        # Slice along dimensions with active sliders
        button_dims = [None, None]
        for key, val in self.slider.items():
            if not val.disabled:
                self.lab[key].value = self.make_slider_label(
                    self.slider_x[key], val.value)
                vslice = vslice[val.dim, val.value]
            else:
                button_dims[self.buttons[key].value.lower() == "y"] = val.dim

        # Check if dimensions of arrays agree, if not, plot the transpose
        slice_dims = vslice.dims
        transp = slice_dims == button_dims
        for key in self.im.keys():
            if transp:
                self.im[key].set_data(getattr(vslice, key).T)
            else:
                self.im[key].set_data(getattr(vslice, key))

        return
