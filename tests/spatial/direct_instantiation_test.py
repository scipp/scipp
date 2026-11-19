# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Terence Tsang
from __future__ import annotations

from collections.abc import Callable
from functools import partial

import numpy as np
import pytest

from scipp import DType, Variable, array, vectors
from scipp.spatial import (
    affine_transforms,
    linear_transforms,
    rotations,
    translations,
)
from scipp.testing import assert_identical

_RNG = np.random.default_rng(42)


def _sample(item: tuple[int, ...], shape: tuple[int, ...]) -> np.ndarray:
    return _RNG.random((*shape, *item))


_SHAPE_CASES = [pytest.param((5,) * ndim, id=f'{ndim}D') for ndim in range(4)]
_DTYPE_CASES = [  # dtype, sampler, constructor
    pytest.param(
        getattr(DType, str(np.dtype(float))),  # Default float type
        partial(_sample, ()),
        array,
        id='float',
    ),
    pytest.param(
        DType.vector3,
        partial(_sample, (3,)),
        vectors,
        id='vector3',
    ),
    pytest.param(
        DType.translation3,
        partial(_sample, (3,)),
        translations,
        id='translation3',
    ),
    pytest.param(
        DType.rotation3,
        # This isn't normed and thus wouldn't represent a proper
        # rotation... but we don't really care about that, just that the
        # quaternion components are preserved.
        partial(_sample, (4,)),
        rotations,
        id='rotation3',
    ),
    pytest.param(
        DType.linear_transform3,
        partial(_sample, (3, 3)),
        linear_transforms,
        id='linear_transform3',
    ),
    pytest.param(
        DType.affine_transform3,
        partial(_sample, (4, 4)),
        affine_transforms,
        id='affine_transform3',
    ),
]


@pytest.mark.parametrize('shape', _SHAPE_CASES)
@pytest.mark.parametrize(('dtype', 'sampler', 'constructor'), _DTYPE_CASES)
def test_values_consistent_with_input(
    shape: tuple[int, ...],
    dtype: DType,
    sampler: Callable[[tuple[int, ...]], np.ndarray],
    constructor: Callable[..., Variable],
) -> None:
    """
    Test that the data layouts of scalar and array numeric
    :class:`Variable`s are consistent with the input arrays, no matter
    if constructed from the :mod:`scipp.spatial` constructors or
    directly with the class.
    """
    array_obj = sampler(shape)
    dims = tuple(f'dim{i}' for i, _ in enumerate(shape))
    direct = Variable(values=array_obj, dims=dims, dtype=dtype)
    indirect = constructor(values=array_obj, dims=dims)

    assert direct.dims == indirect.dims == dims
    assert direct.shape == indirect.shape == shape
    assert direct.dtype == indirect.dtype == dtype

    assert_identical(direct, indirect)
    assert (direct.values == array_obj).all()
    assert (indirect.values == array_obj).all()
