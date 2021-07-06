# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
from .._scipp import core as sc
from .._variable import linspace
from .tools import to_bin_edges


def _resample(array, dim, edges):
    if array.unit == sc.units.counts:
        return sc.rebin(array, dim, edges)
    if array.dtype == sc.dtype.float64:
        array = array.copy()
    else:
        array = array.astype(sc.dtype.float64)
    # Scale by bin widths, so `rebin` is effectively performing a "mean"
    # operation instead of "sum".
    # Note that it is inefficient to do this repeatedly. Rather than working
    # around that here we should look into supporting an alternative to
    # `rebin` that works on non-counts data
    coord = array.coords[dim]
    width = coord[dim, 1:] - coord[dim, :-1]
    width.unit = sc.units.one
    array.data *= width
    unit = array.unit
    array.unit = sc.units.counts
    array = _resample(array, dim, edges)
    width = edges[dim, 1:] - edges[dim, :-1]
    width.unit = sc.units.one
    array /= width
    array.unit = unit
    return array


class ResamplingModel():
    def __init__(self, array, resolution=None, bounds=None):
        self._resolution = {} if resolution is None else resolution
        self._bounds = {} if bounds is None else bounds
        self._resampled = None
        self._resampled_params = None
        self._array = None
        self._home = None
        self._home_params = None
        self.update_array(array)

    @property
    def resolution(self):
        return self._resolution

    @resolution.setter
    def resolution(self, res):
        """
        If bounds for a given dim are provided but not resolution (or if
        resolution is None), the dimension is squeezed out.
        """
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
        array = sc.DataArray(data=var)
        for dim in coords:
            try:
                array.coords[dim] = coords[dim]
            except sc.DimensionError:  # Masks may be lower-dimensional
                pass
        plan = []
        for edge in self.edges:
            dim = edge.dims[-1]
            if dim not in var.dims:
                continue
            coord = array.meta[dim]
            if len(coord.dims) == 1:
                plan.append(edge)
            else:
                plan.insert(0, edge)
        for edge in plan:
            try:
                array = _resample(array, dim, edge)
            except RuntimeError:  # Limitation of rebin for slice of inner dim
                array = _resample(array.copy(), edge.dims[-1], edge)
        return array

    def _make_edges(self, params):
        edges = []
        for i, (dim, par) in enumerate(params.items()):
            if isinstance(par, int):
                continue
            low, high, unit, res = par
            if res is None:
                res = 1
            edges.append(
                linspace(dim=dim, unit=unit, start=low, stop=high,
                         num=res + 1))
            # The order of edges matters. We need to put the length 1 edges
            # first to rebin these dims first and effectively slice them out,
            # otherwise we will rebin N-D variables on high resolution.
            if res == 1:
                edges.insert(0, edges.pop(-1))
        return edges

    def _call_resample(self):
        out = self._array
        params = {}
        for dim, s in self.bounds.items():
            dim = str(dim)
            if s is None:
                coord = self._array.meta[dim]
                # TODO handle flipped coord
                low = sc.min(coord[dim, 0]).value
                high = sc.max(coord[dim, -1]).value
                params[dim] = (low, high, coord.unit, self.resolution[dim])
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
                    # Note we use self._array, not out for selection:
                    # out's coord may have turned 1-d due to prior slicing,
                    # yielding length-0 slice here if limits are outside
                    # range on this slice, which rebin cannot handle.
                    if len(self._array.meta[dim].dims) == 1:
                        out = out[sc.get_slice_params(out.data, out.meta[dim],
                                                      low, high)]
                params[dim] = (low.value, high.value, low.unit,
                               self.resolution.get(dim, None))
        if params == self._home_params:
            # This is a crude caching mechanism for past views. Currently we
            # have the "back" buttons disabled in the matplotlib toolbar, so
            # we cache only the "home" view. This is the most expensive to
            # create anyway.
            self._resampled_params = self._home_params
            self._resampled = self._home
        elif self._resampled is None or params != self._resampled_params:
            self._resampled_params = params
            self._edges = self._make_edges(params)
            self._resampled = self._resample(out)
            for name, mask in out.masks.items():
                self._resampled.masks[name] = self._rebin(mask, out.meta).data
            for dim in params:
                size_one = self._resampled.sizes.get(dim, None) == 1
                no_resolution = self.resolution.get(dim, None) is None
                if size_one and no_resolution:
                    self._resampled = self._resampled[dim, 0]
        if self._home is None:
            self._home = self._resampled
            self._home_params = self._resampled_params

    def update_array(self, array):
        """
        Update the internal array with a new array.
        """
        self._array = self._make_array(array)

    def reset(self):
        """
        Reset view parameters to force a plot update.
        """
        self._home_params = None
        self._resampled_params = None


class ResamplingBinnedModel(ResamplingModel):
    def _make_array(self, array):
        return array

    def _resample(self, array):
        # We could bin with all edges and then use `bins.sum()` but especially
        # for inputs with many bins handling the final edges using `histogram`
        # is faster with the current implementation of `sc.bin`.
        edges = self.edges[-1]
        dim = edges.dims[-1]
        # TODO `bin` applies masks, but later we add rebinned masks. This is
        # inconsistent with how dense data is handled, where data is preserved
        # even if masked, but masks grow. Note that manual masks handling for
        # dense data in _call_resample is thus redundant, if it could be
        # handled consistently here.
        if dim in array.bins.coords:
            # Must specify bounds for final dim despite handling by `histogram`
            # below: If coord is ragged binning would throw otherwise.
            bounds = sc.concatenate(edges[dim, 0], edges[dim, -1], dim)
            binned = sc.bin(array=array, edges=self.edges[:-1] + [bounds])
            return sc.histogram(binned, edges)
        else:
            return sc.bin(array=array, edges=self.edges).bins.sum()


def _with_edges(array):
    new_array = array.copy(deep=False)
    prefix = ''.join(array.dims)
    for dim, var in array.coords.items():
        new_array.coords[f'{prefix}_{dim}'] = var
        if var.sizes[dim] == array.sizes[dim]:
            new_array.coords[dim] = to_bin_edges(var, dim)
        elif var.dtype not in [sc.dtype.float32, sc.dtype.float64]:
            # rebin does not support int coords right now
            # TODO use copy=False
            new_array.coords[dim] = var.astype(sc.dtype.float64)
    return new_array, prefix


def _replace_edge_coords(array, dims, bounds, prefix):
    coords = {}
    for dim in dims:
        if dim in bounds:
            if isinstance(bounds[dim], int):
                coords[dim] = array.meta[f'{prefix}_{dim}']
            else:
                coords[dim] = array.meta[dim]
        else:
            coords[dim] = array.meta[f'{prefix}_{dim}']
    return coords


class ResamplingDenseModel(ResamplingModel):
    def _make_array(self, array):
        array, self._prefix = _with_edges(array)
        return array

    def _resample(self, array):
        coords = _replace_edge_coords(array, self._array.dims, self.bounds,
                                      self._prefix)
        return self._rebin(array.data, coords)


def resampling_model(array, **kwargs):
    if array.data.bins is None:
        return ResamplingDenseModel(array, **kwargs)
    else:
        return ResamplingBinnedModel(array, **kwargs)
