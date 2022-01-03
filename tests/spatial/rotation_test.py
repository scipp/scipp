# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
from scipp.spatial import rotation, rotation_from_rotvec, rotations, rotation_as_rotvec
from math import pi
import scipp as sc

import pytest


def test_from_rotvec_bad_unit():
    with pytest.raises(sc.UnitError):
        rotation_from_rotvec(value=[90, 0, 0], unit="m")


def test_from_rotvec():
    values = [1.2, -2.3, 3.4]
    rot = rotation_from_rotvec(value=values, unit='deg')
    from scipy.spatial.transform import Rotation as R
    expected = R.from_rotvec(values, degrees=True)

    # Test rotations are equivalent by applying to a test vector.
    test_vec = sc.vector(value=[4, 5, 6])
    assert sc.allclose(rot * test_vec, sc.matrix(value=expected.as_matrix()) * test_vec)


def test_from_rotvec_rotation():
    v1 = sc.vector(value=[1, 2, 3], unit='m')
    rot = rotation_from_rotvec(value=[90, 0, 0], unit='deg')
    assert sc.allclose(rot * v1, sc.vector(value=[1, -3, 2], unit='m'))


def test_from_rotvec_rad():
    assert sc.identical(rotation_from_rotvec(value=[pi / 2, 0, 0], unit="rad"),
                        rotation_from_rotvec(value=[90, 0, 0], unit="deg"))


def test_as_rotvec_bad_unit():
    rot = rotation_from_rotvec(value=[1.2, -2.3, 3.4], unit='deg')
    with pytest.raises(sc.UnitError):
        rotation_as_rotvec(rot, unit='m')


def test_from_quaternion():
    quat = [1, 0, 0, 0]

    transform = rotation(value=quat)
    vector = sc.vector(value=[1, 2, 3], unit=sc.units.m)

    assert sc.allclose(transform * vector, sc.vector(value=[1, -2, -3],
                                                     unit=sc.units.m))


def test_from_quaternions():
    quat1 = [1, 0, 0, 0]
    quat2 = [0, 1, 0, 0]

    transforms = rotations(dims=["x"], values=[quat1, quat2])
    vectors = sc.vectors(dims=["x"], values=[[1, 2, 3], [4, 5, 6]], unit=sc.units.m)

    assert sc.allclose(
        transforms * vectors,
        sc.vectors(dims=["x"], values=[[1, -2, -3], [-4, 5, -6]], unit=sc.units.m))
