# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet
from .panel import PlotPanel
import ipywidgets as ipw
from .._scipp import core as sc


class PlotPanel3d(PlotPanel):
    """
    Additional widgets that control the position, opacity and shape of the
    cut surface in the 3d plot.
    """
    def __init__(self):
        super().__init__()

        self.current_cut_surface_value = None
        self.options = [
            'x', 'y', 'z', 'radius', 'radius_x', 'radius_y', 'radius_z', 'value'
        ]
        self._cut_sliders = {}
        self._cut_surface_thicknesses = {}
        self._current_cut = None

        self._create_cut_surface_controls()

    def _make_cut_controls(self, key, low, high):
        length = (high - low).value
        # 1000 steps, truncated for readable display
        step = float(f'{length/1000:.0e}')
        unit = '' if low.unit == sc.units.dimensionless else f' [{low.unit}]'
        cut_surface_thickness = ipw.BoundedFloatText(
            value=100 * step,  # about 10% of total
            min=0,
            max=length,
            step=step,
            layout={"width": "200px"},
            description=f'Δ{key}{unit}:',
            style={'description_width': 'initial'})
        cut_surface_thickness.observe(self._update_cut_surface, names="value")

        # Add slider to control position of cut surface
        cut_slider = ipw.FloatSlider(min=low.value,
                                     max=high.value,
                                     step=0.5 * cut_surface_thickness.value,
                                     description=f'{key}:',
                                     value=0.5 * (high + low).value,
                                     continuous_update=False,
                                     layout={"width": "350px"})
        cut_unit = ipw.Label(value=str(low.unit))
        cut_slider.observe(self._update_cut_surface, names="value")

        self._cut_sliders[key] = cut_slider
        self._cut_surface_thicknesses[key] = cut_surface_thickness
        controls = ipw.HBox([ipw.HBox([cut_slider, cut_unit]), cut_surface_thickness])
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
            continuous_update=False,
            style={'description_width': '60px'})
        self.opacity_slider.observe(self._update_opacity, names="value")

        # Add buttons to provide a choice of different cut surfaces:
        # - Cartesian X, Y, Z
        # - Cylindrical X, Y, Z (cylinder major axis)
        # - Spherical R
        # - Value-based iso-surface
        # Note additional spaces required in cylindrical names because
        # options must be unique.
        self.cut_surface_buttons = ipw.ToggleButtons(
            options=dict(
                zip(['X ', 'Y ', 'Z ', 'R ', ' X ', ' Y ', ' Z ', ''],
                    range(len(self.options)))),
            value=None,
            description='Cut surface:',
            button_style='',
            tooltips=[
                'X-plane', 'Y-plane', 'Z-plane', 'Sphere', 'Cylinder-X', 'Cylinder-Y',
                'Cylinder-Z', 'Value'
            ],
            icons=(['cube'] * 3) + ['circle-o'] + (['toggle-on'] * 3) + ['magic'],
            style={"button_width": "50px"},
        )
        self.cut_surface_buttons.observe(self._update_cut_surface_buttons,
                                         names="value")
        # Add a capture for a click event: if the active button is clicked,
        # this resets the togglebuttons value to None and deletes the cut
        # surface.
        self.cut_surface_buttons.on_msg(self._check_if_reset_needed)

        self._cut_controls = []
        self._cut_controls_box = ipw.VBox([self.cut_surface_buttons])
        self.container.children = (self.opacity_slider, self._cut_controls_box)

    def set_range(self, key, low, high):
        # TODO scaling? See old impl:
        # self.xminmax["x"] = axparams['x']['lims'] / axparams['x']['scaling']
        controls = self._make_cut_controls(key, low, high)
        self._cut_controls.append(controls)
        self._cut_controls_box.children += (controls, )

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
            self.controller.update_opacity(alpha={"main": change["new"][1]})
        else:
            self.controller.update_opacity(alpha={
                "main": change["new"][0],
                "cut": change["new"][1]
            })

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
        if change['old'] is not None:
            self._cut_controls[change['old']].layout.display = 'none'
        if change["new"] is None:
            self._current_cut = None
        else:
            self._current_cut = self.options[change['new']]
            self._cut_controls[change['new']].layout.display = ''
        self._update_cut_surface()
        self._update_opacity({"new": self.opacity_slider.value})

    def _update_cut_surface(self, change=None):
        """
        Ask the `PlotController3d` to update the pixel colors.
        """
        self.controller.remove_cut_surface()
        if self._current_cut is None:
            return
        delta = self._cut_surface_thicknesses[self._current_cut].value
        self._cut_sliders[self._current_cut].step = 0.5 * delta
        self.controller.add_cut_surface(
            key=self.options[self.cut_surface_buttons.value],
            center=self._cut_sliders[self._current_cut].value,
            delta=delta,
            inactive=self.opacity_slider.lower,
            active=self.opacity_slider.upper)

    def update_data(self, axparams=None):
        """
        If we are currently using value-based slicing, the cut surface needs an
        update when a data update is performed when a slider is moved.
        """
        if self._current_cut == 'value':
            self._update_cut_surface()

    def rescale_to_data(self, vmin=None, vmax=None, mask_info=None):
        """
        Update the pixel colors using new colorbar limits.
        """
        controls = self._make_cut_controls('value', vmin, vmax)
        if len(self._cut_controls) != len(self.options):
            self._cut_controls.append(controls)
            self._cut_controls_box.children += (controls, )
        else:
            controls.layout.display = self._cut_controls[-1].layout.display
            self._cut_controls[-1] = controls
            children = self._cut_controls_box.children[:-1] + (controls, )
            self._cut_controls_box.children = children
