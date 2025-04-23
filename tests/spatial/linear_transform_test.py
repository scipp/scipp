# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

import numpy as np

import scipp as sc
import scipp.testing
from scipp.spatial import inv, linear_transform, linear_transforms


def test_linear_transform_numpy() -> None:
    matrix = np.array([[1, 2, 3], [7, 8, 9], [4, 5, 6]])
    trans = linear_transform(value=matrix, unit='s')
    assert trans.unit == 's'

    test_vec = sc.vector([0.1, 0.2, 0.1], unit='m')
    res = trans * test_vec
    np.testing.assert_allclose(res.values, matrix @ test_vec.value)
    assert res.unit == 's m'


def test_linear_transform_list() -> None:
    matrix = [[1, 2, 3], [7, 8, 9], [4, 5, 6]]
    trans = linear_transform(value=matrix, unit='s')
    assert trans.unit == 's'

    test_vec = sc.vector([0.1, 0.2, 0.1], unit='m')
    res = trans * test_vec
    np.testing.assert_allclose(res.values, np.array(matrix) @ test_vec.value)
    assert res.unit == 's m'


def test_linear_transforms_numpy() -> None:
    matrix = np.array(
        [[[1, 2, 3], [7, 8, 9], [4, 5, 6]], [[-1, 3, -2], [0, 9, -5], [7, 7, -4]]]
    )
    trans = linear_transforms(dims=['lin'], values=matrix, unit='s')
    assert trans.unit == 's'

    test_vec = sc.vector([0.1, 0.2, 0.1], unit='m')
    res = trans * test_vec
    np.testing.assert_allclose(res.values, matrix @ test_vec.value)
    assert res.unit == 's m'
    assert res.dims == ('lin',)


def test_linear_transforms_list() -> None:
    matrix = [[[1, 2, 3], [7, 8, 9], [4, 5, 6]], [[-1, 3, -2], [0, 9, -5], [7, 7, -4]]]
    trans = linear_transforms(dims=['lin'], values=matrix, unit='s')
    assert trans.unit == 's'

    test_vec = sc.vector([0.1, 0.2, 0.1], unit='m')
    res = trans * test_vec
    np.testing.assert_allclose(res.values, np.array(matrix) @ test_vec.value)
    assert res.unit == 's m'
    assert res.dims == ('lin',)


def test_linear_transform_default_unit_is_dimensionless() -> None:
    matrix = [[1, 2, 3], [7, 8, 9], [4, 5, 6]]
    trans = linear_transform(value=matrix)
    assert trans.unit == 'one'


def test_linear_transforms_default_unit_is_dimensionless() -> None:
    matrix = [[[1, 2, 3], [7, 8, 9], [4, 5, 6]]]
    trans = linear_transforms(dims=['x'], values=matrix)
    assert trans.unit == 'one'


def test_linear_transform_value() -> None:
    matrix = np.array([[1, 2, 3], [7, 8, 9], [4, 5, 6]])
    trans = linear_transform(value=matrix)
    np.testing.assert_array_equal(trans.value, matrix)


def test_linear_transforms_values() -> None:
    matrix = np.array(
        [[[1, 2, 3], [7, 8, 9], [4, 5, 6]], [[-1, 3, -2], [0, 9, -5], [7, 7, -4]]]
    )
    trans = linear_transforms(dims=['lin'], values=matrix)
    np.testing.assert_array_equal(trans.values, matrix)


def test_linear_transform_set_value() -> None:
    trans = linear_transform(value=np.ones((3, 3)))
    matrix = np.array([[1, 2, 3], [7, 8, 9], [4, 5, 6]])
    trans.value = matrix
    np.testing.assert_array_equal(trans.value, matrix)


def test_linear_transform_get_elements() -> None:
    matrix = np.array([[1, 2, 3], [7, 8, 9], [4, 5, 6]])
    trans = linear_transform(value=matrix)
    assert trans.fields.xx == 1
    assert trans.fields.xy == 2
    assert trans.fields.xz == 3
    assert trans.fields.yx == 7
    assert trans.fields.yy == 8
    assert trans.fields.yz == 9
    assert trans.fields.zx == 4
    assert trans.fields.zy == 5
    assert trans.fields.zz == 6


