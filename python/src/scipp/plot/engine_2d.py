from .tools import parse_params, make_fake_coord, to_bin_edges, to_bin_centers
from .._utils import name_with_unit, value_to_string
from .._scipp import core as sc

# Other imports
import numpy as np


class PlotEngine2d(PlotEngine):

    def __init__(self,
                 parent=None,
                 scipp_obj_dict=None,
                 axes=None,
                 masks=None,
                 cmap=None,
                 log=None,
                 vmin=None,
                 vmax=None,
                 color=None,
                 button_options=None,
                 aspect=None,
                 positions=None):

        super().__init__(parent=parent,
                         scipp_obj_dict=scipp_obj_dict,
                         axes=axes,
                         masks=masks,
                         cmap=cmap,
                         log=log,
                         vmin=vmin,
                         vmax=vmax,
                         color=color,
                         button_options=button_options,
                         aspect=aspect,
                         positions=positions)
        return


    def update_buttons(self, owner, event, dummy):
        toggle_slider = False
        if not self.slider[owner.dim].disabled:
            toggle_slider = True
            self.slider[owner.dim].disabled = True
            self.thickness_slider[owner.dim].disabled = True
        for dim, button in self.buttons.items():
            if (button.value == owner.value) and (dim != owner.dim):
                if self.slider[dim].disabled:
                    button.value = owner.old_value
                else:
                    button.value = None
                button.old_value = button.value
                if toggle_slider:
                    self.slider[dim].disabled = False
                    self.thickness_slider[dim].disabled = False
        owner.old_value = owner.value
        self.update_axes()
        return

    def update_axes(self):
        # Go through the buttons and select the right coordinates for the axes
        for dim, button in self.buttons.items():
            if self.slider[dim].disabled:
                but_val = button.value.lower()
                self.extent[but_val] = self.slider_xlims[self.name][dim].values
                self.axparams[but_val]["lims"] = self.extent[but_val].copy()
                if getattr(self,
                           "log" + but_val) and (self.extent[but_val][0] <= 0):
                    self.axparams[but_val]["lims"][
                        0] = 1.0e-03 * self.axparams[but_val]["lims"][1]
                # self.axparams[but_val]["labels"] = name_with_unit(
                #     self.slider_label[self.name][dim]["coord"],
                #     name=self.slider_label[self.name][dim]["name"])
                self.axparams[but_val]["labels"] = name_with_unit(
                    self.data_arrays[self.name].coords[dim])
                self.axparams[but_val]["dim"] = dim
                # Get the dimensions corresponding to the x/y buttons
                self.button_dims[but_val == "x"] = button.dim
                self.dim_to_xy[dim] = but_val

        extent_array = np.array(list(self.extent.values())).flatten()
        self.current_lims['x'] = extent_array[:2]
        self.current_lims['y'] = extent_array[2:]

        # TODO: if labels are used on a 2D coordinates, we need to update
        # the axes tick formatter to use xyrebin coords
        for xy, param in self.axparams.items():
            # Create coordinate axes for resampled array to be used as image
            offset = 2 * (xy == "y")
            self.xyrebin[xy] = sc.Variable(
                dims=[param["dim"]],
                values=np.linspace(extent_array[0 + offset],
                                   extent_array[1 + offset],
                                   self.image_resolution[xy] + 1),
                unit=self.data_arrays[self.name].coords[param["dim"]].unit)

        # Set axes labels
        self.ax.set_xlabel(self.axparams["x"]["labels"])
        self.ax.set_ylabel(self.axparams["y"]["labels"])
        for xy, param in self.axparams.items():
            axis = getattr(self.ax, "{}axis".format(xy))
            is_log = getattr(self, "log{}".format(xy))
            axis.set_major_formatter(
                self.slider_axformatter[self.name][param["dim"]][is_log])
            axis.set_major_locator(
                self.slider_axlocator[self.name][param["dim"]][is_log])

        # Set axes limits and ticks
        with warnings.catch_warnings():
            warnings.filterwarnings("ignore", category=UserWarning)
            self.image.set_extent(extent_array)
            if len(self.masks[self.name]) > 0:
                for m in self.masks[self.name]:
                    self.members["masks"][m].set_extent(extent_array)
            self.ax.set_xlim(self.axparams["x"]["lims"])
            self.ax.set_ylim(self.axparams["y"]["lims"])

        # # If there are no multi-d coords, we update the edges and widths only
        # # once here.
        # if not self.contains_multid_coord[self.name]:
        #     self.slice_coords()
        # Update the image using resampling
        self.update_slice()

        # Some annoying house-keeping when using X/Y buttons: we need to update
        # the deeply embedded limits set by the Home button in the matplotlib
        # toolbar. The home button actually brings the first element in the
        # navigation stack to the top, so we need to modify the first element
        # in the navigation stack in-place.
        if self.fig is not None:
            if self.fig.canvas.toolbar is not None:
                if len(self.fig.canvas.toolbar._nav_stack._elements) > 0:
                    # Get the first key in the navigation stack
                    key = list(self.fig.canvas.toolbar._nav_stack._elements[0].
                               keys())[0]
                    # Construct a new tuple for replacement
                    alist = []
                    for x in self.fig.canvas.toolbar._nav_stack._elements[0][
                            key]:
                        alist.append(x)
                    alist[0] = (*self.slider_xlims[self.name][
                        self.button_dims[1]].values, *self.slider_xlims[
                            self.name][self.button_dims[0]].values)
                    # Insert the new tuple
                    self.fig.canvas.toolbar._nav_stack._elements[0][
                        key] = tuple(alist)

        self.rescale_to_data()


        if self.profile_viewer is not None:
            self.update_profile_axes()

        return