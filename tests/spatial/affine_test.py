import numpy as np
import pytest

import scipp as sc
from scipp.spatial import affine_transform, affine_transforms, inv


def test_from_affine_matrix() -> None:
    # affine matrix corresponding to a translation by (4, 5, 6)
    matrix = [[1, 0, 0, 4], [0, 1, 0, 5], [0, 0, 1, 6], [0, 0, 0, 1]]

    transform = affine_transform(value=matrix, unit=sc.units.m)
    vector = sc.vector(value=[1, 2, 3], unit=sc.units.m)

    assert sc.allclose(transform * vector, sc.vector(value=[5, 7, 9], unit=sc.units.m))


def test_from_affine_matrices() -> None:
    # affine matrix corresponding to a translation by (4, 5, 6)
    matrix1 = [[1, 0, 0, 4], [0, 1, 0, 5], [0, 0, 1, 6], [0, 0, 0, 1]]
    # affine matrix corresponding to a translation by (7, 8, 9)
    matrix2 = [[1, 0, 0, 7], [0, 1, 0, 8], [0, 0, 1, 9], [0, 0, 0, 1]]

    transforms = affine_transforms(
        dims=["x"], values=[matrix1, matrix2], unit=sc.units.m
    )
    vectors = sc.vectors(dims=["x"], values=[[1, 2, 3], [3, 2, 1]], unit=sc.units.m)

    assert sc.allclose(
        transforms * vectors,
        sc.vectors(dims=["x"], values=[[5, 7, 9], [10, 10, 10]], unit=sc.units.m),
    )


def test_from_affine_matrix_with_bad_unit() -> None:
    # affine matrix corresponding to a translation by (4, 5, 6)
    matrix = [[1, 0, 0, 4], [0, 1, 0, 5], [0, 0, 1, 6], [0, 0, 0, 1]]

    transform = affine_transform(value=matrix, unit=sc.units.s)
    vector = sc.vector(value=[1, 2, 3], unit=sc.units.m)

    with pytest.raises(sc.UnitError):
        _ = transform * vector


def test_affine_transform_default_unit_is_dimensionless() -> None:
    var = affine_transform(value=np.ones(shape=(4, 4)))
    assert var.unit == sc.units.one


def test_affine_transforms_default_unit_is_dimensionless() -> None:
    var = affine_transforms(dims=['x'], values=np.ones(shape=(3, 4, 4)))
    assert var.unit == sc.units.one


def test_can_get_value_of_0d_variable() -> None:
    rng = np.random.default_rng()
    value = rng.random((4, 4))
    var = affine_transform(value=value)
    assert np.array_equal(var.value, value)


def test_can_set_value_of_0d_variable() -> None:
    rng = np.random.default_rng()
    value = rng.random((4, 4))
    var = affine_transform(value=value)
    var.value += value
    assert np.array_equal(var.value, value + value)


def test_inv() -> None:
    transform = affine_transform(
        value=[[1, 0.1, -1.7, 2], [0.3, 1, -0.8, -2], [0, 0.2, 1, 3.1], [0, 0, 0, 1]],
        unit='cm',
    )
    vec = sc.vector([3.2, 1, 4.1], unit='cm')
    assert sc.allclose(transform * inv(transform) * vec, vec)
    assert sc.allclose(inv(transform) * transform * vec, vec)