def test_linear_transform_set_elements() -> None:
    matrix = np.array([[1, 2, 3], [7, 8, 9], [4, 5, 6]])
    trans = linear_transform(value=matrix, unit='m')
    trans.fields.xx = sc.scalar(10.0, unit='m')
    np.testing.assert_array_equal(trans.values, [[10, 2, 3], [7, 8, 9], [4, 5, 6]])
    trans.fields.yz = sc.scalar(-6.0, unit='m')
    np.testing.assert_array_equal(trans.values, [[10, 2, 3], [7, 8, -6], [4, 5, 6]])
    trans.fields.zx = sc.scalar(0.4, unit='m')
    np.testing.assert_array_equal(trans.values, [[10, 2, 3], [7, 8, -6], [0.4, 5, 6]])


def test_linear_transforms_get_elements() -> None:
    matrix = np.array(
        [[[1, 2, 3], [7, 8, 9], [4, 5, 6]], [[-1, 3, -2], [0, 9, -5], [7, 7, -4]]]
    )
    trans = linear_transforms(dims=['lin'], values=matrix)
    sc.testing.assert_identical(
        trans.fields.xx, sc.array(dims=['lin'], values=[1.0, -1.0])
    )
    sc.testing.assert_identical(
        trans.fields.xy, sc.array(dims=['lin'], values=[2.0, 3.0])
    )
    sc.testing.assert_identical(
        trans.fields.xz, sc.array(dims=['lin'], values=[3.0, -2.0])
    )
    sc.testing.assert_identical(
        trans.fields.yx, sc.array(dims=['lin'], values=[7.0, 0.0])
    )
    sc.testing.assert_identical(
        trans.fields.yy, sc.array(dims=['lin'], values=[8.0, 9.0])
    )
    sc.testing.assert_identical(
        trans.fields.yz, sc.array(dims=['lin'], values=[9.0, -5.0])
    )
    sc.testing.assert_identical(
        trans.fields.zx, sc.array(dims=['lin'], values=[4.0, 7.0])
    )
    sc.testing.assert_identical(
        trans.fields.zy, sc.array(dims=['lin'], values=[5.0, 7.0])
    )
    sc.testing.assert_identical(
        trans.fields.zz, sc.array(dims=['lin'], values=[6.0, -4.0])
    )


def test_linear_transforms_set_elements() -> None:
    matrix = np.array(
        [[[1, 2, 3], [7, 8, 9], [4, 5, 6]], [[-1, 3, -2], [0, 9, -5], [7, 7, -4]]]
    )
    trans = linear_transforms(dims=['lin'], values=matrix)
    trans.fields.xy = sc.array(dims=['lin'], values=[-1.1, 32])
    np.testing.assert_array_equal(
        trans.values,
        [[[1, -1.1, 3], [7, 8, 9], [4, 5, 6]], [[-1, 32, -2], [0, 9, -5], [7, 7, -4]]],
    )
    trans.fields.yy = sc.array(dims=['lin'], values=[100.0, 110.0])
    np.testing.assert_array_equal(
        trans.values,
        [
            [[1, -1.1, 3], [7, 100, 9], [4, 5, 6]],
            [[-1, 32, -2], [0, 110, -5], [7, 7, -4]],
        ],
    )
    trans.fields.zx = sc.array(dims=['lin'], values=[0.0, 22.0])
    np.testing.assert_array_equal(
        trans.values,
        [
            [[1, -1.1, 3], [7, 100, 9], [0, 5, 6]],
            [[-1, 32, -2], [0, 110, -5], [22, 7, -4]],
        ],
    )


def test_linear_transform_inv() -> None:
    matrix = np.array([[1, -2, 3], [7, 8, 9], [4, 5, 6]])
    trans = linear_transform(value=matrix, unit='s')
    vec = sc.vector([6.2, 4.21, 3.0], unit='kg')
    assert sc.allclose(trans * inv(trans) * vec, vec)
    assert sc.allclose(inv(trans) * trans * vec, vec)


def test_linear_transforms_inv() -> None:
    matrix = [[[1, -2, 3], [7, 8, 9], [4, 5, 6]], [[-1, 3, -2], [0, 9, -5], [7, 7, -4]]]
    trans = linear_transforms(dims=['lin'], values=matrix, unit='s')
    vec = sc.vector([6.2, 4.21, 3.0], unit='kg')
    assert sc.allclose(trans * inv(trans) * vec, vec)
    assert sc.allclose(inv(trans) * trans * vec, vec)
    assert (trans * inv(trans) * vec).sizes == {'lin': 2}
    assert (inv(trans) * trans * vec).sizes == {'lin': 2}
