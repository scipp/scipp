# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
from scipp.spatial.transform import from_rotvec
import scipp as sc
import numpy as np

import pytest


def test_from_rotvec_bad_unit():
    rotvec = sc.vector(value=[90, 0, 0], unit='m')
    with pytest.raises(sc.UnitError):
        from_rotvec(rotvec)


def test_from_rotvec():
    rotvec = sc.vector(value=[1.2, -2.3, 3.4], unit='deg')
    rot = from_rotvec(rotvec)
    from scipy.spatial.transform import Rotation as R
    expected = R.from_rotvec(rotvec.values, degrees=True)
    assert sc.identical(rot, sc.matrix(value=expected.as_matrix()))


def test_from_rotvec_rotation():
    v1 = sc.vector(value=[1, 2, 3], unit='m')
    rotvec = sc.vector(value=[90, 0, 0], unit='deg')
    rot = from_rotvec(rotvec)
    assert sc.allclose(rot * v1, sc.vector(value=[1, -3, 2], unit='m'))


def test_from_rotvec_rad():
    deg = sc.vector(value=[90, 0, 0], unit='deg')
    rad = sc.to_unit(deg, 'rad')
    assert np.allclose(from_rotvec(deg).values, from_rotvec(rad).values)
