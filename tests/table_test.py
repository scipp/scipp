# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet
import numpy as np
import scipp as sc


def test_dataset_with_1d_data():
    d = sc.Dataset()
    N = 10
    d['Counts'] = sc.Variable(dims=['tof'], values=10.0 * np.random.rand(N))
    d.coords['tof'] = sc.Variable(dims=['tof'], values=np.arange(N).astype(np.float64))
    sc.table(d)


def test_dataset_with_1d_data_with_bin_edges():
    d = sc.Dataset()
    N = 10
    d['Counts'] = sc.Variable(dims=['row'], values=10.0 * np.random.rand(N))
    d.coords['row'] = sc.Variable(dims=['row'],
                                  values=np.arange(N + 1).astype(np.float64))
    sc.table(d)


def test_dataset_with_1d_data_with_variances():
    d = sc.Dataset()
    N = 10
    d['Counts'] = sc.Variable(dims=['tof'],
                              values=10.0 * np.random.rand(N),
                              variances=np.random.rand(N))
    d.coords['tof'] = sc.Variable(dims=['tof'], values=np.arange(N).astype(np.float64))
    sc.table(d)


def test_dataset_with_1d_data_with_coord_variances():
    d = sc.Dataset()
    N = 10
    d['Counts'] = sc.Variable(dims=['tof'],
                              values=10.0 * np.random.rand(N),
                              variances=np.random.rand(N))
    d.coords['tof'] = sc.Variable(dims=['tof'],
                                  values=np.arange(N).astype(np.float64),
                                  variances=0.1 * np.random.rand(N))
    sc.table(d)


def test_dataset_with_1d_data_with_units():
    d = sc.Dataset()
    N = 10
    d['Sample'] = sc.Variable(dims=['tof'],
                              values=10.0 * np.random.rand(N),
                              unit=sc.units.m,
                              variances=np.random.rand(N))
    d.coords['tof'] = sc.Variable(dims=['tof'],
                                  values=np.arange(N).astype(np.float64),
                                  unit=sc.units.us,
                                  variances=0.1 * np.random.rand(N))
    sc.table(d)


def test_dataset_with_0d_data():
    d = sc.Dataset()
    d['Scalar'] = sc.scalar(1.2)
    sc.table(d)


def test_dataset_with_everything():
    d = sc.Dataset()
    N = 10
    d['Counts'] = sc.Variable(dims=['tof'], values=10.0 * np.random.rand(N))
    d['Sample'] = sc.Variable(dims=['tof'],
                              values=10.0 * np.random.rand(N),
                              unit=sc.units.m,
                              variances=np.random.rand(N))
    d['Scalar'] = sc.scalar(1.2)
    d.coords['tof'] = sc.Variable(dims=['tof'],
                                  values=np.arange(N).astype(np.float64),
                                  unit=sc.units.us,
                                  variances=0.1 * np.random.rand(N))
    sc.table(d)


def test_variable():
    N = 10
    v = sc.Variable(dims=['tof'],
                    values=np.arange(N).astype(np.float64),
                    unit=sc.units.us,
                    variances=0.1 * np.random.rand(N))
    sc.table(v)


def test_dataset_with_coords_only():
    N = 10
    d = sc.Dataset(data={},
                   coords={
                       'tof':
                       sc.Variable(dims=['tof'],
                                   values=np.arange(N).astype(np.float64),
                                   variances=0.1 * np.random.rand(N))
                   })
    sc.table(d)


def test_dataset_with_non_dimensional_coord():
    d = sc.Dataset()
    N = 10
    d['Counts'] = sc.Variable(dims=['tof'],
                              values=10.0 * np.random.rand(N),
                              variances=np.random.rand(N))
    d.coords['tof'] = sc.Variable(dims=['tof'],
                                  values=np.arange(N).astype(np.float64),
                                  variances=0.1 * np.random.rand(N))
    d.coords["Normalized"] = sc.Variable(dims=['tof'], values=np.arange(N))
    sc.table(d)


def test_dataset_with_masks():
    d = sc.Dataset()
    N = 10
    d['Counts'] = sc.Variable(dims=['tof'],
                              values=10.0 * np.random.rand(N),
                              variances=np.random.rand(N))
    d['Counts'].masks["Mask"] = sc.Variable(dims=['tof'],
                                            values=np.zeros(N, dtype=bool))
    d.coords['tof'] = sc.Variable(dims=['tof'],
                                  values=np.arange(N).astype(np.float64),
                                  variances=0.1 * np.random.rand(N))
    d.coords["Normalized"] = sc.Variable(dims=['tof'], values=np.arange(N))

    sc.table(d)


def test_dataset_histogram_with_masks():
    N = 10

    d = sc.Dataset(data={
        "Counts":
        sc.Variable(dims=['x'],
                    values=10.0 * np.random.rand(N),
                    variances=np.random.rand(N))
    },
                   coords={'x': sc.Variable(dims=['x'], values=np.arange(N + 1))})
    d.coords["Normalized"] = sc.Variable(dims=['x'], values=np.arange(N))

    d['Counts'].masks["Mask"] = sc.Variable(dims=['x'], values=np.zeros(N, dtype=bool))

    sc.table(d)


def test_display_when_only_non_dim_coords_is_bin_edges():
    da = sc.DataArray(
        coords={'lab': sc.Variable(dims=['x'], values=np.arange(11), unit=sc.units.m)},
        data=sc.Variable(dims=['x'], values=np.random.random(10), unit=sc.units.counts))
    sc.table(da)


def test_display_when_only_attr_is_bin_edges():
    da = sc.DataArray(
        attrs={'attr0': sc.Variable(dims=['x'], values=np.arange(11), unit=sc.units.m)},
        data=sc.Variable(dims=['x'], values=np.random.random(10), unit=sc.units.counts))
    sc.table(da)


def test_display_when_largest_coord_non_dimensional():
    da = sc.DataArray(coords={
        'x':
        sc.Variable(dims=['x'], values=np.arange(10), unit=sc.units.m),
        'lab':
        sc.Variable(dims=['x'], values=np.arange(11), unit=sc.units.m)
    },
                      data=sc.Variable(dims=['x'],
                                       values=np.random.random(10),
                                       unit=sc.units.counts))
    sc.table(da)


def test_with_scalar_variable():
    sc.table(sc.scalar(value=1.0))


def test_with_scalar_dataarray():
    x = sc.array(dims=['x'], values=[0, 1])
    sc.table(sc.DataArray(data=sc.scalar(value=1.0), coords={'x': x}))


def test_with_scalar_dataset():
    ds = sc.Dataset(data={'a': sc.scalar(value=1.0), 'b': sc.scalar(value=2.0)})
    sc.table(ds)
