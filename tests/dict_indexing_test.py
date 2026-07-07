# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
import numpy as np
import pytest
import scipp as sc

# Base shape (N, M, K, L, S) with named dims.
_N, _M, _K, _L, _S = 2, 3, 4, 5, 6
_DIMS = ('n', 'm', 'k', 'l', 's')
_SHAPE = (_N, _M, _K, _L, _S)


def _make_variable() -> sc.Variable:
    return sc.array(
        dims=list(_DIMS),
        values=np.arange(np.prod(_SHAPE), dtype=float).reshape(_SHAPE),
    )


def _make_data_array() -> sc.DataArray:
    return sc.DataArray(_make_variable())


def _make_dataset() -> sc.Dataset:
    return sc.Dataset({'a': _make_data_array()})


_make_obj_params = [
    pytest.param(_make_variable, id='Variable'),
    pytest.param(_make_data_array, id='DataArray'),
    pytest.param(_make_dataset, id='Dataset'),
]

# Each case: (index_dict, expected_dims_after_slicing)
_index_params = [
    # All 5 dims indexed → scalar ()
    pytest.param(
        {'n': 0, 'm': 1, 'k': 2, 'l': 3, 's': 4},
        (),
        id='scalar',
    ),
    # n, m, s indexed → (K, L) remain
    pytest.param(
        {'n': 0, 'm': 1, 's': 2},
        ('k', 'l'),
        id='kl',
    ),
    # n, m, k, l indexed → (S,) remains
    pytest.param(
        {'n': 0, 'm': 1, 'k': 2, 'l': 3},
        ('s',),
        id='s',
    ),
    # m, l, s indexed → (N, K) remain
    pytest.param(
        {'m': 1, 'l': 3, 's': 2},
        ('n', 'k'),
        id='nk',
    ),
]


@pytest.mark.parametrize('make_obj', _make_obj_params)
@pytest.mark.parametrize(('index_dict', 'expected_dims'), _index_params)
def test_getitem_with_dict(make_obj, index_dict, expected_dims):
    obj = make_obj()
    result = obj[index_dict]
    assert result.dims == expected_dims
    # Must equal the result of applying each (dim, idx) slice in sequence.
    expected = obj
    for dim, idx in index_dict.items():
        expected = expected[dim, idx]
    assert sc.identical(result, expected)


@pytest.mark.parametrize('make_obj', _make_obj_params)
@pytest.mark.parametrize(('index_dict', 'expected_dims'), _index_params)
def test_setitem_with_dict(make_obj, index_dict, expected_dims):
    obj = make_obj()
    fill = obj[index_dict] * 2.0
    obj[index_dict] = fill
    assert sc.identical(obj[index_dict], fill)


@pytest.mark.parametrize('make_obj', _make_obj_params)
def test_getitem_empty_dict(make_obj):
    obj = make_obj()
    result = obj[{}]
    assert sc.identical(result, obj)
    assert result is not obj


@pytest.mark.parametrize('make_obj', _make_obj_params)
def test_getitem_bad_dim_label(make_obj):
    obj = make_obj()
    with pytest.raises(sc.DimensionError):
        _ = obj[{'nonexistent': 0}]


@pytest.mark.parametrize('make_obj', _make_obj_params)
def test_getitem_bad_index(make_obj):
    obj = make_obj()
    with pytest.raises(IndexError):
        _ = obj[{'n': 999}]
