# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

import numpy as np
import scipp as sc
from plot_helper import make_dense_dataset
from scipp.plot import plot


def test_plot_projection_3d():
    plot(make_dense_dataset(ndim=3), projection="3d")


def test_plot_projection_3d_with_labels():
    plot(make_dense_dataset(ndim=3, labels=True),
         projection="3d",
         axes={'x': "somelabels"})


def test_plot_projection_3d_with_bin_edges():
    plot(make_dense_dataset(ndim=3, binedges=True), projection="3d")


def test_plot_projection_3d_with_masks():
    plot(make_dense_dataset(ndim=3, masks=True), projection="3d")


def test_plot_projection_3d_with_vectors():
    N = 1000
    M = 100
    theta = np.random.random(N) * np.pi
    phi = np.random.random(N) * 2.0 * np.pi
    r = 10.0 + (np.random.random(N) - 0.5)
    x = r * np.sin(theta) * np.sin(phi)
    y = r * np.sin(theta) * np.cos(phi)
    z = r * np.cos(theta)
    tof = np.arange(M).astype(np.float)
    a = np.arange(M * N).reshape([M, N]) * np.sin(y)
    d = sc.Dataset()
    d.coords['xyz'] = sc.Variable(['xyz'],
                                  values=np.array([x, y, z]).T,
                                  dtype=sc.dtype.vector_3_float64)
    d.coords['tof'] = sc.Variable(['tof'], values=tof)
    d['a'] = sc.Variable(['tof', 'xyz'], values=a)
    plot(d, projection="3d", positions="xyz")


def test_plot_projection_3d_with_vectors_non_dim_coord():
    N = 1000
    M = 100
    theta = np.random.random(N) * np.pi
    phi = np.random.random(N) * 2.0 * np.pi
    r = 10.0 + (np.random.random(N) - 0.5)
    x = r * np.sin(theta) * np.sin(phi)
    y = r * np.sin(theta) * np.cos(phi)
    z = r * np.cos(theta)
    tof = np.arange(M).astype(np.float)
    a = np.arange(M * N).reshape([M, N]) * np.sin(y)
    d = sc.Dataset()
    d.coords['xyz'] = sc.Variable(['xyz'],
                                  values=np.array([x, y, z]).T,
                                  dtype=sc.dtype.vector_3_float64)
    d.coords['pos'] = sc.Variable(['xyz'],
                                  values=np.array([x, y, z]).T + 20.0,
                                  dtype=sc.dtype.vector_3_float64)
    d.coords['tof'] = sc.Variable(['tof'], values=tof)
    d['a'] = sc.Variable(['tof', 'xyz'], values=a)
    plot(d, projection="3d", positions="pos")


def test_plot_variable_3d():
    N = 50
    v3d = sc.Variable(['tof', 'x', 'y'],
                      values=np.random.rand(N, N, N),
                      unit=sc.units.m)
    plot(v3d, projection="3d")


def test_plot_4d_with_masks_projection_3d():
    data = sc.DataArray(data=sc.Variable(
        dims=['pack', 'tube', 'straw', 'pixel'],
        values=np.random.rand(2, 8, 7, 256)),
                        coords={})
    a = np.sin(np.linspace(0, 3.14, num=256))
    data += sc.Variable(dims=['pixel'], values=a)
    data.masks['tube_ends'] = sc.Variable(dims=['pixel'],
                                          values=np.where(
                                              a > 0.5, True, False))
    plot(data, projection="3d")
