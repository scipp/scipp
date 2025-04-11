import numpy as np

import scipp as sc
from scipp.spatial import inv, translation, translations


def test_translation() -> None:
    transform = translation(value=[1, -5, 5], unit=sc.units.m)
    vector = sc.vector(value=[1, 2, 3], unit=sc.units.m)

    assert sc.allclose(transform * vector, sc.vector(value=[2, -3, 8], unit=sc.units.m))


def test_translations() -> None:
    transforms = translations(
        dims=["x"], values=[[1, 0, -2], [3, 4, 5]], unit=sc.units.m
    )
    vectors = sc.vectors(dims=["x"], values=[[1, 2, 3], [4, 5, 6]], unit=sc.units.m)

    assert sc.allclose(
        transforms * vectors,
        sc.vectors(dims=["x"], values=[[2, 2, 1], [7, 9, 11]], unit=sc.units.m),
    )


def test_translation_default_unit_is_dimensionless() -> None:
    var = translation(value=np.ones(shape=(3,)))
    assert var.unit == sc.units.one


def test_translations_default_unit_is_dimensionless() -> None:
    var = translations(dims=['x'], values=np.ones(shape=(2, 3)))
    assert var.unit == sc.units.one


def test_can_get_value_of_0d_variable() -> None:
    rng = np.random.default_rng()
    value = rng.random((3,))
    var = translation(value=value)
    assert np.array_equal(var.value, value)


def test_can_set_value_of_0d_variable() -> None:
    rng = np.random.default_rng()
    value = rng.random((3,))
    var = translation(value=value)
    var.value += value
    assert np.array_equal(var.value, value + value)


def test_inv() -> None:
    rng = np.random.default_rng()
    value = rng.random((3,))
    transform = translation(value=value, unit='kg')
    vec = sc.vector([3.2, 1, 4.1], unit='kg')
    assert sc.allclose(transform * inv(transform) * vec, vec)
    assert sc.allclose(inv(transform) * transform * vec, vec)
