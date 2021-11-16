# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
from enum import Enum

from ..core import bin as bin_
from ..core import dtype, units
from ..core import broadcast
from ..core import linspace, rebin, get_slice_params, concat, histogram
from ..core import DataArray, DimensionError
from .tools import to_bin_edges


class ResamplingMode(Enum):
    sum = 0
    mean = 1


def _resample(array, mode: ResamplingMode, dim, edges):
    if mode == ResamplingMode.sum:
        return rebin(array, dim, edges)
    if array.dtype == dtype.float64:
        array = array.copy()
    else:
        array = array.astype(dtype.float64)
    # Scale by bin widths, so `rebin` is effectively performing a "mean"
    # operation instead of "sum".
    # TODO
    # Note that it is inefficient to do this repeatedly. Rather than working
    # around that here we should look into supporting an alternative to
    # `rebin` that works on non-counts data
    coord = array.coords[dim]
    width = coord[dim, 1:] - coord[dim, :-1]
    width.unit = units.one
    array.data *= width
    unit = array.unit
    array.unit = units.counts
    array = rebin(array, dim, edges)
    width = edges[dim, 1:] - edges[dim, :-1]
    width.unit = units.one
    array /= width
    array.unit = unit
    return array


class ResamplingModel():
    def __init__(self, array, *, resolution=None, bounds=None):
        """
        Model over a data array providing unified resampling functionality.
        """
        self._mode = None
        self._resolution = {} if resolution is None else resolution
        self._bounds = {} if bounds is None else bounds
        self._resampled = None
        self._resampled_params = None
        self._array = None
        self._home = None
        self._home_params = None
        self.update_array(array)

    @property
    def mode(self):
        return self._mode

    @mode.setter
    def mode(self, m: ResamplingMode):
        self._mode = ResamplingMode(m)
        self.reset()

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

    def _rebin(self, var, coords, *, sanitize=False):
        """
        Rebin a variable with given coords to new edges.

        The `sanitize` parameter is an unfortunate artifact from mixing binned and
        dense data in the case of masks of binned data. The called to _with_edges
        below is done during setup of ResamplingDenseModel but not
        ResamplingBinnedModel. A proper solution would likely include support for
        non-edge inputs in `rebin`, or rather a function with similar constraits
        such as `bin`, but for dense data.
        """
        data = var
        # In case of masks, we need to preprocess to potentially broadcast `var` (the
        # mask being resampled) to take into account multi-dimensional coords: A mask
        # depending on `dim1` is implicitly "constant" along other dimensions, say
        # `dim2`. If a coord for `dim1` depends on `dim2`, resampling must result in
        # an output mask depending on `dim1` as well as `dim2`. We need to broadcast
        # the input along `dim2` to accomplish this.
        for dim in coords:
            coord = coords[dim]
            data_dims = set(data.dims)
            coord_dims = set(coord.dims)
            if dim in coord_dims and not data_dims.isdisjoint(coord_dims):
                for d in coord_dims:
                    if d not in data_dims:
                        data = broadcast(data,
                                         dims=[d] + data.dims,
                                         shape=[coord.sizes[d]] + data.shape)
        array = DataArray(data=data)
        for dim in coords:
            try:
                array.coords[dim] = coords[dim]
            except DimensionError:  # Masks may be lower-dimensional
                pass
        if sanitize:
            array, _ = _with_edges(array)
        plan = []
        for edge in self.edges:
            dim = edge.dims[-1]
            if dim not in data.dims:
                continue
            coord = array.meta[dim]
            if len(coord.dims) == 1:
                plan.append(edge)
            else:
                plan.insert(0, edge)
        for edge in plan:
            try:
                array = _resample(array, self.mode, dim, edge)
            except RuntimeError:  # Limitation of rebin for slice of inner dim
                array = _resample(array.copy(), self.mode, edge.dims[-1], edge)
        return array

    def _make_edges(self, params):
        edges = []
        for dim, par in params.items():
            if isinstance(par, int):
                continue
            low, high, unit, res = par
            if res is None:
                res = 1
            edges.append(linspace(dim=dim, unit=unit, start=low, stop=high,
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
                low = coord[dim, 0].min().value
                high = coord[dim, -1].max().value
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
                        out = out[get_slice_params(out.data, out.meta[dim], low, high)]
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
                self._resampled.masks[name] = self._rebin(
                    mask, out.meta, sanitize=isinstance(self,
                                                        ResamplingBinnedModel)).data
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

    def _strip_masks(self, array):
        array = array.copy(deep=False)
        for name in list(array.masks.keys()):
            del array.masks[name]
        return array

    def _resample(self, array):
        # We could bin with all edges and then use `bins.sum()` but especially
        # for inputs with many bins handling the final edges using `histogram`
        # is faster with the current implementation of `sc.bin`.
        # To avoid excessive memory use by `bin` we have to ensure that the
        # dimension handled by `histogram` is not a dimension with many bins.
        # We therefore select the smallest dimension of the input array for
        # handling with `histogram`.
        sizes = {}
        for edges in self.edges:
            dim = edges.dims[-1]
            sizes[dim] = array.sizes[dim]
        dim = min(sizes, key=sizes.get)
        # `bin` applies masks, but later we add rebinned masks. This would be
        # inconsistent with how dense data is handled, where data is preserved
        # even if masked, but masks grow. Therefore, we remove masks here. They
        # get handled in _call_resample.
        array = self._strip_masks(array)
        if dim in array.bins.coords:
            index = list(sizes.keys()).index(dim)
            edges = self.edges[index]
            # Must specify bounds for final dim despite handling by `histogram`
            # below: If coord is ragged binning would throw otherwise.
            bounds = concat([edges[dim, 0], edges[dim, -1]], dim)
            binned = bin_(
                array,
                edges=[bounds if i == index else e for i, e in enumerate(self.edges)])
            # TODO Use histogramming with "mean" mode once implemented
            if self.mode == ResamplingMode.mean:
                return bin_(binned, edges=[edges]).bins.mean()
            else:
                return histogram(binned, bins=edges)
        elif self.mode == ResamplingMode.mean:
            return bin_(array, edges=self.edges).bins.mean()
        else:
            return bin_(array, edges=self.edges).bins.sum()


def _with_edges(array):
    new_array = array.copy(deep=False)
    prefix = ''.join(array.dims)
    for dim, var in array.coords.items():
        if dim not in array.dims:
            continue
        new_array.coords[f'{prefix}_{dim}'] = var
        if var.sizes[dim] == array.sizes[dim]:
            new_array.coords[dim] = to_bin_edges(var, dim)
        elif var.dtype not in [dtype.float32, dtype.float64]:
            # rebin does not support int coords right now
            new_array.coords[dim] = var.astype(dtype.float64, copy=False)
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
