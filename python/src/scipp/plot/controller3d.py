
import ipywidgets as ipw
import numpy as np


class PlotController1d:

    def __init__(self, data_names, ndim):

        self.view = None
        # self.widgets = ipw.VBox()
        # self.keep_buttons = {}
        # self.data_names = data_names
        # self.slice_label= None
        # # self.make_keep_button()
        # if ndim < 2:
        #     self.widgets.layout.display = 'none'

        self.widgets = self.create_cut_surface_controls()



    def _ipython_display_(self):
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        return self.widgets





    def create_cut_surface_controls(self):

        # Opacity slider: top value controls opacity if no cut surface is
        # active. If a cut curface is present, the upper slider is the opacity
        # of the slice, while the lower slider value is the opacity of the
        # data not in the cut surface.
        self.opacity_slider = ipw.FloatRangeSlider(
            min=0.0,
            max=1.0,
            value=[0.1, 1],
            step=0.01,
            description="Opacity slider: When no cut surface is active, the "
            "max value of the range slider controls the overall opacity, "
            "and the lower value has no effect. When a cut surface is "
            "present, the max value is the opacity of the slice, while the "
            "min value is the opacity of the background.",
            continuous_update=True,
            style={'description_width': '60px'})
        self.opacity_slider.observe(self.update_opacity, names="value")
        self.opacity_checkbox = ipw.Checkbox(
            value=self.opacity_slider.continuous_update,
            description="Continuous update",
            indent=False,
            layout={"width": "20px"})
        self.opacity_checkbox_link = ipw.jslink(
            (self.opacity_checkbox, 'value'),
            (self.opacity_slider, 'continuous_update'))

        # self.toggle_outline_button = ipw.ToggleButton(value=show_outline,
        #                                                   description='',
        #                                                   button_style='')
        # self.toggle_outline_button.observe(self.toggle_outline, names="value")
        # # Run a trigger to update button text
        # self.toggle_outline({"new": show_outline})

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
        self.cut_surface_buttons.observe(self.update_cut_surface_buttons,
                                         names="value")
        # Add a capture for a click event: if the active button is clicked,
        # this resets the togglebuttons value to None and deletes the cut
        # surface.
        self.cut_surface_buttons.on_msg(self.check_if_reset_needed)

        # Allow to change the thickness of the cut surface
        self.cut_surface_thickness = ipw.BoundedFloatText(
            value=self.pixel_size * 1.1,
            min=0,
            layout={"width": "150px"},
            disabled=True,
            description="Thickness:",
            style={'description_width': 'initial'})

        # Add slider to control position of cut surface
        self.cut_slider = ipw.FloatSlider(min=0,
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
        self.cut_slider.observe(self.update_cut_surface, names="value")

        # Allow to change the thickness of the cut surface
        self.cut_surface_thickness = ipw.BoundedFloatText(
            value=0.05 * self.box_size.max(),
            min=0,
            layout={"width": "150px"},
            disabled=True,
            description="Thickness:",
            style={'description_width': 'initial'})
        self.cut_surface_thickness.observe(self.update_cut_surface,
                                           names="value")
        self.cut_thickness_link = ipw.jslink(
            (self.cut_slider, 'step'), (self.cut_surface_thickness, 'value'))
        self.cut_slider.observe(self.update_cut_surface, names="value")

        # Put widgets into boxes
        return ipw.HBox([
            self.cut_surface_buttons,
            ipw.VBox([
                ipw.HBox([self.cut_slider, self.cut_checkbox]),
                self.cut_surface_thickness
            ])
        ])


    def update_opacity(self, change):
        """
        Update opacity of all points when opacity slider is changed.
        Take cut surface into account if present.
        """
        if self.cut_surface_buttons.value is None:
            arr = self.points_geometry.attributes["rgba_color"].array
            arr[:, 3] = change["new"][1]
            self.points_geometry.attributes["rgba_color"].array = arr
            # There is a strange effect with point clouds and opacities.
            # Results are best when depthTest is False, at low opacities.
            # But when opacities are high, the points appear in the order
            # they were drawn, and not in the order they are with respect
            # to the camera position. So for high opacities, we switch to
            # depthTest = True.
            self.points_material.depthTest = change["new"][1] > 0.9
        else:
            self.update_cut_surface({"new": self.cut_slider.value})

    def check_if_reset_needed(self, owner, content, buffers):
        if owner.value == self.current_cut_surface_value:
            self.cut_surface_buttons.value = None
        self.current_cut_surface_value = owner.value

    def update_cut_surface_buttons(self, change):
        if change["new"] is None:
            self.cut_slider.disabled = True
            self.cut_checkbox.disabled = True
            self.cut_surface_thickness.disabled = True
            self.update_opacity({"new": self.opacity_slider.value})
        else:
            self.points_material.depthTest = False
            if change["old"] is None:
                self.cut_slider.disabled = False
                self.cut_checkbox.disabled = False
                self.cut_surface_thickness.disabled = False
            self.update_cut_slider_bounds()

    def update_cut_slider_bounds(self):
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
            self.remaining_inds = [(j + 1) % 3, (j + 2) % 3]
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
            self.cut_slider.min = self.vminmax[0]
            self.cut_slider.max = self.vminmax[1]
            self.cut_slider.value = 0.5 * (self.vminmax[0] + self.vminmax[1])
            # Update slider step because it is no longer related to pixel size.
            # Slice thickness is linked to the step via jslink.
            self.cut_slider.step = (self.cut_slider.max -
                                    self.cut_slider.min) / 10.0
        if self.cut_surface_buttons.value < self.cut_options["Value"]:
            self.cut_slider.step = self.pixel_size * 1.1

    def update_cut_surface(self, change):
        newc = None
        target = self.cut_slider.value
        # Cartesian X, Y, Z
        if self.cut_surface_buttons.value < self.cut_options["Xcylinder"]:
            newc = np.where(
                np.abs(self.positions[:, self.cut_surface_buttons.value] -
                       target) < 0.5 * self.cut_surface_thickness.value,
                self.opacity_slider.upper, self.opacity_slider.lower)
        # Cylindrical X, Y, Z
        elif self.cut_surface_buttons.value < self.cut_options["Sphere"]:
            newc = np.where(
                np.abs(
                    np.sqrt(self.positions[:, self.remaining_inds[0]] *
                            self.positions[:, self.remaining_inds[0]] +
                            self.positions[:, self.remaining_inds[1]] *
                            self.positions[:, self.remaining_inds[1]]) -
                    target) < 0.5 * self.cut_surface_thickness.value,
                self.opacity_slider.upper, self.opacity_slider.lower)
        # Spherical
        elif self.cut_surface_buttons.value == self.cut_options["Sphere"]:
            newc = np.where(
                np.abs(
                    np.sqrt(self.positions[:, 0] * self.positions[:, 0] +
                            self.positions[:, 1] * self.positions[:, 1] +
                            self.positions[:, 2] * self.positions[:, 2]) -
                    target) < 0.5 * self.cut_surface_thickness.value,
                self.opacity_slider.upper, self.opacity_slider.lower)
        # Value iso-surface
        elif self.cut_surface_buttons.value == self.cut_options["Value"]:
            newc = np.where(
                np.abs(self.vslice - target) <
                0.5 * self.cut_surface_thickness.value,
                self.opacity_slider.upper, self.opacity_slider.lower)

        # Unfortunately, one cannot edit the value of the geometry array
        # in-place, as this does not trigger an update on the threejs side.
        # We have to update the entire array.
        c3 = self.points_geometry.attributes["rgba_color"].array
        c3[:, 3] = newc
        self.points_geometry.attributes["rgba_color"].array = c3
