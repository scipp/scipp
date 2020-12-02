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
                       masks=False,
                       ragged=False,
                       dtype=sc.dtype.float64,
                       unit=sc.units.counts):

    dim_list = ['tof', 'x', 'y', 'z', 'Q_x']

    N = 50
    M = 10

    d = sc.Dataset()
    shapes = []
    dims = []
    for i in range(ndim):
        n = N - (i * M)
        d.coords[dim_list[i]] = sc.Variable(dims=[dim_list[i]],
                                            values=np.arange(n + binedges,
                                                             dtype=np.float64))
        dims.append(dim_list[i])
        shapes.append(n)

    if ragged:
        grid = []
        for i, dim in enumerate(dims):
            if binedges and (i < ndim - 1):
                grid.append(d.coords[dim].values[:-1])
            else:
                grid.append(d.coords[dim].values)
        mesh = np.meshgrid(*grid, indexing="ij")
        d.coords[dims[-1]] = sc.Variable(dims,
                                         values=mesh[-1] +
                                         np.indices(mesh[-1].shape)[0])

    a = np.sin(np.arange(np.prod(shapes)).reshape(*shapes).astype(np.float64))
    d["Sample"] = sc.Variable(dims, values=a, unit=unit, dtype=dtype)
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


def make_binned_data_array(ndim=1, variances=False, masks=False):

    dim_list = ['tof', 'x', 'y', 'z', 'Q_x']

    N = 50
    M = 10

    values = 10.0 * np.random.random(N)

    da = sc.DataArray(data=sc.Variable(dims=['position'],
                                       unit=sc.units.counts,
                                       values=values),
                      coords={
                          'position':
                          sc.Variable(
                              dims=['position'],
                              values=['site-{}'.format(i) for i in range(N)])
                      })

    if variances:
        da.variances = values

    bin_list = []
    for i in range(ndim):
        da.coords[dim_list[i]] = sc.Variable(dims=['position'],
                                             unit=sc.units.m,
                                             values=np.random.random(N))
        bin_list.append(
            sc.Variable(dims=[dim_list[i]],
                        unit=sc.units.m,
                        values=np.linspace(0.1, 0.9, M - i)))

    binned = sc.bin(da, bin_list)

    if masks:
        # Make a checkerboard mask, see https://stackoverflow.com/a/51715491
        binned.masks["mask"] = sc.Variable(
            dims=binned.dims,
            values=(np.indices(binned.shape).sum(axis=0) % 2).astype(np.bool))

    return binned
