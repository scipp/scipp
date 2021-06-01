# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

import numpy as np
import scipp as sc
from ..factory import make_dense_data_array, make_dense_dataset
from .plot_helper import plot


def make_data_array_with_position_vectors():
    N = 1000
    M = 100
    theta = np.random.random(N) * np.pi
    phi = np.random.random(N) * 2.0 * np.pi
    r = 10.0 + (np.random.random(N) - 0.5)
    x = r * np.sin(theta) * np.sin(phi)
    y = r * np.sin(theta) * np.cos(phi)
    z = r * np.cos(theta)
    time = np.arange(M, dtype=np.float64)
    a = np.arange(M * N).reshape([M, N]) * np.sin(y)
    da = sc.DataArray(data=sc.Variable(['time', 'xyz'], values=a),
                      coords={
                          'xyz':
                          sc.vectors(dims=['xyz'],
                                     values=np.array([x, y, z]).T),
                          'pos':
                          sc.vectors(dims=['xyz'],
                                     values=np.array([x, y, z]).T + 20.0),
                          'time':
                          sc.Variable(['time'], values=time)
                      })
    return da


def test_plot_projection_3d():
    plot(make_dense_data_array(ndim=3), projection="3d")


def test_plot_projection_3d_dataset():
    plot(make_dense_dataset(ndim=3), projection="3d")


def test_plot_projection_3d_with_labels():
    plot(make_dense_data_array(ndim=3, labels=True),
         projection="3d",
         axes={'x': "lab"})


def test_plot_projection_3d_with_bin_edges():
    plot(make_dense_data_array(ndim=3, binedges=True), projection="3d")


def test_plot_projection_3d_with_masks():
    plot(make_dense_data_array(ndim=3, masks=True), projection="3d")


def test_plot_projection_3d_with_aspect():
    plot(make_dense_data_array(ndim=3), projection="3d", aspect="equal")
    plot(make_dense_data_array(ndim=3), projection="3d", aspect="auto")


def test_plot_projection_3d_with_vectors():
    plot(make_data_array_with_position_vectors(),
         projection="3d",
         positions="xyz")


def test_plot_projection_3d_with_vectors_non_dim_coord():
    plot(make_data_array_with_position_vectors(),
         projection="3d",
         positions="pos")


def test_plot_projection_3d_with_vectors_with_aspect():
    plot(make_data_array_with_position_vectors(),
         projection="3d",
         positions="xyz",
         aspect="auto")


def test_plot_variable_3d():
    N = 50
    v3d = sc.Variable(['time', 'y', 'x'],
                      values=np.random.rand(N, N, N),
                      unit=sc.units.m)
    plot(v3d, projection="3d")


def test_plot_4d_with_masks_projection_3d():
    da = sc.DataArray(data=sc.Variable(dims=['pack', 'tube', 'straw', 'pixel'],
                                       values=np.random.rand(2, 8, 7, 256)),
                      coords={})
    a = np.sin(np.linspace(0, 3.14, num=256))
    da += sc.Variable(dims=['pixel'], values=a)
    da.masks['tube_ends'] = sc.Variable(dims=['pixel'],
                                        values=np.where(a > 0.5, True, False))
    plot(da, projection="3d")


def test_plot_customized_axes():
    da = make_dense_data_array(ndim=3)
    plot(da,
         projection="3d",
         xlabel="MyXlabel",
         ylabel="MyYlabel",
         zlabel="MyZlabel")


def test_plot_3d_with_2d_position_coordinate():
    nx = 50
    ny = 40
    nt = 10

    xx, yy = np.meshgrid(np.arange(nx, dtype=np.float64),
                         np.arange(ny, dtype=np.float64))
    da = sc.DataArray(
        data=sc.Variable(['x', 'y', 't'],
                         values=np.arange(nx * ny * nt).reshape(nx, ny, nt)),
        coords={
            'pos':
            sc.vectors(dims=['x', 'y'],
                       values=np.array([xx, yy, np.zeros_like(xx)
                                        ]).T.reshape(nx, ny, 3)),
            't':
            sc.arange('t', nt + 1, dtype=np.float64)
        })

    plot(da, projection="3d", positions="pos")


def test_plot_redraw():
    da = make_dense_data_array(ndim=3, unit='K')
    p = sc.plot(da, projection="3d")
    before = p.view.figure.points_geometry.attributes["rgba_color"].array
    da *= 5.0
    p.redraw()
    after = p.view.figure.points_geometry.attributes["rgba_color"].array
    assert np.any(before != after)
