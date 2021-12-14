from scipp.spatial import affine_transform, affine_transforms
import scipp as sc


def test_from_affine_matrix():
    # affine matrix corresponding to a translation by (4, 5, 6)
    matrix = [[1, 0, 0, 4], [0, 1, 0, 5], [0, 0, 1, 6], [0, 0, 0, 1]]

    transform = affine_transform(value=matrix, unit=sc.units.m)
    vector = sc.vector(value=[1, 2, 3], unit=sc.units.m)

    assert sc.allclose(transform * vector, sc.vector(value=[5, 7, 9], unit=sc.units.m))


def test_from_affine_matrices():
    # affine matrix corresponding to a translation by (4, 5, 6)
    matrix1 = [[1, 0, 0, 4], [0, 1, 0, 5], [0, 0, 1, 6], [0, 0, 0, 1]]
    # affine matrix corresponding to a translation by (7, 8, 9)
    matrix2 = [[1, 0, 0, 7], [0, 1, 0, 8], [0, 0, 1, 9], [0, 0, 0, 1]]

    transforms = affine_transforms(dims=["x"],
                                   values=[matrix1, matrix2],
                                   unit=sc.units.m)
    vectors = sc.vectors(dims=["x"], values=[[1, 2, 3], [3, 2, 1]], unit=sc.units.m)

    assert sc.allclose(
        transforms * vectors,
        sc.vectors(dims=["x"], values=[[5, 7, 9], [10, 10, 10]], unit=sc.units.m))
