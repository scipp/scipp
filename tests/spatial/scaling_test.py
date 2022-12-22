import scipp as sc
from scipp.spatial import scaling_from_vector, scalings_from_vectors


def test_from_scaling_vector():
    transform = scaling_from_vector(value=[1, 0, -2])
    vector = sc.vector(value=[1, 2, 3], unit=sc.units.m)

    assert sc.allclose(transform * vector, sc.vector(value=[1, 0, -6], unit=sc.units.m))


def test_from_scaling_vectors():
    transforms = scalings_from_vectors(dims=["x"], values=[[1, 0, -2], [3, 2, 1]])
    vectors = sc.vectors(dims=["x"], values=[[1, 2, 3], [4, 5, 6]], unit=sc.units.m)

    assert sc.allclose(
        transforms * vectors,
        sc.vectors(dims=["x"], values=[[1, 0, -6], [12, 10, 6]], unit=sc.units.m))
