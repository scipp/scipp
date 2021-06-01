# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet
import numpy as np
import scipp as sc


def make_dense_data_array(ndim=1,
                          variances=False,
                          binedges=False,
                          labels=False,
                          masks=False,
                          attrs=False,
                          ragged=False,
                          dims=None,
                          dtype=sc.dtype.float64,
                          unit='counts'):

    dim_list = ['x', 'y', 'z', 'time', 'temperature']
    units = dict(zip(dim_list, ['m', 'm', 'm', 's', 'K']))

    shapes = np.arange(50, 0, -10)[:ndim]
    if dims is None:
        dims = dim_list[:ndim][::-1]

    a = 10.0 * np.sin(
        np.arange(np.prod(shapes), dtype=np.float64).reshape(*shapes))

    data = sc.Variable(dims, values=a, unit=unit, dtype=dtype)
    if variances:
        data.variances = np.abs(np.random.normal(a * 0.1, 0.05))

    coord_dict = {
        dims[i]: sc.arange(dims[i],
                           shapes[i] + binedges,
                           unit=units[dims[i]],
                           dtype=np.float64)
        for i in range(ndim)
    }
    attr_dict = {}
    mask_dict = {}

    if labels:
        coord_dict["somelabels"] = sc.linspace(dims[0],
                                               101.,
                                               105.,
                                               shapes[0],
                                               unit='s')
    if attrs:
        attr_dict["attr"] = sc.linspace(dims[0], 10., 77., shapes[0], unit='s')
    if masks:
        mask_dict["mask"] = sc.Variable(dims,
                                        values=np.where(a > 0, True, False))

    if ragged:
        grid = []
        for i, dim in enumerate(dims):
            if binedges and (i < ndim - 1):
                grid.append(coord_dict[dim].values[:-1])
            else:
                grid.append(coord_dict[dim].values)
        mesh = np.meshgrid(*grid, indexing="ij")
        coord_dict[dims[-1]] = sc.Variable(dims,
                                           values=mesh[-1] +
                                           np.indices(mesh[-1].shape)[0])
    return sc.DataArray(data=data,
                        coords=coord_dict,
                        attrs=attr_dict,
                        masks=mask_dict)


def make_dense_dataset(entries=['a', 'b'], **kwargs):

    ds = sc.Dataset()
    for entry in entries:
        ds[entry] = make_dense_data_array(**kwargs)
        ds[entry] *= np.random.rand()
    return ds


def make_binned_data_array(ndim=1, variances=False, masks=False):

    dim_list = ['x', 'y', 'z', 'time', 'temperature']

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
            values=(np.indices(binned.shape).sum(axis=0) % 2).astype(bool))

    return binned
