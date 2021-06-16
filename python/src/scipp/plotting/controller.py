# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .view1d import PlotView1d


class PlotController:
    """
    Main controller class.

    This handles all communications between the `PlotWidgets`, `PlotView`,
    `PlotModel`, `PlotPanel` and `PlotProfile`.

    It mainly holds information about limits and dimension labels of coordinate
    axes, which is necessary to update figures in a generic way.
    It also keeps a record of which coordinates contain bin-edges and
    bin-centers, as this information is lost once the model has converted
    everything to bin edges to allow for more generic code.

    """
    def __init__(self,
                 axes=None,
                 name=None,
                 vmin=None,
                 vmax=None,
                 norm=None,
                 scale=None,
                 multid_coord=None,
                 widgets=None,
                 model=None,
                 profile_model=None,
                 panel=None,
                 profile=None,
                 view=None):

        self.widgets = widgets
        self.model = model
        # TODO calling copy here may not be enough to avoid interdependencies
        self._profile_model = profile_model
        self.panel = panel
        self.profile = profile
        if profile is not None:
            self._profile_view = PlotView1d(figure=profile,
                                            formatters=view.formatters)
        self.view = view

        self.axes = axes
        self.name = name
        self.update_data_lock = False
        self.axparams = {}

        self.vmin = vmin
        self.vmax = vmax
        self.norm = norm if norm is not None else "linear"

        self.scale = {dim: "linear" for dim in self.axes.values()}
        if scale is not None:
            for dim, item in scale.items():
                self.scale[dim] = item

        # Save the current profile dimension
        self.profile_dim = None
        # Keep track if a coordinate with more than one dimension is present
        self.multid_coord = multid_coord

        sizes = self.model.data_arrays[self.name].sizes

        self._initialize_widgets(sizes)
        self.initialize_model()

    def render(self, norm=None):
        """
        Update axes (and data) to render the figure once all components
        have been created.
        """
        if self.profile is not None:
            self.connect_profile()

        self.connect_widgets()
        self.connect_view()
        if self.panel is not None:
            self.connect_panel()
        self.update_axes()
        self.update_norm_button(norm)

    def _initialize_widgets(self, sizes):
        """
        Initialize widget parameters once the `PlotModel`, `PlotView` and
        `PlotController` have been created.
        """
        ranges = {}
        for dim in self.widgets.get_slider_bounds():
            ranges[dim] = self.model.get_slice_coord_bounds(
                self.name, dim, [0, 1])
        self.widgets.initialize(sizes, ranges)

    def initialize_model(self):
        """
        Dummy initialization for `PlotModel`.
        """
        return

    def connect_widgets(self):
        """
        Connect callbacks to the `PlotWidgets` interface.
        """
        self.widgets.connect({
            "toggle_profile_view": self.toggle_profile_view,
            "update_data": self.update_data,
            "toggle_mask": self.toggle_mask,
            "lock_update_data": self.lock_update_data,
            "unlock_update_data": self.unlock_update_data,
            "swap_dimensions": self.swap_dimensions,
        })

    def connect_view(self):
        """
        Connect callbacks to the `PlotView` interface.
        """
        view_callbacks = {
            "update_profile": self.update_profile,
            "toggle_hover_visibility": self.toggle_hover_visibility,
            "keep_line": self.keep_line,
            "remove_line": self.remove_line
        }
        figure_callbacks = {
            "rescale_to_data": self.rescale_to_data,
            "transpose": self.transpose,
            "toggle_dim_scale": self.toggle_dim_scale,
            "toggle_norm": self.toggle_norm,
            "home_view": self.home_view,
            "pan_view": self.pan_view,
            "zoom_view": self.zoom_view,
            "save_view": self.save_view
        }
        self.view.connect(view_callbacks=view_callbacks,
                          figure_callbacks=figure_callbacks)

    def connect_panel(self):
        """
        Dummy connect for `PlotPanel`.
        """
        return

    def connect_profile(self):
        """
        Connect callbacks to the `PlotWidgets` interface.
        """
        self.profile.connect()

    def lock_update_data(self):
        """
        When the thickness slider is changed, the range, and possibly the
        value, of the position slider are changed. We therefore temporary lock
        data updates until all slider ranges and values have been updated
        before manually updating the displayed data slice.
        """
        self.update_data_lock = True

    def unlock_update_data(self):
        """
        Release the data update lock.
        """
        self.update_data_lock = False

    def home_view(self, button=None):
        self.update_axes()

    def pan_view(self, button=None):
        self.view.pan_view()

    def zoom_view(self, button=None):
        self.view.zoom_view()

    def save_view(self, button=None):
        self.view.save_view()

    def find_vmin_vmax(self, button=None):
        """
        Find sensible min and max values for the data.
        If the limits were requested by the user: if the rescale button is
        pressed by the user, it means we forcibly update the self.vmin/vmax.
        If no button is pressed, we use the global limits instead of the
        min and max values found by the model.
        """
        [vmin, vmax] = self.model.rescale_to_data(scale=self.norm)
        if self.vmin is not None:
            if button is None:
                vmin = self.vmin
            else:
                self.vmin = None
        if self.vmax is not None:
            if button is None:
                vmax = self.vmax
            else:
                self.vmax = None
        return vmin, vmax

    def rescale_to_data(self, button=None):
        """
        Automatically rescale the y axis (1D plot) or the colorbar (2D+3D
        plots) to the minimum and maximum value inside the currently displayed
        data slice.
        """
        vmin, vmax = self.find_vmin_vmax(button=button)
        self.view.rescale_to_data(vmin, vmax)
        if self.panel is not None:
            self.panel.rescale_to_data(vmin=vmin,
                                       vmax=vmax,
                                       mask_info=self.get_masks_info())
        self.refresh()

    def refresh(self):
        """
        Update the view. This is used when either vmin and vmax limits have
        been changed, or the normalization has been modified.
        """
        self.view.refresh(mask_info=self.get_masks_info())

    def transpose(self, owner=None):
        """
        Transpose the displayed axes.
        """
        self.update_axes(dims=self.model.dims[::-1])

    def toggle_dim_scale(self, dim):
        """
        Toggle dim scale from toolbar button signal.
        """
        def toggle(change):
            self.scale[dim] = "log" if change['new'] else "linear"
            self.update_axes()

        return toggle

    def toggle_norm(self, owner):
        """
        Toggle data normalization from toolbar button signal.
        """
        self.norm = "log" if owner.value else "linear"
        vmin, vmax = self.find_vmin_vmax()
        self.view.toggle_norm(self.norm, vmin, vmax)
        self.refresh()

    def swap_dimensions(self, index, old_dim, new_dim):
        """
        Swap one dimension for another in the displayed axes.
        """
        dims = self.model.dims
        dims = [old_dim if dim == new_dim else dim for dim in dims]
        self.update_axes(dims=dims)
        # Update the slider readout here because the widgets do not have access
        # to the model, which holds the coordinates.
        lower, upper = self.model.get_slice_coord_bounds(
            self.name, new_dim, [0, 1])
        self.widgets.update_slider_readout(index, lower, upper, [0, 1],
                                           new_dim == self.multid_coord)

    def update_norm_button(self, *args, **kwargs):
        """
        Change state of norm button according to supplied norm value.
        """
        self.view.update_norm_button(*args, **kwargs)

    def update_axes(self, dims=None, normalize=True):
        """
        This function is called when a dimension that is displayed along a
        given axis is changed. This happens for instance when we want to
        flip/transpose a 2D image, or display a new dimension along the x-axis
        in a 1D plot.
        This function gathers the relevant parameters about the axes currently
        selected for display, and then offloads the computation of the new
        state to the model. If then gets the updated data back from the model
        and sends it over to the view for display.
        """
        if dims is None:
            dims = self.model.dims
        else:
            self.model.dims = dims
        self.view.dims = dims
        # TODO mechanism for params from 3d model
        # TODO also for PlotPanel3d
        # other_params = self.model.update_axes(self.axparams)
        # if other_params is not None:
        #     self.axparams.update(other_params)
        self.view.update_axes(scale=self.scale)
        if self.panel is not None:
            self.panel.update_axes()
        if self.profile is not None:
            self.toggle_profile_view()
        self.update_data()
        if normalize:
            self.rescale_to_data()

    def update_data(self, change=None):
        """
        This function is called when the data in the displayed 1D plot or 2D
        image is to be updated. This happens for instance when we move a slider
        which is navigating an additional dimension. It is also always
        called when update_axes is called since the displayed data needs to be
        updated when the axes have changed.
        """
        owner_dim = None

        if self.update_data_lock:
            return

        slices = self.widgets.get_slider_bounds()
        if change is not None:
            owner_dim = self.widgets.get_index_dim(change["owner"].index)
            lower, upper = self.model.get_slice_coord_bounds(
                self.name, owner_dim, slices[owner_dim])
            self.widgets.update_slider_readout(change["owner"].index, lower,
                                               upper, slices[owner_dim],
                                               owner_dim == self.multid_coord)

        info = {"slice_label": self._make_slice_label(slices, "")[1:]}

        new_values = self.model.update_data(slices)
        self.view.update_data(new_values,
                              info=info,
                              mask_info=self.get_masks_info())
        if self.panel is not None:
            self.panel.update_data(info)
        if self.profile_dim is not None:
            if owner_dim == self.profile_dim:
                self.profile.update_slice_area(lower, upper)
            else:
                self.model.update_profile_model(visible=True,
                                                slices=slices,
                                                profile_dim=self.profile_dim)

    def toggle_mask(self, change):
        """
        Hide or show a given mask.
        """
        self.view.toggle_mask(change)
        self.refresh()
        if self.profile is not None:
            self.profile.toggle_mask(change["owner"].mask_group,
                                     change["owner"].mask_name, change["new"])

    def _make_axparam(self, dim):
        return {"scale": self.scale[dim], "dim": dim}

    def get_masks_info(self):
        """
        Get information on masks from widgets.
        """
        return self.widgets.get_masks_info()

    def keep_line(self, target=None, name=None, color=None, line_id=None):
        """
        Get a message from either a panel (1d plot) or a view (picking) to keep
        the currently displayed line or profile.
        Send the message to the appropriate target: either the view1d or the
        profile view.
        """
        if name is None:
            name = self.name
        if target == "profile":
            self.profile.keep_line(name=name, color=color, line_id=line_id)
            self.profile.rescale_to_data()
        else:
            self.view.keep_line(name=name, color=color, line_id=line_id)

    def remove_line(self, target=None, name=None, line_id=None):
        """
        Get a message from either a panel (1d plot) or a view (picking) to
        remove a given line or profile.
        Send the message to the appropriate target: either the view1d or the
        profile view.
        """
        if name is None:
            name = self.name
        if target == "profile":
            self.profile.remove_line(name=name, line_id=line_id)
        else:
            self.view.remove_line(name=name, line_id=line_id)

    def toggle_profile_view(self, owner=None):
        """
        Show or hide the 1d plot displaying the profile along an additional
        dimension.
        As we do this, we also collect some information on the limits of the
        view area to be displayed.
        """
        if owner is None:
            self.profile_dim = None
            visible = False
            self.widgets.clear_profile_buttons()
        else:
            self.profile_dim = self.widgets.get_index_dim(owner.index)
            if owner.button_style == "info":
                owner.button_style = ""
                visible = False
            else:
                owner.button_style = "info"
                self.widgets.clear_profile_buttons(exclude=owner.index)
                visible = True

            if visible:
                profile_axparams = {'x': self._make_axparam(self.profile_dim)}
                self._profile_model.update_axes(axparams=profile_axparams)
                self._profile_view.update_axes(axparams=profile_axparams)
            if not visible or self.profile.is_visible():
                self.view.reset_profile()

        self.profile.toggle_view(visible=visible)
        self.toggle_hover_visibility(False)
        self.view.update_profile_connection(visible=visible)

        if visible:
            slices = self.widgets.get_slider_bounds()
            self.model.update_profile_model(visible=visible,
                                            slices=slices,
                                            profile_dim=self.profile_dim)
            lower, upper = self.model.get_slice_coord_bounds(
                self.name, self.profile_dim, slices[self.profile_dim])
            self.profile.update_slice_area(lower, upper)

    def update_profile(self, slices):
        """
        This is called from a mouse move event, which requires an update of the
        currently displayed profile.
        We gather the information on which dims should be sliced by the model,
        ask the model to slice down the data, and send the new data returned by
        the model to the profile view.
        """
        info = {"slice_label": ""}
        # TODO
        # ax_dims = {self.axparams[xyz]["dim"]: xyz for xyz in self.axparams}
        # xydata = {'x': xdata, 'y': ydata}

        slices.update(self.widgets.get_slider_bounds(exclude=self.profile_dim))

        # Add pixel locations to profile label
        # for dim in ax_dims:
        #    info["slice_label"] = "{},{}:{}".format(
        #        info["slice_label"], dim,
        #        value_to_string(xydata[ax_dims[dim]], precision=1))

        # info["slice_label"] = self._make_slice_label(slices,
        #                                              info["slice_label"])[1:]

        new_values = self._profile_model.update_data(slices=slices)
        self._profile_view.update_data(new_values,
                                       info=info,
                                       mask_info=self.get_masks_info())

    def _make_slice_label(self, slices, label):
        # Add slice ranges to profile label
        for dim in slices:
            lower, upper = self.model.get_slice_coord_bounds(
                self.name, dim, slices[dim])
            label = "{},{}[{}]".format(
                label, dim,
                self.widgets.get_slice_extent(lower, upper, slices[dim],
                                              dim == self.multid_coord))
        return label

    def toggle_hover_visibility(self, value):
        """
        Show/hide the profile view depending on the value of the profile button
        in the widgets.
        """
        self.profile.toggle_hover_visibility(value)

    def redraw(self):
        """
        Update the model data dicts and re-draw the figure.
        """
        self.model.update_data_arrays()
        self.update_data()
