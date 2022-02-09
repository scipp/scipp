# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
import numpy as np
import scipp as sc


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
        self.da = sc.bin(binned, edges=[y])
        self.da_transposed = self.da.transpose().copy()

    def time_outer(self, nbin):
        x = sc.linspace(dim='x', start=0, stop=1, num=nbin - 1, unit='m')
        sc.bin(self.da, edges=[x])

    def time_outer_transposed(self, nbin):
        x = sc.linspace(dim='x', start=0, stop=1, num=nbin - 1, unit='m')
        sc.bin(self.da.transpose(), edges=[x])

    def time_outer_transposed_copied(self, nbin):
        x = sc.linspace(dim='x', start=0, stop=1, num=nbin - 1, unit='m')
        sc.bin(self.da_transposed, edges=[x])


class Bin1d:
    """
    Benchmark sc.bin, for 1d binning.
    """
    def setup(self):
        self.table = sc.data.table_xyz(100_000_000)
        self.x_coarse = sc.linspace(dim='x', start=0, stop=1, num=2**8, unit='m')
        self.x_fine = sc.linspace(dim='x', start=0, stop=1, num=2**16, unit='m')

    def time_coarse(self):
        sc.bin(self.table, edges=[self.x_coarse])

    def time_fine(self):
        sc.bin(self.table, edges=[self.x_fine])

    def time_coarse_then_fine(self):
        """Iteratore coarse -> fine binning which may be faster than direct fine binning
        """
        coarse = sc.bin(self.table, edges=[self.x_coarse])
        sc.bin(coarse, edges=[self.x_fine])
