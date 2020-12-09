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
        # Add all coords, including sliced slice dim coord
        self.coords = self.initialize_map(plot_array.coords, slice_obj,
                                          plot_array.isedges)
        # Slice the attributes
        self.attrs = self.initialize_map(plot_array.attrs, slice_obj,
                                         plot_array.isedges)
        # Slice the masks
        self.masks = {}
        for m in plot_array.masks:
            msk = plot_array.masks[m]
            if slice_obj[0] in msk.dims:
                msk = msk[slice_obj]
            self.masks[m] = msk
        # Copy edges info
        self.isedges = plot_array.isedges

    def __getitem__(self, slice_obj):
        return PlotArrayView(self, slice_obj)

    def initialize_map(self, old_map, slice_obj, isedges):
        new_map = {}
        for key in old_map.keys():
            if slice_obj[0] in old_map[key].dims:
                sl = list(slice_obj)
                if isedges[key][slice_obj[0]]:
                    if hasattr(slice_obj[1], "start"):
                        sl[1] = slice(slice_obj[1].start,
                                      slice_obj[1].stop + 1)
                    else:
                        sl[1] = slice(slice_obj[1], slice_obj[1] + 2)
                new_map[key] = old_map[key][tuple(sl)]
            else:
                new_map[key] = old_map[key]
        return new_map


class PlotArray:
    """
    Helper class to hold the contents of a DataArray without making copies.
    PlotArray can be sliced to provide a PlotArrayView onto the contents of the
    PlotArray.
    """
    def __init__(self, data, coords=None, masks=None, attrs=None):
        self.data = data
        self.coords = {}
        self.masks = {}
        self.attrs = {}
        self.isedges = {}
        dim_to_shape = dict(zip(data.dims, data.shape))

        if coords is not None:
            self.initialize_map(self.coords, coords, dim_to_shape)
        if attrs is not None:
            self.initialize_map(self.attrs, attrs, dim_to_shape)
        if masks is not None:
            self.masks.update({m: masks[m] for m in masks})

    def __getitem__(self, slice_obj):
        return PlotArrayView(self, slice_obj)

    def initialize_map(self, new_map, old_map, dim_to_shape):
        for dim, item in old_map.items():
            key = str(dim)
            new_map[key] = item
            self.isedges[key] = {}
            for i, dim_ in enumerate(item.dims):
                self.isedges[key][dim_] = item.shape[
                    i] == dim_to_shape[dim_] + 1
