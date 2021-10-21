# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
import numpy as np
import scipp as sc


class Binned1d:
    """
    Benchmark different aspects of binned data
    """
    params = (list(10**np.arange(8)), list(10**np.arange(0, 7)))
    param_names = ['nevent', 'nbin']

    def setup(self, nevent, nbin):
        self.da = sc.data.binned_x(nevent, nbin)

    def time_bins_constituents(self, nevent, nbin):
        self.da.bins.constituents

    def time_bins_size(self, nevent, nbin):
        self.da.bins.size()

    def time_bins_sum(self, nevent, nbin):
        self.da.bins.sum()

    def time_bins_mean(self, nevent, nbin):
        self.da.bins.sum()

    def time_bins_concat(self, nevent, nbin):
        self.da.bins.concat('x')
