# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
import numpy as np
from numpy.random import default_rng
import scipp as sc
from scipp.binning import make_binned


class Bin2dRebinLongDim:
    """
    Benchmark sc.bin, changing existing long dimension
    """
    params = (list(2**np.arange(10, 16)), )
    param_names = ['nbin']
    timeout = 300.0

    def setup(self, nbin):
        binned = sc.data.binned_x(nevent=2 * nbin, nbin=nbin)
        y = sc.linspace(dim='y', start=0, stop=1, num=2, unit='m')
        self.da = make_binned(binned, edges=[y])
        self.da_transposed = self.da.transpose().copy()

    def time_outer(self, nbin):
        x = sc.linspace(dim='x', start=0, stop=1, num=nbin - 1, unit='m')
        make_binned(self.da, edges=[x])

    def time_outer_transposed(self, nbin):
        x = sc.linspace(dim='x', start=0, stop=1, num=nbin - 1, unit='m')
        make_binned(self.da.transpose(), edges=[x])

    def time_outer_transposed_copied(self, nbin):
        x = sc.linspace(dim='x', start=0, stop=1, num=nbin - 1, unit='m')
        make_binned(self.da_transposed, edges=[x])


class Bin1d:
    """
    Benchmark sc.bin, for 1d binning.
    """
    params = (list(2**np.arange(0, 20)), )
    param_names = ['nbin']
    timeout = 300.0

    def setup(self, nbin):
        self.table = sc.data.table_xyz(50_000_000)
        self.x = sc.linspace(dim='x', start=0, stop=1, num=nbin + 1, unit='m')

    def time_bin_table(self, nbin):
        make_binned(self.table, edges=[self.x])


class BinMapToNewDim:
    """
    Benchmark make_binned, "mapping" existing dim(s) to new dim(s).
    """
    params = (list(2**np.arange(10, 18)), list(2**np.arange(2, 12)))
    param_names = ['n_initial', 'n_final']
    timeout = 300.0

    def setup(self, n_initial, n_final):
        rng = default_rng(seed=1234)
        table = sc.data.table_xyz(1_000_000)
        self.da_x = table.bin(x=n_initial)
        self.da_xy = table.bin(x=n_initial // 64, y=64)
        self.param_1 = sc.array(dims=['x'], values=rng.random((n_initial, )))
        self.param_2 = sc.array(dims=['x'], values=rng.random((n_initial, )))
        self.edges = sc.arange('param', n_final + 1, dtype='float64')
        self.edges_1 = sc.arange('param_1', n_final // 4 + 1, dtype='float64')
        self.edges_2 = sc.arange('param_2', 4 + 1, dtype='float64')

    def time_bin_with_just_bin_coord_1d_to_1d(self, n_initial, n_final):
        da = self.da_x.copy(deep=False)
        da.coords['param'] = self.param_1
        make_binned(da, edges=[self.edges], erase=['x'])

    def time_bin_with_just_bin_coord_2d_to_1d(self, n_initial, n_final):
        da = self.da_xy.copy(deep=False)
        da.coords['param'] = self.param_1.fold('x', sizes=da.sizes)
        make_binned(da, edges=[self.edges], erase=['x'])

    def time_bin_with_just_bin_coord_1d_to_2d(self, n_initial, n_final):
        da = self.da_x.copy(deep=False)
        da.coords['param_1'] = self.param_1
        da.coords['param_2'] = self.param_2
        make_binned(da, edges=[self.edges_1, self.edges_2], erase=['x'])
