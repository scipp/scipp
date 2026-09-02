# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
import numpy as np
import pytest

import scipp as sc


def make_variable() -> sc.Variable:
    return sc.array(
        dims=['x', 'y', 'z'], values=np.arange(24.0).reshape(2, 3, 4), unit='counts'
    )


def make_data_array() -> sc.DataArray:
    da = sc.DataArray(make_variable())
    da.coords['x'] = sc.array(dims=['x'], values=[0.1, 0.2], unit='m')
    da.coords['y'] = sc.array(dims=['y'], values=[1.0, 2.0, 3.0], unit='s')
    da.coords['z'] = sc.array(dims=['z'], values=['a', 'b', 'c', 'd'])
    return da


def make_dataset() -> sc.Dataset:
    return sc.Dataset({'a': make_data_array()})


make_obj_params = [
    pytest.param(make_variable, id='Variable'),
    pytest.param(make_data_array, id='DataArray'),
    pytest.param(make_dataset, id='Dataset'),
]

make_coord_obj_params = [
    pytest.param(make_data_array, id='DataArray'),
    pytest.param(make_dataset, id='Dataset'),
]

index_params = [
    pytest.param({'x': 0}, id='int'),
    pytest.param({'y': slice(0, 2)}, id='slice'),
    pytest.param({'x': 1, 'z': -1}, id='int-int'),
    pytest.param({'z': slice(1, None), 'x': 0, 'y': 2}, id='mixed'),
    pytest.param({'x': 0, 'y': 1, 'z': 2}, id='scalar-output'),
]


def chained(obj, index_dict):
    for dim, index in index_dict.items():
        obj = obj[dim, index]
    return obj


@pytest.mark.parametrize('make_obj', make_obj_params)
@pytest.mark.parametrize('index_dict', index_params)
def test_getitem_dict_equivalent_to_chained_slicing(make_obj, index_dict):
    obj = make_obj()
    assert sc.identical(obj[index_dict], chained(obj, index_dict))


@pytest.mark.parametrize('make_obj', make_obj_params)
def test_getitem_dict_result_is_independent_of_dict_order(make_obj):
    obj = make_obj()
    assert sc.identical(
        obj[{'x': 0, 'z': slice(1, 3)}], obj[{'z': slice(1, 3), 'x': 0}]
    )


@pytest.mark.parametrize('make_obj', make_obj_params)
def test_getitem_dict_returns_view(make_obj):
    obj = make_obj()
    view = obj[{'x': 0, 'y': 1}]
    view *= 0.0
    assert sc.identical(obj[{'x': 0, 'y': 1}], view)


@pytest.mark.parametrize('make_obj', make_obj_params)
def test_getitem_empty_dict(make_obj):
    obj = make_obj()
    result = obj[{}]
    assert sc.identical(result, obj)
    assert result is not obj


@pytest.mark.parametrize('make_obj', make_obj_params)
def test_getitem_dict_with_numpy_integer(make_obj):
    obj = make_obj()
    assert sc.identical(obj[{'x': np.int64(1)}], obj['x', 1])


@pytest.mark.parametrize('make_obj', make_obj_params)
def test_getitem_dict_with_integer_array_returns_copy(make_obj):
    obj = make_obj()
    result = obj[{'y': [2, 0], 'x': 1}]
    assert sc.identical(result, obj['x', 1]['y', [2, 0]])
    result *= 0.0
    assert not sc.identical(obj[{'y': [2, 0], 'x': 1}], result)


@pytest.mark.parametrize('make_obj', make_obj_params)
def test_getitem_dict_bad_dim_raises(make_obj):
    obj = make_obj()
    with pytest.raises(sc.DimensionError):
        obj[{'nonexistent': 0}]


@pytest.mark.parametrize('make_obj', make_obj_params)
def test_getitem_dict_out_of_bounds_raises(make_obj):
    obj = make_obj()
    with pytest.raises(IndexError):
        obj[{'x': 999}]


@pytest.mark.parametrize('make_obj', make_obj_params)
def test_getitem_dict_float_index_raises(make_obj):
    obj = make_obj()
    with pytest.raises(TypeError):
        obj[{'x': 0.5}]


@pytest.mark.parametrize('make_obj', make_obj_params)
def test_getitem_dict_non_string_key_raises(make_obj):
    obj = make_obj()
    with pytest.raises(TypeError):
        obj[{0: 0}]


@pytest.mark.parametrize('make_obj', make_obj_params)
@pytest.mark.parametrize('index_dict', index_params)
def test_setitem_dict(make_obj, index_dict):
    obj = make_obj()
    expected = make_obj()
    obj[index_dict] = obj[index_dict] * 2.0
    view = chained(expected, index_dict)
    view *= 2.0
    assert sc.identical(obj, expected)


@pytest.mark.parametrize('make_obj', make_obj_params)
def test_setitem_empty_dict(make_obj):
    obj = make_obj()
    obj[{}] = obj * 2.0
    assert sc.identical(obj, make_obj() * 2.0)


@pytest.mark.parametrize('make_obj', make_obj_params)
def test_setitem_dict_with_integer_array_raises(make_obj):
    obj = make_obj()
    with pytest.raises(TypeError):
        obj[{'y': [0, 2], 'x': 0}] = obj[{'y': [0, 2], 'x': 0}]


def test_setitem_dict_from_numpy_values():
    var = make_variable()
    var[{'x': 0, 'z': 2}] = np.zeros(3)
    assert sc.identical(
        var['x', 0]['z', 2], sc.zeros(dims=['y'], shape=[3], unit='counts')
    )


@pytest.mark.parametrize('make_obj', make_coord_obj_params)
def test_getitem_dict_with_labels(make_obj):
    obj = make_obj()
    index = {'x': sc.scalar(0.2, unit='m'), 'z': sc.scalar('c')}
    assert sc.identical(obj[index], obj['x', 1]['z', 2])


@pytest.mark.parametrize('make_obj', make_coord_obj_params)
def test_getitem_dict_with_label_range(make_obj):
    obj = make_obj()
    index = {'y': slice(sc.scalar(1.0, unit='s'), sc.scalar(3.0, unit='s')), 'x': 0}
    expected = obj['x', 0]['y', sc.scalar(1.0, unit='s') : sc.scalar(3.0, unit='s')]
    assert sc.identical(obj[index], expected)


@pytest.mark.parametrize('make_obj', make_coord_obj_params)
def test_setitem_dict_with_labels(make_obj):
    obj = make_obj()
    index = {'x': sc.scalar(0.2, unit='m'), 'y': sc.scalar(2.0, unit='s')}
    obj[index] = obj[index] * 0.0
    assert sc.identical(obj[index], make_obj()[index] * 0.0)


def test_getitem_dict_with_label_on_variable_raises():
    var = make_variable()
    with pytest.raises(sc.DimensionError):
        var[{'x': sc.scalar(0.1, unit='m')}]


def test_datagroup_getitem_dict():
    dg = sc.DataGroup({'v': make_variable(), 'da': make_data_array()})
    result = dg[{'x': 0, 'y': slice(1, 3)}]
    assert sc.identical(result['v'], dg['v']['x', 0]['y', 1:3])
    assert sc.identical(result['da'], dg['da']['x', 0]['y', 1:3])
