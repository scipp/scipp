# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
from math import pi

import numpy as np
import pytest

import scipp as sc
from scipp.spatial import (
    inv,
    rotation,
    rotation_as_rotvec,
    rotations,
    rotations_from_rotvecs,
)


def test_from_rotvec_bad_unit() -> None:
    with pytest.raises(sc.UnitError):
        rotations_from_rotvecs(sc.vector(value=[90, 0, 0], unit="m"))


def test_from_rotvec() -> None:
    values = [1.2, -2.3, 3.4]
    rot = rotations_from_rotvecs(sc.vectors(dims=["x"], values=[values], unit='deg'))
    from scipy.spatial.transform import Rotation as R

    expected = R.from_rotvec(values, degrees=True)

    # Test rotations are equivalent by applying to a test vector.
    test_vec = sc.vector(value=[4, 5, 6])
    assert sc.allclose(
        rot * test_vec, sc.vector(expected.as_matrix() @ test_vec.values)
    )


def test_from_rotvec_rotation() -> None:
    v1 = sc.vector(value=[1, 2, 3], unit='m')
    rot = rotations_from_rotvecs(sc.vector(value=[90, 0, 0], unit='deg'))
    assert sc.allclose(rot * v1, sc.vector(value=[1, -3, 2], unit='m'))


def test_from_rotvec_rad() -> None:
    assert sc.identical(
        rotations_from_rotvecs(sc.vector(value=[pi / 2, 0, 0], unit="rad")),
        rotations_from_rotvecs(sc.vector(value=[90, 0, 0], unit="deg")),
    )


def test_as_rotvec_bad_unit() -> None:
    rot = rotations_from_rotvecs(sc.vector(value=[1.2, -2.3, 3.4], unit='deg'))
    with pytest.raises(sc.UnitError):
        rotation_as_rotvec(rot, unit='m')


def test_from_quaternion() -> None:
    quat = [1, 0, 0, 0]

    transform = rotation(value=quat)
    vector = sc.vector(value=[1, 2, 3], unit=sc.units.m)

    assert sc.allclose(
        transform * vector, sc.vector(value=[1, -2, -3], unit=sc.units.m)
    )


def test_from_quaternions() -> None:
    quat1 = [1, 0, 0, 0]
    quat2 = [0, 1, 0, 0]

    transforms = rotations(dims=["x"], values=[quat1, quat2])
    vectors = sc.vectors(dims=["x"], values=[[1, 2, 3], [4, 5, 6]], unit=sc.units.m)

    assert sc.allclose(
        transforms * vectors,
        sc.vectors(dims=["x"], values=[[1, -2, -3], [-4, 5, -6]], unit=sc.units.m),
    )


def test_from_quaternions_2d() -> None:
    quat1 = [1, 0, 0, 0]
    quat2 = [0, 1, 0, 0]

    transforms = rotations(dims=["y", "x"], values=[[quat1, quat2]])
    vectors = sc.vectors(dims=["x"], values=[[1, 2, 3], [4, 5, 6]], unit=sc.units.m)

    assert sc.allclose(
        transforms * vectors,
        sc.vectors(
            dims=["y", "x"], values=[[[1, -2, -3], [-4, 5, -6]]], unit=sc.units.m
        ),
    )


def test_rotation_default_unit_is_dimensionless() -> None:
    var = rotation(value=np.ones(shape=(4,)))
    assert var.unit == sc.units.one


def test_rotations_default_unit_is_dimensionless() -> None:
    var = rotations(dims=['x'], values=np.ones(shape=(3, 4)))
    assert var.unit == sc.units.one


def test_can_get_value_of_0d_variable() -> None:
    rng = np.random.default_rng()
    value = rng.random((4,))
    var = rotation(value=value)
    assert np.array_equal(var.value, value)


def test_can_set_value_of_0d_variable() -> None:
    rng = np.random.default_rng()
    value = rng.random((4,))
    var = rotation(value=value)
    var.value += value
    assert np.array_equal(var.value, value + value)


def test_inv() -> None:
    transform = rotation(value=[0.1, 1.1, 2.8, -0.4])
    vec = sc.vector([3.2, 1, 4.1], unit='s')
    assert sc.allclose(transform * inv(transform) * vec, vec)
    assert sc.allclose(inv(transform) * transform * vec, vec)
