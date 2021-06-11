# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet
import numpy as np
import scipp as sc

dim_list = ['xx', 'y', 'z', 'time', 'temperature']


def make_scalar(variance=False, dtype=sc.dtype.float64, unit='counts'):
    var = sc.scalar(10.0 * np.random.rand(), unit=unit, dtype=dtype)
    if variance:
        var.variance = np.random.rand()
    return var


def make_variable(ndim=1,
                  variances=False,
                  dims=None,
                  dtype=sc.dtype.float64,
                  unit='counts'):

    shapes = np.arange(50, 0, -10)[:ndim]
    if dims is None:
        dims = dim_list[:ndim][::-1]

    a = 10.0 * np.sin(
        np.arange(np.prod(shapes), dtype=np.float64).reshape(*shapes))

    print(dims, a, unit, dtype)
    var = sc.array(dims=dims, values=a, unit=unit, dtype=dtype)
    if variances:
        var.variances = np.abs(np.random.normal(a * 0.1, 0.05))

    return var


def make_scalar_array(variance=False,
                      label=False,
                      mask=False,
                      attr=False,
                      dtype=sc.dtype.float64,
                      unit='counts'):

    data = make_scalar(variance=variance, dtype=dtype, unit=unit)

    coord_dict = {'xx': make_scalar(dtype=dtype, unit=unit)}
    attr_dict = {}
    mask_dict = {}

    if label:
        coord_dict["lab"] = make_scalar(dtype=dtype, unit=unit)
    if attr:
        attr_dict["attr"] = make_scalar(dtype=dtype, unit=unit)
    if mask:
        mask_dict["mask"] = make_scalar(dtype=dtype, unit=unit)

    return sc.DataArray(data=data,
                        coords=coord_dict,
                        attrs=attr_dict,
                        masks=mask_dict)


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

    coord_units = dict(zip(dim_list, ['m', 'm', 'm', 's', 'K']))

    data = make_variable(ndim=ndim,
                         variances=variances,
                         dims=dims,
                         dtype=dtype,
                         unit=unit)

    coord_dict = {
        data.dims[i]: sc.arange(data.dims[i],
                                data.shape[i] + binedges,
                                unit=coord_units[data.dims[i]],
                                dtype=np.float64)
        for i in range(ndim)
    }
    attr_dict = {}
    mask_dict = {}

    if labels:
        coord_dict["lab"] = sc.linspace(data.dims[0],
                                        101.,
                                        105.,
                                        data.shape[0],
                                        unit='s')
    if attrs:
        attr_dict["attr"] = sc.linspace(data.dims[0],
                                        10.,
                                        77.,
                                        data.shape[0],
                                        unit='s')
    if masks:
        mask_dict["mask"] = sc.Variable(data.dims,
                                        values=np.where(
                                            data.values > 0, True, False))

    if ragged:
        grid = []
        for i, dim in enumerate(data.dims):
            if binedges and (i < ndim - 1):
                grid.append(coord_dict[dim].values[:-1])
            else:
                grid.append(coord_dict[dim].values)
        mesh = np.meshgrid(*grid, indexing="ij")
        coord_dict[data.dims[-1]] = sc.Variable(data.dims,
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
        ds[entry] *= int(10.0 * np.random.rand())
    return ds


def make_binned_data_array(ndim=1, variances=False, masks=False):

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
