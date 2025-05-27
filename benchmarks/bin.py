# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
from typing import ClassVar

import numpy as np

import scipp as sc
from scipp.binning import make_binned


class Bin2dRebinLongDim:
    """
    Benchmark sc.bin, changing existing long dimension
    """

    params: ClassVar[tuple[list[int]]] = (list(2 ** np.arange(10, 16)),)
    param_names: ClassVar[list[str]] = ['nbin']
    timeout: ClassVar[float] = 300.0

    def setup(self, nbin: int) -> None:
        binned = sc.data.binned_x(nevent=2 * nbin, nbin=nbin)
        y = sc.linspace(dim='y', start=0, stop=1, num=2, unit='m')
        self.da = make_binned(binned, edges=[y])
        self.da_transposed = self.da.transpose().copy()

    def time_outer(self, nbin: int) -> None:
        x = sc.linspace(dim='x', start=0, stop=1, num=nbin - 1, unit='m')
        make_binned(self.da, edges=[x])

    def time_outer_transposed(self, nbin: int) -> None:
        x = sc.linspace(dim='x', start=0, stop=1, num=nbin - 1, unit='m')
        make_binned(self.da.transpose(), edges=[x])

    def time_outer_transposed_copied(self, nbin: int) -> None:
        x = sc.linspace(dim='x', start=0, stop=1, num=nbin - 1, unit='m')
        make_binned(self.da_transposed, edges=[x])


class Bin1d:
    """
    Benchmark sc.bin, for 1d binning.
    """

    params: ClassVar[tuple[list[int]]] = (list(2 ** np.arange(0, 20)),)
    # For full runs (too large for CI) use:
    # params = (list(2 ** np.arange(0, 25)),)
    param_names: ClassVar[list[str]] = ['nbin']
    timeout: ClassVar[float] = 300.0

    def setup(self, nbin: int) -> None:
        self.table = sc.data.table_xyz(50_000_000)
        self.x = sc.linspace(dim='x', start=0, stop=1, num=nbin + 1, unit='m')

    def time_bin_table(self, nbin: int) -> None:
        make_binned(self.table, edges=[self.x])


class Group1d:
    """
    Benchmark sc.group, for 1d grouping.
    """

    nevent: ClassVar[list[int]] = [1_000, 1_000_000, 10_000_000, 100_000_000]
    ngroup: ClassVar[list[int]] = list(2 ** np.arange(2, 20))
    # For full runs (too large for CI) use:
    # nevent = [1_000, 1_000_000, 10_000_000, 100_000_000]
    # ngroup = list(2 ** np.arange(2, 25))
    params: ClassVar[tuple[list[int], list[int]]] = (nevent, ngroup)
    param_names: ClassVar[list[str]] = ['nevent', 'ngroup']
    timeout: ClassVar[float] = 300.0

    def setup(self, nevent: int, ngroup: int) -> None:
        self.table = sc.data.table_xyz(nevent)
        self.table.coords['group'] = (ngroup * self.table.coords['x']).to(dtype='int64')
        del self.table.coords['x']
        self.contiguous_groups = sc.arange('group', ngroup, unit='m')
        self.groups = sc.arange('group', ngroup, unit='m')
        self.groups.values[0] = 1
        self.groups.values[1] = 0

    def time_group_contiguous(self, nevent: int, ngroup: int) -> None:
        self.table.group(self.contiguous_groups)

    def time_group(self, nevent: int, ngroup: int) -> None:
        self.table.group(self.groups)
