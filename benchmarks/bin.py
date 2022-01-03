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
