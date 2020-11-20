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
        self.data = plot_array.data[slice_obj]
        self.coords = {str(key): plot_array.coords[key] for key in set(plot_array.coords.keys()) - set([slice_obj[0]])}
        self.masks = {key: plot_array.masks[key][slice_obj] for key in plot_array.masks}

    def __getitem__(self, slice_obj):
        return PlotArrayView(self, slice_obj)


class PlotArray:
    """
    Helper class to hold the contents of a DataArray without making copies.
    PlotArray can be sliced to provide a PlotArrayView onto the contents of the
    PlotArray.
    """
    def __init__(self, data_array):
        self.data = data_array.data
        self.coords = {str(key): data_array.coords[key] for key in data_array.coords}
        self.masks = {key: data_array.masks[key] for key in data_array.masks}

    def __getitem__(self, slice_obj):
        return PlotArrayView(self, slice_obj)