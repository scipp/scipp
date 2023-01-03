# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

import numpy as np
import pytest

import scipp as sc

from ..factory import make_binned_data_array, make_dense_data_array


def _with_fake_pos(*args, **kwargs):
    da = make_dense_data_array(*args, **kwargs)
    da.coords['pos'] = sc.geometry.position(da.coords['xx'], da.coords['yy'],
                                            da.coords['zz']).transpose(da.dims[:3])
    return da


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
    da = sc.DataArray(data=sc.Variable(dims=['time', 'xyz'], values=a),
                      coords={
                          'xyz':
                          sc.vectors(dims=['xyz'], values=np.array([x, y, z]).T),
                          'pos':
                          sc.vectors(dims=['xyz'], values=np.array([x, y, z]).T + 20.0),
                          'time':
                          sc.Variable(dims=['time'], values=time)
                      })
    return da


def test_plot_projection_3d():
    da = _with_fake_pos(ndim=3)
    sc.plot(da, positions='pos', projection="3d")
    sc.plot(da, positions='pos', projection="3d", resampling_mode='sum')
    sc.plot(da, positions='pos', projection="3d", resampling_mode='mean')


def test_plot_projection_3d_log_norm():
    sc.plot(_with_fake_pos(ndim=3), positions='pos', projection="3d", norm='log')


def test_plot_projection_3d_dataset():
    sc.plot(_with_fake_pos(ndim=3), positions='pos', projection="3d")


def test_plot_projection_3d_with_labels():
    sc.plot(_with_fake_pos(ndim=3, labels=True),
            positions='pos',
            projection="3d",
            labels={'x': "lab"})


def test_plot_projection_3d_with_masks():
    sc.plot(_with_fake_pos(ndim=3, masks=True), positions='pos', projection="3d")


def test_plot_projection_3d_with_vectors():
    sc.plot(make_data_array_with_position_vectors(), projection="3d", positions="xyz")


def test_plot_projection_3d_with_vectors_non_dim_coord():
    sc.plot(make_data_array_with_position_vectors(), projection="3d", positions="pos")


def test_plot_variable_3d():
    N = 50
    v3d = sc.Variable(dims=['time', 'y', 'x'],
                      values=np.random.rand(N, N, N),
                      unit=sc.units.m)
    positions = sc.vectors(dims=v3d.dims, values=np.random.rand(N, N, N, 3))
    sc.plot(v3d, positions=positions, projection="3d")


def test_plot_4d_with_masks_projection_3d():
    da = sc.DataArray(data=sc.Variable(dims=['pack', 'tube', 'straw', 'pixel'],
                                       values=np.random.rand(2, 8, 7, 256)),
                      coords={})
    a = np.sin(np.linspace(0, 3.14, num=256))
    da += sc.Variable(dims=['pixel'], values=a)
    da.masks['tube_ends'] = sc.Variable(dims=['pixel'],
                                        values=np.where(a > 0.5, True, False))
    da.coords['pos'] = sc.geometry.position(sc.arange(dim='pack', start=0., stop=2),
                                            sc.arange(dim='tube', start=0., stop=8),
                                            sc.arange(dim='straw', start=0., stop=7))
    sc.plot(da, positions='pos', projection="3d")


def test_plot_customized_axes():
    da = _with_fake_pos(ndim=3)
    sc.plot(da,
            positions='pos',
            projection="3d",
            xlabel="MyXlabel",
            ylabel="MyYlabel",
            zlabel="MyZlabel")


def test_plot_3d_with_2d_position_coordinate():
    nx = 50
    ny = 40
    nt = 10

    xx, yy = np.meshgrid(np.arange(nx, dtype=np.float64), np.arange(ny,
                                                                    dtype=np.float64))
    da = sc.DataArray(
        data=sc.Variable(dims=['x', 'y', 't'],
                         values=np.arange(nx * ny * nt).reshape(nx, ny, nt)),
        coords={
            'pos':
            sc.vectors(dims=['x', 'y'],
                       values=np.array([xx, yy,
                                        np.zeros_like(xx)]).T.reshape(nx, ny, 3)),
            't':
            sc.arange('t', nt + 1, dtype=np.float64)
        })

    sc.plot(da, projection="3d", positions="pos")


def test_plot_3d_binned_data():
    da = make_binned_data_array(ndim=1)
    pos = sc.vectors(dims=da.dims, values=np.random.rand(da.sizes[da.dims[0]], 3))
    sc.plot(da, projection='3d', positions=pos)
    sc.plot(da, projection='3d', positions=pos, resampling_mode='sum')
    sc.plot(da, projection='3d', positions=pos, resampling_mode='mean')


def test_plot_redraw():
    da = _with_fake_pos(ndim=3, unit='K')
    p = sc.plot(da, positions='pos', projection="3d")
    before = p.view.figure.points_geometry.attributes["color"].array
    da *= 5.0
    p.redraw()
    after = p.view.figure.points_geometry.attributes["color"].array
    assert np.any(before != after)


def test_plot_projection_3d_with_camera():
    da = make_data_array_with_position_vectors()
    da.coords['xyz'].unit = 'm'
    sc.plot(da,
            projection="3d",
            positions="xyz",
            camera={
                'position': sc.vector(value=[150, 10, 10], unit='m'),
                'look_at': sc.vector(value=[0, 0, 30], unit='m')
            })
    sc.plot(da,
            projection="3d",
            positions="xyz",
            camera={'position': sc.vector(value=[150, 10, 10], unit='m')})
    sc.plot(da,
            projection="3d",
            positions="xyz",
            camera={'look_at': sc.vector(value=[0, 0, 30], unit='m')})


def test_plot_projection_3d_with_camera_supports_compatible_units():
    da = make_data_array_with_position_vectors()
    da.coords['xyz'].unit = 'm'
    sc.plot(da,
            projection="3d",
            positions="xyz",
            camera={
                'position': sc.vector(value=[150, 10, 10], unit='mm'),
                'look_at': sc.vector(value=[0, 0, 30], unit='mm')
            })
    sc.plot(da,
            projection="3d",
            positions="xyz",
            camera={'position': sc.vector(value=[150, 10, 10], unit='mm')})
    sc.plot(da,
            projection="3d",
            positions="xyz",
            camera={'look_at': sc.vector(value=[0, 0, 30], unit='mm')})


def test_plot_projection_3d_with_camera_raises_if_camera_param_units_wrong():
    da = make_data_array_with_position_vectors()
    da.coords['xyz'].unit = 'm'
    with pytest.raises(sc.UnitError):
        sc.plot(da,
                projection="3d",
                positions="xyz",
                camera={'position': sc.vector(value=[150, 10, 10], unit='s')})
    with pytest.raises(sc.UnitError):
        sc.plot(da,
                projection="3d",
                positions="xyz",
                camera={'look_at': sc.vector(value=[0, 0, 30], unit='s')})
