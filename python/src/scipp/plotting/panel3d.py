# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet
from .panel import PlotPanel
import ipywidgets as ipw
import numpy as np


class PlotPanel3d(PlotPanel):
    """
    Additional widgets that control the position, opacity and shape of the
    cut surface in the 3d plot.
    """
    def __init__(self, unit):
        super().__init__()

        self._unit = unit
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
        self._cut_sliders = {}
        self._cut_units = {}
        self._cut_surface_thicknesses = {}
        self._current_cut = None

        self._create_cut_surface_controls()

    def _make_cut_controls(self, key):
        # TODO use key
        # Allow to change the thickness of the cut surface
        cut_surface_thickness = ipw.BoundedFloatText(
            value=1,
            min=0,
            layout={"width": "200px"},
            description="Thickness:",
            style={'description_width': 'initial'})
        cut_surface_thickness.observe(self._update_cut_surface, names="value")

        # Add slider to control position of cut surface
        cut_slider = ipw.FloatSlider(min=0,
                                     max=1,
                                     step=cut_surface_thickness.value,
                                     description="Position:",
                                     value=0.5,
                                     layout={"width": "350px"})
        cut_unit = ipw.Label()
        cut_checkbox = ipw.Checkbox(value=True,
                                    description="Continuous update",
                                    indent=False,
                                    layout={"width": "20px"})
        cut_checkbox_link = ipw.jslink((cut_checkbox, 'value'),
                                       (cut_slider, 'continuous_update'))
        cut_thickness_link = ipw.jslink((cut_slider, 'step'),
                                        (cut_surface_thickness, 'value'))
        cut_slider.observe(self._update_cut_surface, names="value")

        self._cut_sliders[key] = cut_slider
        self._cut_units[key] = cut_unit
        self._cut_surface_thicknesses[key] = cut_surface_thickness
        controls = ipw.HBox([
            ipw.HBox([cut_slider, cut_unit, cut_checkbox]),
            cut_surface_thickness
        ])
        controls.layout.display = 'none'
        return controls

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
            style={"button_width": "35px"},
        )
        self.cut_surface_buttons.observe(self._update_cut_surface_buttons,
                                         names="value")
        # Add a capture for a click event: if the active button is clicked,
        # this resets the togglebuttons value to None and deletes the cut
        # surface.
        self.cut_surface_buttons.on_msg(self._check_if_reset_needed)

        self._cut_controls = [ self._make_cut_controls(key) for key in self.cut_options ]
        self.container.children = (ipw.HBox([
            self.opacity_slider, self.opacity_checkbox
        ]), ipw.VBox([self.cut_surface_buttons, *self._cut_controls]))

    def get_cut_options(self):
        """
        Accessor for the current possible cut options, so that they can be
        defined only here, and retrieved by the `PlotModel3d` upon
        initialisation.
        """
        return self.cut_options

    def set_position_unit(self, unit):
        self._position_unit = unit

    def set_limits(self, limits):
        #self.xminmax["x"] = axparams['x']['lims'] / axparams['x']['scaling']
        #self.xminmax["y"] = axparams['y']['lims'] / axparams['y']['scaling']
        #self.xminmax["z"] = axparams['z']['lims'] / axparams['z']['scaling']
        # TODO scaling?
        for ax in ['x', 'y', 'z']:
            self._cut_sliders[f'{ax.upper()}plane'].min = limits[ax][0]
            self._cut_sliders[f'{ax.upper()}plane'].max = limits[ax][1]
            self._cut_sliders[f'{ax.upper()}plane'].description = f'{ax}:'
            self._cut_surface_thicknesses[f'{ax.upper()}plane'].max = limits[ax][1] - limits[ax][0]
            if self._position_unit is not None:
                self._cut_units[f'{ax.upper()}plane'].value = str(self._position_unit)
            else:
                self._cut_units[f'{ax.upper()}plane'].value = self.controller.get_coord_unit(
                    axparams[self.cut_surface_buttons.label.replace(
                        ' ', '').lower()]["dim"])
        self.xminmax = limits

    def update_axes(self):
        """
        Reset axes limits and cut surface buttons.
        """
        self.cut_surface_buttons.value = None
        self.current_cut_surface_value = None

    def _update_opacity(self, change):
        """
        Update opacity of all points when opacity slider is changed.
        Take cut surface into account if present.
        """
        if self.cut_surface_buttons.value is None:
            self.controller.update_opacity(alpha=change["new"][1])
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
        print(change)
        if change['old'] is not None:
            self._cut_controls[change['old']].layout.display = 'none'
        if change['new'] is not None:
            self._current_cut = list(self.cut_options)[change['new']]
            self._cut_controls[change['new']].layout.display = ''
        if change["new"] is None:
            self._update_opacity({"new": self.opacity_slider.value})
        else:
            self.controller.update_depth_test(False)
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
            pass
        # Cylindrical X, Y, Z
        elif self.cut_surface_buttons.value < self.cut_options["Sphere"]:
            j = self.cut_surface_buttons.value - 3
            remaining_axes = self.permutations["xyz"[j]]
            rmax = np.abs([
                self.xminmax[remaining_axes[0]][0],
                self.xminmax[remaining_axes[1]][0],
                self.xminmax[remaining_axes[0]][1],
                self.xminmax[remaining_axes[1]][1]
            ]).max() * np.sqrt(2.0)
            self._safe_cut_slider_range_update(0, rmax)
            self.cut_slider.value = 0.5 * self.cut_slider.max
            self.cut_slider.description = "Radius:"
            self._set_cylindrical_or_spherical_unit()
        # Spherical
        elif self.cut_surface_buttons.value == self.cut_options["Sphere"]:
            rmax = np.abs(list(self.xminmax.values())).max() * np.sqrt(3.0)
            self._safe_cut_slider_range_update(0, rmax)
            self.cut_slider.value = 0.5 * self.cut_slider.max
            self.cut_slider.description = "Radius:"
            self._set_cylindrical_or_spherical_unit()
        # Value iso-surface
        elif self.cut_surface_buttons.value == self.cut_options["Value"]:
            self.cut_surface_thickness.max = self.vmax - self.vmin
            self._safe_cut_slider_range_update(self.vmin, self.vmax)
            self.cut_slider.value = 0.5 * (self.vmin + self.vmax)
            # Update slider step because it is no longer related to pixel size.
            # Slice thickness is linked to the step via jslink.
            self.cut_slider.step = (self.cut_slider.max -
                                    self.cut_slider.min) / 10.0
            self.cut_slider.description = "Value:"
            self.cut_unit.value = str(self._unit)
        #if self.cut_surface_buttons.value < self.cut_options["Value"]:
        #    self.cut_slider.step = self.controller.get_pixel_size() * 1.1
        #    self.cut_surface_thickness.max = (self.cut_slider.max -
        #                                      self.cut_slider.min)
        self.lock_surface_update = False

    def _update_cut_surface(self, change=None):
        """
        Ask the `PlotController3d` to update the pixel colors.
        """
        self.controller.update_cut_surface(
            target=self._cut_sliders[self._current_cut].value,
            button_value=self.cut_surface_buttons.value,
            surface_thickness=self._cut_surface_thicknesses[self._current_cut].value,
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
        self.vmin = vmin.value
        self.vmax = vmax.value
        if self.cut_surface_buttons.value == self.cut_options["Value"]:
            self._update_cut_slider_bounds()

    def _set_cylindrical_or_spherical_unit(self):
        """
        For cylindrical and spherical cut surfaces, if the data is dense, set
        slider unit to None as the unit may be ill-defined if the coordinates
        of the dense data do no all have the same dimension.
        """
        if self._position_unit is not None:
            self.cut_unit.value = str(self._position_unit)
        else:
            self.cut_unit.value = ""
