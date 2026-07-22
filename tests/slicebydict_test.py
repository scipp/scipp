# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
import itertools
from typing import ClassVar

import numpy as np
import pytest
import scipp as sc

# Base shape (N, M, K, L, S) with named dims.
_N, _M, _K, _L, _S = 2, 3, 4, 5, 6
_DIMS = ('n', 'm', 'k', 'l', 's')
_SHAPE = (_N, _M, _K, _L, _S)


_VALUES = np.arange(np.prod(_SHAPE), dtype=float).reshape(_SHAPE)


def _make_variable() -> sc.Variable:
    return sc.array(dims=list(_DIMS), values=_VALUES)


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


# --- Label-based (coordinate-value) dict indexing ---
# Variable has no coordinates so is excluded from these tests.

# Needed both for DATA['float']['coords'] and DATA['float']['label_index']
# so defined here to avoid a forward-reference inside the class body dict literal.
_float_coord_values = {d: np.linspace(0.1, 0.9, s) for d, s in zip(_DIMS, _SHAPE, strict=True)}


class TestSliceByLabelDict:
    # All per-variant data is nested under 'float'/'str' so that label_variants can
    # be built with a comprehension over DATA.items() — the comprehension body only
    # has access to loop variables, not bare class-attribute names.
    DATA: ClassVar[dict] = {
        'float': {
            'coords': {
                d: sc.array(dims=[d], values=v, unit='m')
                for d, v in _float_coord_values.items()
            },
            'label_index': {
                'n': sc.scalar(_float_coord_values['n'][0], unit='m'),
                'm': sc.scalar(_float_coord_values['m'][1], unit='m'),
                's': sc.scalar(_float_coord_values['s'][4], unit='m'),
            },
            'expected_values': _VALUES[0, 1, :, :, 4],
        },
        'str': {
            'coords': {
                d: sc.array(dims=[d], values=list('abcdefghijklmnopqrstuvwxyz')[:s])
                for d, s in zip(_DIMS, _SHAPE, strict=True)
            },
            'label_index': {
                'n': sc.scalar('b'),
                'm': sc.scalar('c'),
                's': sc.scalar('e'),
            },
            'expected_values': _VALUES[1, 2, :, :, 4],
        },
    }

    # lambda coords=data['coords'] captures the per-iteration coords dict via default
    # argument: without it, all lambdas would share a closure over the loop variable and
    # see the last iteration's value after the comprehension finishes.
    label_variants: ClassVar[dict] = {
        coord_type: {
            'make': {
                'DataArray': (
                    lambda coords=data['coords']: sc.DataArray(
                        _make_variable(),
                        coords=coords,
                    )
                ),
                'Dataset': (
                    lambda coords=data['coords']: sc.Dataset({'a': sc.DataArray(
                        _make_variable(),
                        coords=coords,
                    )}
                )),
            },
            'label_index': data['label_index'],
            'expected_values': data['expected_values'],
        }
        for coord_type, data in DATA.items()
    }

    label_index_params: ClassVar[list] = [
        pytest.param(
            variant['make'][obj_type],
            variant['label_index'],
            variant['expected_values'],
            id=f'{obj_type}-{coord_type}',
        )
        for (coord_type, variant), obj_type in itertools.product(
            label_variants.items(), ['DataArray', 'Dataset']
        )
    ]

    @staticmethod
    def _extract_values(obj):
        return obj['a'].values if isinstance(obj, sc.Dataset) else obj.values

    @pytest.mark.parametrize(
        ('make_obj', 'label_index', 'expected_values'),
        label_index_params
    )
    def test_getitem_scalar_label_dict(self, make_obj, label_index, expected_values):
        obj = make_obj()
        result = obj[label_index]
        assert result.dims == ('k', 'l')
        assert np.array_equal(self._extract_values(result), expected_values)

    @pytest.mark.parametrize(
        ('make_obj', 'label_index', 'expected_values'),
        label_index_params
    )
    def test_setitem_scalar_label_dict(self, make_obj, label_index, expected_values):
        obj = make_obj()
        obj[label_index] = obj[label_index] * 0.0
        values = self._extract_values(obj[label_index])
        assert values.shape == (_K, _L)
        assert np.all(values == 0.0)
