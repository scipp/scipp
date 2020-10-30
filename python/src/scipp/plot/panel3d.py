# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .panel import PlotPanel
import ipywidgets as ipw
import numpy as np


class PlotPanel3d(PlotPanel):
    """
    Additional widgets that control the position, opacity and shape of the
    cut surface in the 3d plot.
    """
    def __init__(self, pixel_size=None):
        super().__init__()

        self.pixel_size = pixel_size
        self.current_cut_surface_value = None
        self.permutations = {"x": ["y", "z"], "y": ["x", "z"], "z": ["x", "y"]}
        self.lock_surface_update = False
        self.vmin = None
        self.vmax = None
        self.xminmax = {}
        self.cut_options = {
            "Xplane": 0,
            "Yplane": 1,
            "Zplane": 2,
            "Xcylinder": 3,
            "Ycylinder": 4,
            "Zcylinder": 5,
            "Sphere": 6,
            "Value": 7
        }

        self._create_cut_surface_controls()

    def _create_cut_surface_controls(self):
        """
        Generate the buttons and sliders with `ipywidgets`.
        """

        # Opacity slider: top value controls opacity if no cut surface is
        # active. If a cut curface is present, the upper slider is the opacity
        # of the slice, while the lower slider value is the opacity of the
        # data not in the cut surface.
        self.opacity_slider = ipw.FloatRangeSlider(
            min=0.0,
            max=1.0,
            value=[0.03, 1],
            step=0.01,
            description="Opacity slider: When no cut surface is active, the "
            "max value of the range slider controls the overall opacity, "
            "and the lower value has no effect. When a cut surface is "
            "present, the max value is the opacity of the slice, while the "
            "min value is the opacity of the background.",
            continuous_update=True,
            style={'description_width': '60px'})
        self.opacity_slider.observe(self._update_opacity, names="value")
        self.opacity_checkbox = ipw.Checkbox(
            value=self.opacity_slider.continuous_update,
            description="Continuous update",
            indent=False,
            layout={"width": "20px"})
        self.opacity_checkbox_link = ipw.jslink(
            (self.opacity_checkbox, 'value'),
            (self.opacity_slider, 'continuous_update'))

        # Add buttons to provide a choice of different cut surfaces:
        # - Cartesian X, Y, Z
        # - Cylindrical X, Y, Z (cylinder major axis)
        # - Spherical R
        # - Value-based iso-surface
        # Note additional spaces required in cylindrical names because
        # options must be unique.
        self.cut_surface_buttons = ipw.ToggleButtons(
            options=[('X ', self.cut_options["Xplane"]),
                     ('Y ', self.cut_options["Yplane"]),
                     ('Z ', self.cut_options["Zplane"]),
                     ('R ', self.cut_options["Sphere"]),
                     (' X ', self.cut_options["Xcylinder"]),
                     (' Y ', self.cut_options["Ycylinder"]),
                     (' Z ', self.cut_options["Zcylinder"]),
                     ('', self.cut_options["Value"])],
            value=None,
            description='Cut surface:',
            button_style='',
            tooltips=[
                'X-plane', 'Y-plane', 'Z-plane', 'Sphere', 'Cylinder-X',
                'Cylinder-Y', 'Cylinder-Z', 'Value'
            ],
            icons=(['cube'] * 3) + ['circle-o'] + (['toggle-on'] * 3) +
            ['magic'],
            style={"button_width": "55px"},
            layout={'width': '350px'})
        self.cut_surface_buttons.observe(self._update_cut_surface_buttons,
                                         names="value")
        # Add a capture for a click event: if the active button is clicked,
        # this resets the togglebuttons value to None and deletes the cut
        # surface.
        self.cut_surface_buttons.on_msg(self._check_if_reset_needed)

        # Allow to change the thickness of the cut surface
        self.cut_surface_thickness = ipw.BoundedFloatText(
            value=self.pixel_size * 1.1,
            min=0,
            layout={"width": "200px"},
            disabled=True,
            description="Thickness:",
            style={'description_width': 'initial'})
        self.cut_surface_thickness.observe(self._update_cut_surface,
                                           names="value")

        # Add slider to control position of cut surface
        self.cut_slider = ipw.FloatSlider(
            min=0,
            max=1,
            step=self.cut_surface_thickness.value,
            description="Position:",
            disabled=True,
            value=0.5,
            layout={"width": "350px"})
        self.cut_checkbox = ipw.Checkbox(value=True,
                                         description="Continuous update",
                                         indent=False,
                                         layout={"width": "20px"},
                                         disabled=True)
        self.cut_checkbox_link = ipw.jslink(
            (self.cut_checkbox, 'value'),
            (self.cut_slider, 'continuous_update'))
        self.cut_slider.observe(self._update_cut_surface, names="value")

        self.cut_thickness_link = ipw.jslink(
            (self.cut_slider, 'step'), (self.cut_surface_thickness, 'value'))
        self.cut_slider.observe(self._update_cut_surface, names="value")

        # Put widgets into boxes
        self.container.children = (ipw.HBox(
            [self.opacity_slider, self.opacity_checkbox]),
                                   ipw.HBox([
                                       self.cut_surface_buttons,
                                       ipw.VBox([
                                           ipw.HBox([
                                               self.cut_slider,
                                               self.cut_checkbox
                                           ]), self.cut_surface_thickness
                                       ])
                                   ]))

    def get_cut_options(self):
        """
        Accessor for the current possible cut options, so that they can be
        defined only here, and retrieved by the `PlotModel3d` upon
        initialisation.
        """
        return self.cut_options

    def update_axes(self, axparams):
        """
        Reset axes limits and cut surface buttons.
        """
        self.xminmax["x"] = axparams['x']['lims']
        self.xminmax["y"] = axparams['y']['lims']
        self.xminmax["z"] = axparams['z']['lims']
        self.cut_surface_buttons.value = None
        self.current_cut_surface_value = None

    def _update_opacity(self, change):
        """
        Update opacity of all points when opacity slider is changed.
        Take cut surface into account if present.
        """
        if self.cut_surface_buttons.value is None:
            self.interface["update_opacity"](alpha=change["new"][1])
        else:
            self._update_cut_surface()

    def _check_if_reset_needed(self, owner, content, buffers):
        """
        If the currently selected surface button is clicked again, then we do
        a reset by hiding the cut surface.
        """
        if owner.value == self.current_cut_surface_value:
            self.cut_surface_buttons.value = None
        self.current_cut_surface_value = owner.value

    def _update_cut_surface_buttons(self, change):
        """
        Handle button update when the type of cut surface is changed.
        """
        if change["new"] is None:
            self.cut_slider.disabled = True
            self.cut_checkbox.disabled = True
            self.cut_surface_thickness.disabled = True
            self._update_opacity({"new": self.opacity_slider.value})
        else:
            self.interface["update_depth_test"](False)
            if change["old"] is None:
                self.cut_slider.disabled = False
                self.cut_checkbox.disabled = False
                self.cut_surface_thickness.disabled = False
            self._update_cut_slider_bounds()
            self._update_cut_surface()

    def _update_cut_slider_bounds(self):
        """
        When axes are changed, we update the possible spatial range for the cut
        surface position sliders.
        We also update the possible range for value-based slicing.
        """
        self.lock_surface_update = True
        # Cartesian X, Y, Z
        if self.cut_surface_buttons.value < self.cut_options["Xcylinder"]:
            minmax = self.xminmax["xyz"[self.cut_surface_buttons.value]]
            if minmax[0] < self.cut_slider.max:
                self.cut_slider.min = minmax[0]
                self.cut_slider.max = minmax[1]
            else:
                self.cut_slider.max = minmax[1]
                self.cut_slider.min = minmax[0]
            self.cut_slider.value = 0.5 * (minmax[0] + minmax[1])
        # Cylindrical X, Y, Z
        elif self.cut_surface_buttons.value < self.cut_options["Sphere"]:
            j = self.cut_surface_buttons.value - 3
            remaining_axes = self.permutations["xyz"[j]]
            rmax = np.abs([
                self.xminmax[remaining_axes[0]][0],
                self.xminmax[remaining_axes[1]][0],
                self.xminmax[remaining_axes[0]][1],
                self.xminmax[remaining_axes[1]][1]
            ]).max()
            self.cut_slider.min = 0
            self.cut_slider.max = rmax * np.sqrt(2.0)
            self.cut_slider.value = 0.5 * self.cut_slider.max
        # Spherical
        elif self.cut_surface_buttons.value == self.cut_options["Sphere"]:
            rmax = np.abs(list(self.xminmax.values())).max()
            self.cut_slider.min = 0
            self.cut_slider.max = rmax * np.sqrt(3.0)
            self.cut_slider.value = 0.5 * self.cut_slider.max
        # Value iso-surface
        elif self.cut_surface_buttons.value == self.cut_options["Value"]:
            self.cut_surface_thickness.max = self.vmax - self.vmin
            # We need to be careful about the order here, as ipywidgets throws
            # if we set min larger than the current max
            if self.vmax > self.cut_slider.min:
                self.cut_slider.max = self.vmax
                self.cut_slider.min = self.vmin
            else:
                self.cut_slider.min = self.vmin
                self.cut_slider.max = self.vmax
            self.cut_slider.value = 0.5 * (self.vmin + self.vmax)
            # Update slider step because it is no longer related to pixel size.
            # Slice thickness is linked to the step via jslink.
            self.cut_slider.step = (self.cut_slider.max -
                                    self.cut_slider.min) / 10.0
        if self.cut_surface_buttons.value < self.cut_options["Value"]:
            self.cut_slider.step = self.pixel_size * 1.1
        self.lock_surface_update = False

    def _update_cut_surface(self, change=None):
        """
        Ask the `PlotController3d` to update the pixel colors.
        """
        self.interface["update_cut_surface"](
            target=self.cut_slider.value,
            button_value=self.cut_surface_buttons.value,
            surface_thickness=self.cut_surface_thickness.value,
            opacity_lower=self.opacity_slider.lower,
            opacity_upper=self.opacity_slider.upper)

    def update_data(self, axparams=None):
        """
        If we are currently using value-based slicing, the cut surface needs an
        update when a data update is performed when a slider is moved.
        """
        if self.cut_surface_buttons.value == self.cut_options["Value"]:
            self._update_cut_surface()

    def rescale_to_data(self, vmin=None, vmax=None, mask_info=None):
        """
        Update the pixel colors using new colorbar limits.
        """
        self.vmin = vmin
        self.vmax = vmax
        if self.cut_surface_buttons.value == self.cut_options["Value"]:
            self._update_cut_slider_bounds()
