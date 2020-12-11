# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock

import numpy as np
import scipp as sc

from .helpers import PlotArray


class NonOwningDataArray():
    def __init__(self, data, coords):
        self._data = data
        self._coords = coords

    @property
    def data(self):
        return self._data

    @property
    def coords(self):
        return self._coords

    def __getitem__(self, key):
        def maybe_slice(var, dim, s):
            if isinstance(s, int):
                start = s
                stop = s + 2
            else:
                start = s.start
                stop = s.stop + 1
            if dim in var.dims:
                return var[dim, start:stop]
            else:
                return var

        dim, s = key
        coords = {
            name: maybe_slice(self.coords[name], dim, s)
            for name in self.coords
        }
        return NonOwningDataArray(data=self._data[dim, s], coords=coords)


class ResamplingModel():
    def __init__(self, data, resolution={}, bounds={}):
        self._resolution = resolution
        self._bounds = bounds
        self._resampled = None
        self._resampled_params = None
        self._array = data
        #PlotArray(
        #    data=data,
        #    coords=dict(
        #        zip([str(dim) for dim in coords.keys()], coords.values())))

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
        for edge in self.edges:
            dim = edge.dims[-1]
            if not dim in var.dims:
                continue
            coord = coords[dim]
            try:
                var = sc.rebin_with_coord(var, coord, edge)
            except RuntimeError:  # Limitation of rebin for slice of inner dim
                var = sc.rebin_with_coord(var.copy(), coord, edge)
        return var

    def _make_edges(self, params):
        edges = []
        for dim, par in params.items():
            if isinstance(par, int):
                continue
            low, high, res = par
            edges.append(
                sc.Variable(dims=[dim],
                            values=np.linspace(low, high, num=res + 1)))
        #print(edges)
        return edges

    def _call_resample(self):
        #print(self.bounds)
        out = self._array
        params = {}
        for dim, s in self.bounds.items():
            if s is None:
                low = self._array.coords[dim].values[0]
                high = self._array.coords[dim].values[-1]
                params[dim] = (low, high, self.resolution[dim])
            elif isinstance(s, int):
                out = out[dim, s]
                params[dim] = s
            else:
                low, high = s
                if isinstance(low, int):
                    out = out[dim, low:high]
                    low = out.coords[dim][dim, 0]
                    high = out.coords[dim][dim, -1]
                    # TODO 2d coord handling?
                    params[dim] = (low.value, high.value, self.resolution[dim])
                else:
                    params[dim] = (low.value, high.value, self.resolution[dim])
                    out = out[sc.get_slice_params(out.data, out.coords[dim],
                                                  low, high)]
        if self._resampled is None or params != self._resampled_params:
            self._resampled_params = params
            self._edges = self._make_edges(params)
            self._resampled = self._resample(out)


class ResamplingBinnedModel(ResamplingModel):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def _resample(self, array):
        # TODO Note that this approach only works for binned data containing
        # all required event coords. This excludes grouped data. Must use
        # something based on groupby in this case?
        a = sc.bin_with_coords(array.data, array.coords, self.edges,
                               []).bins.sum()
        for name, mask in array.masks.items():
            a.masks[name] = self._rebin(mask, array.coords)
        return a


class ResamplingDenseModel(ResamplingModel):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def _resample(self, array):
        return sc.DataArray(
            data=_rebin(array.data, array.coords),
            coords={edge.dims[-1]: edge
                    for edge in self.edges},
            masks={
                name: _rebin(mask, array.coords)
                for name, mask in array.masks.items()
            })


def resampling_model(data, **kwargs):
    if data.data.bins is None:
        return ResamplingDenseModel(data, **kwargs)
    else:
        return ResamplingBinnedModel(data, **kwargs)
