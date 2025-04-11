# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

import pytest

import scipp as sc
import scipp.testing


def test_as_vectors_scalars() -> None:
    sc.testing.assert_identical(
        sc.spatial.as_vectors(
            1.0 * sc.Unit('m'), 2.0 * sc.Unit('m'), 3.0 * sc.Unit('m')
        ),
        sc.vector([1, 2, 3], unit='m'),
    )


def test_as_vectors_arrays_with_matching_dims() -> None:
    x = sc.array(dims=['p'], values=[1.1, 2.3, 4.5])
    y = 2 * x
    z = -3 * x

    vec = sc.spatial.as_vectors(x, y, z)
    sc.testing.assert_identical(vec.fields.x, x)
    sc.testing.assert_identical(vec.fields.y, y)
    sc.testing.assert_identical(vec.fields.z, z)


def test_as_vectors_arrays_broadcast() -> None:
    x = sc.array(dims=['1'], values=[1.1, 2.3, 4.5])
    y = 2 * x
    z = sc.array(dims=['2'], values=[6.5, 4.3])

    vec = sc.spatial.as_vectors(x, y, z)
    sc.testing.assert_identical(vec.fields.x, x.broadcast(sizes={'1': 3, '2': 2}))
    sc.testing.assert_identical(vec.fields.y, y.broadcast(sizes={'1': 3, '2': 2}))
    sc.testing.assert_identical(vec.fields.z, z.broadcast(sizes={'1': 3, '2': 2}))


def test_as_vectors_requires_matching_units() -> None:
    v = sc.scalar(1.0, unit='m')
    with pytest.raises(sc.UnitError):
        sc.spatial.as_vectors(v, v, sc.scalar(1.0, unit='s'))
    with pytest.raises(sc.UnitError):
        sc.spatial.as_vectors(v, sc.scalar(1.0, unit='s'), v)
    with pytest.raises(sc.UnitError):
        sc.spatial.as_vectors(sc.scalar(1.0, unit='s'), v, v)


@pytest.mark.parametrize('dtype', ['float32', 'int64'])
def test_as_vectors_does_not_require_float64(dtype) -> None:
    v = sc.scalar(1.0, unit='m', dtype=dtype)
    w = sc.scalar(1.0, unit='m', dtype='float64')
    assert sc.allclose(sc.spatial.as_vectors(v, v, v), sc.spatial.as_vectors(w, w, w))
