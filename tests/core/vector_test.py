# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 Scipp contributors (https://github.com/scipp)
import numpy as np

import scipp as sc


def test_norm() -> None:
    assert sc.allclose(sc.norm(sc.vector([1.0, 2.0, 0.0])), sc.scalar(np.sqrt(1 + 4)))


def test_dot() -> None:
    assert sc.identical(
        sc.dot(sc.vector([1, 0, 2]), sc.vector([0, 1, 3])), sc.scalar(6.0)
    )


def test_cross() -> None:
    assert sc.identical(
        sc.cross(sc.vector([0, 0, 1]), sc.vector([0, 1, 0])), sc.vector([-1, 0, 0])
    )


def test_angle_between() -> None:
    assert sc.allclose(
        sc.spatial.angle_between(sc.vector([1, 0, 0]), sc.vector([1, 1, 0])),
        sc.scalar(np.pi / 4, unit='rad'),
    )
