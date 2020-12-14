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


class PlotArray:
    """
    Helper class to hold the contents of a DataArray without making copies.
    PlotArray can be sliced to provide a PlotArrayView onto the contents of the
    PlotArray.
    """
    def __init__(self, data, meta={}, masks={}):
        self.data = data
        self.meta = {str(name): var for name, var in meta.items()}
        self.masks = masks

    def _is_edges(self, var, dim):
        return dict(zip(var.dims, var.shape))[dim] == dict(
            zip(self.data.dims, self.data.shape))[dim] + 1

    def _maybe_slice(self, var, dim, s):
        if dim in var.dims:
            if self._is_edges(var, dim):
                if isinstance(s, int):
                    return var[dim, s:s + 2]
                else:
                    return var[dim, s.start:s.stop + 1]
            else:
                return var[dim, s]
        else:
            return var

    def __getitem__(self, key):
        dim, s = key
        meta = {
            name: self._maybe_slice(self.meta[name], dim, s)
            for name in self.meta
        }
        masks = {
            name: self._maybe_slice(self.masks[name], dim, s)
            for name in self.masks
        }
        return PlotArray(data=self.data[dim, s], meta=meta, masks=masks)
