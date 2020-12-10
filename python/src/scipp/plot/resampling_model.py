import numpy as np
import scipp as sc


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
    def __init__(self, data, coords, resolution={}, bounds={}):
        self._resolution = resolution
        self._bounds = bounds
        self._resampled = None
        self._resampled_params = None
        self._array = NonOwningDataArray(
            data=data,
            coords=dict(
                zip([str(dim) for dim in coords.keys()], coords.values())))

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

    def _make_edges(self, params):
        edges = []
        for dim, par in params.items():
            if isinstance(par, int):
                continue
            low, high, res = par
            edges.append(
                sc.Variable(dims=[dim],
                            values=np.linspace(low, high, num=res + 1)))
        return edges

    def _call_resample(self):
        out = self._array
        params = {}
        for dim, s in self.bounds.items():
            if isinstance(s, int):
                out = out[dim, s]
                params[dim] = s
            else:
                low, high = s
                params[dim] = (low.value, high.value, self.resolution[dim])
                out = out[sc.get_slice_params(out.data, out.coords[dim], low,
                                              high)]
        if self._resampled is None or params != self._resampled_params:
            self._resampled_params = params
            self._resampled = self._resample(out, self._make_edges(params))


class ResamplingBinnedModel(ResamplingModel):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def _resample(self, array, edges):
        # TODO Note that this approach only works for binned data containing
        # all required event coords. This excludes grouped data. Must use
        # something based on groupby in this case?
        return sc.bin_with_coords(array.data, array.coords, edges,
                                  []).bins.sum()


class ResamplingDenseModel(ResamplingModel):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def _resample(self, array, edges):
        out = array.data
        for edge in edges:
            dim = edge.dims[-1]
            coord = array.coords[dim]
            try:
                out = sc.rebin_with_coord(out, coord, edge)
            except RuntimeError:  # Limitation of rebin for slice of inner dim
                out = sc.rebin_with_coord(out.copy(), coord, edge)
        return sc.DataArray(data=out,
                            coords={edge.dims[-1]: edge
                                    for edge in edges})


def resampling_model(data, **kwargs):
    if data.bins is None:
        return ResamplingDenseModel(data, **kwargs)
    else:
        return ResamplingBinnedModel(data, **kwargs)
