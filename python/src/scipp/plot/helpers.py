# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet


class Plot(dict):
    """
    The Plot object is used as output for the plot command.
    It is a small wrapper around python dict, with an `_ipython_display_`
    representation.
    The dict will contain one entry for each entry in the input supplied to
    the plot function.
    More functionalities can be added in the future.
    """
    def __init__(self, *arg, **kw):
        super(Plot, self).__init__(*arg, **kw)

    def _ipython_display_(self):
        """
        IPython display representation for Jupyter notebooks.
        """
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        """
        Return plot contents into a single VBocx container
        """
        import ipywidgets as ipw
        contents = []
        for item in self.values():
            if item is not None:
                contents.append(item._to_widget())
        return ipw.VBox(contents)

    def show(self):
        for item in self.values():
            item.show()

    def as_static(self, *args, **kwargs):
        """
        Convert all the contents of the dict to static plots, releasing the
        memory held by the plot (which makes a full copy of the input data).
        """
        for key, item in self.items():
            self[key] = item.as_static(*args, **kwargs)


class PlotArrayView:
    """
    Helper class to provide a view for the slicing mechanism of PlotArray.
    """
    def __init__(self, plot_array, slice_obj):
        # Add data slice
        self.data = plot_array.data[slice_obj]
        # Add all coords except the slice dim coord
        self.coords = {key: plot_array.coords[key] for key in set(plot_array.coords.keys()) - set([slice_obj[0]])}
        # Add the slice dim coord, with range + 1 in case of bin edges
        sl = list(slice_obj)
        if plot_array.isedges[slice_obj[0]]:
            if isinstance(slice_obj[1], int):
                sl[1] = slice(slice_obj[1], slice_obj[1] + 1)
            else:
                sl[1] = slice(slice_obj[1].start, slice_obj[1].stop + 1)
        self.coords[slice_obj[0]] = plot_array.coords[slice_obj[0]][tuple(sl)]
        # Slice the masks
        self.masks = {key: plot_array.masks[key][slice_obj] for key in plot_array.masks}
        # Copy edges info
        self.isedges = plot_array.isedges

    def __getitem__(self, slice_obj):
        return PlotArrayView(self, slice_obj)


class PlotArray:
    """
    Helper class to hold the contents of a DataArray without making copies.
    PlotArray can be sliced to provide a PlotArrayView onto the contents of the
    PlotArray.
    """
    def __init__(self, data, coords, masks=None):
        self.data = data
        self.coords = {}
        self.masks = {}
        self.isedges = {}
        dim_to_shape = dict(zip(data.dims, data.shape))
        if coords is not None:
            for key in coords:
                ks = str(key)
                self.coords[ks] = coords[key]
                self.isedges[ks] = coords[key].shape[-1] == dim_to_shape[coords[key].dims[-1]] + 1
        if masks is not None:
            self.masks.update({key: masks[key] for key in masks})

    def __getitem__(self, slice_obj):
        return PlotArrayView(self, slice_obj)