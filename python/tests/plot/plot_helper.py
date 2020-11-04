# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet
import numpy as np
import scipp as sc


def make_dense_dataset(ndim=1,
                       variances=False,
                       binedges=False,
                       labels=False,
                       masks=False):

    dim_list = ['tof', 'x', 'y', 'z', 'Q_x']

    N = 50
    M = 10

    d = sc.Dataset()
    shapes = []
    dims = []
    for i in range(ndim):
        n = N - (i * M)
        d.coords[dim_list[i]] = sc.Variable(
            dims=[dim_list[i]],
            values=np.arange(n + binedges).astype(np.float64))
        dims.append(dim_list[i])
        shapes.append(n)
    a = np.sin(np.arange(np.prod(shapes)).reshape(*shapes).astype(np.float64))
    d["Sample"] = sc.Variable(dims, values=a, unit=sc.units.counts)
    if variances:
        d["Sample"].variances = np.abs(np.random.normal(a * 0.1, 0.05))
    if labels:
        d.coords["somelabels"] = sc.Variable([dim_list[0]],
                                             values=np.linspace(
                                                 101., 105., shapes[0]),
                                             unit=sc.units.s)
    if masks:
        d["Sample"].masks["mask"] = sc.Variable(dims,
                                                values=np.where(
                                                    a > 0, True, False))
    return d
