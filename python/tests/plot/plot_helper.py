# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

from itertools import product
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
        d.masks["mask"] = sc.Variable(dims,
                                      values=np.where(a > 0, True, False))
    return d


def make_events_dataset(ndim=1):

    dim_list = ['x', 'y', 'z', 'Q_x']

    N = 50
    M = 10

    dims = []
    shapes = []
    for i in range(ndim):
        n = N - (i * M)
        dims.append(dim_list[i])
        shapes.append(n)

    var = sc.Variable(dims=dims,
                      shape=shapes,
                      unit=sc.units.us,
                      dtype=sc.dtype.event_list_float64)
    dat = sc.Variable(dims=dims,
                      unit=sc.units.counts,
                      values=np.ones(shapes),
                      variances=np.ones(shapes))

    indices = tuple()
    for i in range(ndim):
        indices += range(shapes[i]),
    # Now construct all indices combinations using itertools
    for ind in product(*indices):
        # And for each indices combination, slice the original data
        vslice = var
        for i in range(ndim):
            vslice = vslice[dims[i], ind[i]]
        v = np.random.normal(float(N),
                             scale=2.0 * M,
                             size=int(np.random.rand() * N))
        vslice.values = v

    d = sc.Dataset()
    for i in range(ndim):
        d.coords[dim_list[i]] = sc.Variable([dim_list[i]],
                                            values=np.arange(N - (i * M),
                                                             dtype=np.float),
                                            unit=sc.units.m)
    d["a"] = sc.DataArray(data=dat, coords={'tof': var})
    return d
