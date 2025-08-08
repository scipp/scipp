# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# mypy: disable-error-code="union-attr"
from typing import ClassVar

import numpy as np

import scipp as sc
from scipp.binning import make_binned


class Binned1d:
    """
    Benchmark different aspects of binned data
    """

    params: ClassVar[tuple[list[int], list[int]]] = (
        list(10 ** np.arange(8)),
        list(10 ** np.arange(0, 7)),
    )
    param_names: ClassVar[list[str]] = ['nevent', 'nbin']

    def setup(self, nevent: int, nbin: int) -> None:
        self.da = sc.data.binned_x(nevent, nbin)

    def time_bins_constituents(self, nevent: int, nbin: int) -> None:
        self.da.bins.constituents  # noqa: B018

    def time_bins_size(self, nevent: int, nbin: int) -> None:
        self.da.bins.size()

    def time_bins_sum(self, nevent: int, nbin: int) -> None:
        self.da.bins.sum()

    def time_bins_mean(self, nevent: int, nbin: int) -> None:
        self.da.bins.sum()

    def time_bins_concat(self, nevent: int, nbin: int) -> None:
        self.da.bins.concat('x')


class Binned2dConcat:
    """
    Benchmark reduction with 'concat' for 2d binned data
    """

    params: ClassVar[tuple[list[int]]] = ([1, 2, 4, 8, 16, 32, 64, 128],)
    param_names: ClassVar[list[str]] = ['nbin']
    timeout: ClassVar[float] = 300.0

    def setup(self, nbin: int) -> None:
        nx = 100000
        binned = sc.data.binned_x(nevent=2 * nx, nbin=nx)
        y = sc.linspace(dim='y', start=0, stop=1, num=nbin + 1, unit='m')
        self.da = make_binned(binned, edges=[y])

    def time_bins_concat(self, nbin: int) -> None:
        self.da.bins.concat('x')


class Binned2dConcatInner:
    """
    Benchmark reduction with 'concat' along inner for 2d binned data
    """

    params: ClassVar[tuple[list[int]]] = (list(2 ** np.arange(10, 16)),)
    param_names: ClassVar[list[str]] = ['nbin']
    timeout: ClassVar[float] = 300.0

    def setup(self, nbin: int) -> None:
        binned = sc.data.binned_x(nevent=2 * nbin, nbin=nbin)
        y = sc.linspace(dim='y', start=0, stop=1, num=2, unit='m')
        self.da = make_binned(binned, edges=[y])

    def time_bins_concat_long_outer(self, nbin: int) -> None:
        self.da.bins.concat('y')


class Lookup:
    """
    Benchmark map operations using sc.lookup()
    """

    def setup(self) -> None:
        self.binned_x = sc.data.binned_x(100_000_000, 10000).bins.coords['x']
        x = sc.linspace(dim='x', start=0.0, stop=1.0, num=1_000_001, unit='m')
        groups = sc.arange(dim='x', start=0, stop=1_000_000) // 1000 % 5
        self.hist_bool = sc.DataArray(data=groups.astype('bool'), coords={'x': x})
        self.hist_int = sc.DataArray(data=groups, coords={'x': x})
        self.hist_float = sc.DataArray(data=groups.astype('float64'), coords={'x': x})

    def time_create_bool(self) -> None:
        self.hist_bool.coords['x'].values[-1] *= 1.1
        sc.lookup(self.hist_bool, 'x')

    def time_create_float64(self) -> None:
        self.hist_float.coords['x'].values[-1] *= 1.1
        sc.lookup(self.hist_float, 'x')

    def time_create_int64(self) -> None:
        self.hist_int.coords['x'].values[-1] *= 1.1
        sc.lookup(self.hist_int, 'x')

    def time_map_bool(self) -> None:
        self.hist_float.coords['x'].values[-1] *= 1.1
        sc.lookup(self.hist_bool, 'x')[self.binned_x]

    def time_map_float64(self) -> None:
        self.hist_float.coords['x'].values[-1] *= 1.1
        sc.lookup(self.hist_float, 'x')[self.binned_x]

    def time_map_int64(self) -> None:
        self.hist_int.coords['x'].values[-1] *= 1.1
        sc.lookup(self.hist_int, 'x')[self.binned_x]

    def time_map_linspace_bool(self) -> None:
        sc.lookup(self.hist_bool, 'x')[self.binned_x]

    def time_map_linspace_int64(self) -> None:
        sc.lookup(self.hist_int, 'x')[self.binned_x]

    def time_map_linspace_float64(self) -> None:
        sc.lookup(self.hist_float, 'x')[self.binned_x]
