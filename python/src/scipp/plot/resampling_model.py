import numpy as np
import scipp as sc


class ResamplingModel():
    def __init__(self, data, resolution={}, bounds={}):
        self._data = data
        self._resolution = resolution
        self._bounds = bounds
        self._resampled = None
        self._resampled_params = None

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
        self._resample()
        return self._resampled

    def _resample(self):
        out = self._data
        edges = []
        params = {}
        for dim, s in self.bounds.items():
            if isinstance(s, int):
                out = out[dim, s]
                params[dim] = s
            else:
                low, high = s
                params[dim] = (low.value, high.value, self.resolution[dim])
                out = out[dim, low:high]
                edges.append(
                    sc.Variable(dims=[dim],
                                values=np.linspace(low.value,
                                                   high.value,
                                                   num=self.resolution[dim] +
                                                   1)))
        if self._resampled is None or params != self._resampled_params:
            self._resampled_params = params
            self._resampled = sc.bin(out.bins, edges=edges).bins.sum()
