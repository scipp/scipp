# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
import numpy as np
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
