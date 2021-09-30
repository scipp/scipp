# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
from .view1d import PlotView1d
from .resampling_model import ResamplingMode


class MarkerModel:
    def __init__(self):
        self._counter = -1
        self._markers = {}

    def generate(self):
        from ..utils import make_random_color
        color = make_random_color(fmt='rgba')
        self._counter += 1
        self._markers[self._counter] = color
        return self._counter, color

    def __delitem__(self, key):
        del self._markers[key]


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
                 dims,
                 vmin=None,
                 vmax=None,
                 norm=None,
                 resampling_mode=None,
                 scale=None,
                 widgets=None,
                 model=None,
                 profile_model=None,
                 panel=None,
                 profile=None,
                 view=None):

        self._dims = dims
        self.widgets = widgets
        self.model = model
        # TODO calling copy here may not be enough to avoid interdependencies
        self._profile_model = profile_model
        self._profile_markers = MarkerModel()
        self.panel = panel
        self.model.mode = resampling_mode
        if view.figure.toolbar is not None:
            view.figure.toolbar.set_resampling_mode_display(self.model.is_resampling)
        self.profile = profile
        if profile is not None:
            self._profile_view = PlotView1d(figure=profile, formatters=view.formatters)
            self._profile_model.dims = self._dims[:-len(self.model.dims)]
            self._profile_model.mode = resampling_mode
        self.view = view

        self.vmin = vmin
        self.vmax = vmax
        self.norm = norm if norm is not None else "linear"

        self.scale = {dim: "linear" for dim in self._dims}
        if scale is not None:
            for dim, item in scale.items():
                self.scale[dim] = item
        self.view.set_scale(scale=self.scale)

    def render(self):
        """
        Update axes (and data) to render the figure once all components
        have been created.
        """
        if self.profile is not None:
            self.profile.connect()

        self.widgets.connect(controller=self)
        self.view.connect(controller=self)
        if self.panel is not None:
            self.panel.controller = self
        self.update_axes()

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
        # Keep current zoom state
        self.update_axes(dims=self.model.dims[::-1], slices=self.view.current_limits)

    def toggle_dim_scale(self, dim):
        """
        Toggle dim scale from toolbar button signal.
        """
        def toggle(change):
            self.scale[dim] = "log" if change['new'] else "linear"
            self.view.set_scale(scale=self.scale)
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

    def toggle_resampling_mode(self, change):
        """
        Toggle data resampling mode from toolbar button signal.
        """
        value = change['owner'].value
        mode = ResamplingMode.mean if value else ResamplingMode.sum
        self.model.mode = mode
        if self._profile_model:
            self._profile_model.mode = mode
        # Call update_axes to update data and rescale. Also turns profile view off,
        # since updating it would be complicated if there are saved lines.
        self.update_axes()

    def swap_dimensions(self, index, old_dim, new_dim):
        """
        Swap one dimension for another in the displayed axes.
        """
        dims = self.model.dims
        dims = [old_dim if dim == new_dim else dim for dim in dims]
        self.update_axes(dims=dims)

    def update_axes(self, dims=None, normalize=True, slices=None):
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
        if self.panel is not None:
            self.panel.update_axes()
        if self.profile is not None:
            self.toggle_profile_view(dims=list(set(self._dims) - set(dims)))
        self.update_data(slices=slices)
        if normalize:
            self.rescale_to_data()

    def update_data(self, *, slices=None):
        """
        This function is called when the data in the displayed 1D plot or 2D
        image is to be updated. This happens for instance when we move a slider
        which is navigating an additional dimension. It is also always
        called when update_axes is called since the displayed data needs to be
        updated when the axes have changed.
        """
        if slices is None:
            slices = self.widgets.slices
        else:
            slices.update(self.widgets.slices)
        new_values = self.model.update_data(slices)
        self.widgets.update_slider_readout(new_values.meta)

        self.view.update_data(new_values, mask_info=self.get_masks_info())
        if self.panel is not None:
            self.panel.update_data(new_values)
        if self.profile is not None:
            self._update_slice_area()

    def _update_slice_area(self):
        if len(self._profile_model.dims) != 1:
            return
        for dim in self._profile_model.dims:
            bounds = self.view.data.meta[dim]
            if len(bounds.dims) == 1:
                xstart, xend = bounds.values
            else:
                xstart = bounds.value
                xend = bounds.value
            # TODO Once profiles != 1d are supported, handle axes here
            self.profile.set_slice_area(xstart, xend)

    def toggle_mask(self, change):
        """
        Hide or show a given mask.
        """
        self.view.toggle_mask(change)
        self.refresh()
        if self.profile is not None:
            self.profile.toggle_mask(change["owner"].mask_group,
                                     change["owner"].mask_name, change["new"])

    def get_masks_info(self):
        """
        Get information on masks from widgets.
        """
        return self.widgets.get_masks_info()

    def keep_line(self, target=None, name=None, color=None, line_id=None):
        """
        Keep the currently displayed line.
        """
        self.view.keep_line(color=color, line_id=line_id, names=[name])

    def remove_line(self, target=None, name=None, line_id=None):
        """
        Remove a line previously added using `keep_line`.
        """
        self.view.remove_line(line_id=line_id, names=[name])

    def toggle_profile_view(self, owner=None, dims=None):
        """
        Show or hide the 1d plot displaying the profile along an additional
        dimension.
        As we do this, we also collect some information on the limits of the
        view area to be displayed.
        """
        self._profile_model.dims = dims
        if owner is None:
            visible = False
            self.widgets.clear_profile_button()
        else:
            assert len(dims) == 1  # TODO support 2d profiles
            if owner.button_style == "info":
                owner.button_style = ""
                visible = False
            else:
                owner.button_style = "info"
                visible = True

            if visible:
                self._profile_view.set_scale(scale=self.scale)
            if not visible or self.profile.is_visible():
                self.view.clear_marks()

        self.profile.toggle_view(visible=visible)
        self.profile.toggle_hover_visibility(False)
        self.view.toggle_mouse_events(active=visible)

    def hover(self, slices):
        if self.profile is not None and self.profile.is_visible():
            if slices:
                slices.update(self.widgets.slices)
                for dim in self._profile_model.dims:
                    del slices[dim]
                new_values = self._profile_model.update_data(slices=slices)
                self._profile_view.update_data(new_values,
                                               mask_info=self.get_masks_info())
                self.profile.toggle_hover_visibility(True)
            else:
                self.profile.toggle_hover_visibility(False)

    def click(self, slices):
        if not slices:
            return
        if self.profile is not None and self.profile.is_visible():
            index, color = self._profile_markers.generate()
            self.profile.keep_line(color=color, line_id=index)
            self.profile.rescale_to_data()
            self.view.mark(color=color, index=index, slices=slices)

    def pick(self, index):
        if self.profile is not None and self.profile.is_visible():
            del self._profile_markers[index]
            self.profile.remove_line(line_id=index)
            self.view.remove_mark(index=index)

    def update_profile(self, slices):
        """
        This is called from a mouse move event, which requires an update of the
        currently displayed profile.
        We gather the information on which dims should be sliced by the model,
        ask the model to slice down the data, and send the new data returned by
        the model to the profile view.
        """
        slices.update(self.widgets.slices)
        for dim in self._profile_model.dims:
            del slices[dim]
        new_values = self._profile_model.update_data(slices=slices)
        self._profile_view.update_data(new_values, mask_info=self.get_masks_info())

    def redraw(self):
        """
        Update the model data dicts and re-draw the figure.
        """
        self.model.update()
        self.update_data()
