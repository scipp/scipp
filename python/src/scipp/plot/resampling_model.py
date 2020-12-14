# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock

import numpy as np
from .._scipp import core as sc
from .helpers import PlotArray


class ResamplingModel():
    def __init__(self, data, resolution=None, bounds=None):
        self._resolution = {} if resolution is None else resolution
        self._bounds = {} if bounds is None else bounds
        self._resampled = None
        self._resampled_params = None
        self._array = data
        self._home = None
        self._home_params = None

    @property
    def resolution(self):
        return self._resolution

    @resolution.setter
    def resolution(self, res):
        self._resolution = res

    @property
    def bounds(self):
        return self._bounds

    @bounds.setter
    def bounds(self, bnds):
        self._bounds = bnds

    @property
    def data(self):
        self._call_resample()
        return self._resampled

    @property
    def edges(self):
        return self._edges

    def _rebin(self, var, coords):
        plan = []
        for edge in self.edges:
            dim = edge.dims[-1]
            if dim not in var.dims:
                continue
            coord = coords[dim]
            if len(coord.dims) == 1:
                plan.append((coord, edge))
            else:
                plan.insert(0, (coord, edge))
        for coord, edge in plan:
            try:
                var = sc.rebin(var, dim, coord, edge)
            except RuntimeError:  # Limitation of rebin for slice of inner dim
                var = sc.rebin(var.copy(), edge.dims[-1], coord, edge)
        return var

    def _make_edges(self, params):
        edges = []
        for dim, par in params.items():
            if isinstance(par, int):
                continue
            low, high, unit, res = par
            edges.append(
                sc.Variable(dims=[dim],
                            unit=unit,
                            values=np.linspace(low, high, num=res + 1)))
        return edges

    def _call_resample(self):
        out = self._array
        params = {}
        for dim, s in self.bounds.items():
            dim = str(dim)
            if s is None:
                low = self._array.meta[dim].values[0]
                high = self._array.meta[dim].values[-1]
                params[dim] = (low, high, self.resolution[dim])
            elif isinstance(s, int):
                out = out[dim, s]
                params[dim] = s
            else:
                low, high = s
                if isinstance(low, int):
                    out = out[dim, low:high]
                    low = out.meta[dim][dim, 0]
                    high = out.meta[dim][dim, -1]
                else:
                    # cannot pre-select bins for multi-dim coords
                    if len(out.meta[dim].dims) == 1:
                        out = out[sc.get_slice_params(out.data, out.meta[dim],
                                                      low, high)]
                params[dim] = (low.value, high.value, low.unit,
                               self.resolution[dim])
        if params == self._home_params:
            # This isa a crude caching mechanism for past views. Currently we
            # have the "back" buttons disabled in the matplotlib toolbar, so
            # we cache only the "home" view. This is the most expensive to
            # create anyay.
            self._resampled_params = self._home_params
            self._resampled = self._home
        elif self._resampled is None or params != self._resampled_params:
            self._resampled_params = params
            self._edges = self._make_edges(params)
            self._resampled = self._resample(out)
        if self._home is None:
            self._home = self._resampled
            self._home_params = self._resampled_params


class ResamplingBinnedModel(ResamplingModel):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        # TODO See #1469. This is a temporary hack to work around the
        # conversion of coords to edges in model.py.
        for name in self._array.meta:
            if name == 'spectrum':
                self._array.meta['spectrum'] = 0.5 * (
                    self._array.meta['spectrum']['spectrum', 1:] +
                    self._array.meta['spectrum']['spectrum', :-1])

    def _resample(self, array):
        # We could bin with all edges and then use `bins.sum()` but especially
        # for inputs with many bins handling the final edges using `histogram`
        # is faster with the current implementation of `sc.bin`.
        binned = sc.bin_with_coords(array.data, array.meta, self.edges[:-1],
                                    [])
        a = sc.histogram(binned, self.edges[-1])
        for name, mask in array.masks.items():
            a.masks[name] = self._rebin(mask, array.meta)
        return a


class ResamplingCountsModel(ResamplingModel):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def _resample(self, array):
        return sc.DataArray(
            data=self._rebin(array.data, array.meta),
            coords={edge.dims[-1]: edge
                    for edge in self.edges},
            masks={
                name: self._rebin(mask, array.meta)
                for name, mask in array.masks.items()
            })


class ResamplingDenseModel(ResamplingModel):
    def __init__(self, array, **kwargs):
        # Scale by bin widths, so `rebin` is effectively performing a "mean"
        # operation instead of "sum".
        super().__init__(self._to_density(array), **kwargs)

    def _to_density(self, array):
        array = PlotArray(data=array.data.astype(sc.dtype.float64),
                          meta=array.meta,
                          masks=array.masks)
        for dim in array.data.dims:
            coord = array.meta[dim]
            array.data *= coord[dim, 1:] - coord[dim, :-1]
        array.data.unit = sc.units.one
        return array

    def _from_density(self, data):
        for edge in self.edges:
            dim = edge.dims[-1]
            width = edge[dim, 1:] - edge[dim, :-1]
            width.unit = sc.units.one
            data /= width
        return data

    def _resample(self, array):
        return sc.DataArray(
            data=self._from_density(self._rebin(array.data, array.meta)),
            coords={edge.dims[-1]: edge
                    for edge in self.edges},
            masks={
                name: self._rebin(mask, array.meta)
                for name, mask in array.masks.items()
            })


def resampling_model(array, **kwargs):
    if array.data.bins is None:
        if array.data.unit == sc.units.counts:
            return ResamplingCountsModel(array, **kwargs)
        else:
            return ResamplingDenseModel(array, **kwargs)
    else:
        return ResamplingBinnedModel(array, **kwargs)
