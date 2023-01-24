# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
import numpy as np
import pytest

import scipp as sc


def make_data_array(var, **kwargs):
    return sc.DataArray(data=var, **kwargs)


def make_dataset(var, **kwargs):
    return sc.Dataset(data={'a': var}, **kwargs)


PARAMS = [
    "make,mapping",
    [(make_data_array, "coords"), (make_data_array, "masks"),
     (make_data_array, "attrs"), (make_dataset, "coords")]
]


@pytest.mark.parametrize(*PARAMS)
def test_setitem(make, mapping):
    var = sc.array(dims=['x'], values=np.arange(4))
    d = make(var)
    mapview = getattr(d, mapping)
    mapview['x'] = var
    with pytest.raises(RuntimeError):
        getattr(d['x', 2:3], mapping)['y'] = sc.scalar(1.0)
    assert 'y' not in mapview
    mapview['y'] = sc.scalar(1.0)
    assert len(mapview) == 2
    assert sc.identical(mapview['y'], sc.scalar(1.0))


@pytest.mark.parametrize(*PARAMS)
def test_contains(make, mapping):
    var = sc.array(dims=['x'], values=np.arange(4.), unit='m')
    d = make(var)
    mapview = getattr(d, mapping)
    assert 'x' not in mapview
    mapview['x'] = sc.scalar(1.0)
    assert 'x' in mapview


@pytest.mark.parametrize(*PARAMS)
def test_keys(make, mapping):
    var = sc.array(dims=['x'], values=np.arange(4.), unit='m')
    d = make(var)
    mapview = getattr(d, mapping)
    mapview['x'] = sc.scalar(1.0)
    assert len(mapview.keys()) == 1
    assert 'x' in mapview.keys()


@pytest.mark.parametrize(*PARAMS)
def test_get(make, mapping):
    var = sc.array(dims=['x'], values=np.arange(4.), unit='m')
    d = make(var)
    mapview = getattr(d, mapping)
    mapview['x'] = sc.scalar(1.0)
    assert sc.identical(mapview.get('x', sc.scalar(0.0)), mapview['x'])
    assert sc.identical(mapview.get('z', mapview['x']), mapview['x'])
    assert mapview.get('z', None) is None
    assert mapview.get('z') is None


@pytest.mark.parametrize(*PARAMS)
def test_pop(make, mapping):
    var = sc.array(dims=['x'], values=np.arange(4.), unit='m')
    d = make(var)
    mapview = getattr(d, mapping)
    mapview['x'] = sc.scalar(1.0)
    mapview['y'] = sc.scalar(2.0)
    assert sc.identical(mapview.pop('x'), sc.scalar(1.0))
    assert list(mapview.keys()) == ['y']
    assert sc.identical(mapview.pop('z', sc.scalar(3.0)), sc.scalar(3.0))
    assert list(mapview.keys()) == ['y']
    assert sc.identical(mapview.pop('y'), sc.scalar(2.0))
    assert len(list(mapview.keys())) == 0


@pytest.mark.parametrize(*PARAMS)
def test_clear(make, mapping):
    var = sc.array(dims=['x'], values=np.arange(4.), unit='m')
    d = make(var)
    mapview = getattr(d, mapping)
    assert len(mapview) == 0
    mapview['x'] = sc.array(dims=['x'], values=3.3 * np.arange(4.), unit='m')
    mapview['y'] = sc.array(dims=['x'], values=-51.0 * np.arange(4.), unit='m')
    assert len(mapview) == 2
    mapview.clear()
    assert len(mapview) == 0


@pytest.mark.parametrize(*PARAMS)
def test_update_from_dict_adds_items(make, mapping):
    var = sc.array(dims=['x'], values=np.arange(4.), unit='m')
    d = make(var)
    mapview = getattr(d, mapping)
    mapview['a'] = sc.scalar(1.0)
    mapview.update({'b': sc.scalar(2.0)})
    assert sc.identical(mapview['a'], sc.scalar(1.0))
    assert sc.identical(mapview['b'], sc.scalar(2.0))


@pytest.mark.parametrize(*PARAMS)
def test_update_from_mapping_adds_items(make, mapping):
    var = sc.array(dims=['x'], values=np.arange(4.), unit='m')
    d = make(var)
    mapview = getattr(d, mapping)
    mapview['a'] = sc.scalar(1.0)
    mapview['b'] = sc.scalar(2.0)
    other = sc.Dataset()
    other.coords['b'] = sc.scalar(3.0)
    other.coords['c'] = sc.scalar(4.0)
    mapview.update(other.coords)
    assert sc.identical(mapview['a'], sc.scalar(1.0))
    assert sc.identical(mapview['b'], sc.scalar(3.0))
    assert sc.identical(mapview['c'], sc.scalar(4.0))


@pytest.mark.parametrize(*PARAMS)
def test_update_from_sequence_of_tuples_adds_items(make, mapping):
    var = sc.array(dims=['x'], values=np.arange(4.), unit='m')
    d = make(var)
    mapview = getattr(d, mapping)
    mapview['a'] = sc.scalar(1.0)
    mapview['b'] = sc.scalar(2.0)
    mapview.update([('b', sc.scalar(3.0)), ('c', sc.scalar(4.0))])
    assert sc.identical(mapview['a'], sc.scalar(1.0))
    assert sc.identical(mapview['b'], sc.scalar(3.0))
    assert sc.identical(mapview['c'], sc.scalar(4.0))


@pytest.mark.parametrize(*PARAMS)
def test_update_from_iterable_of_tuples_adds_items(make, mapping):

    def extra_items():
        yield 'b', sc.scalar(3.0)
        yield 'c', sc.scalar(4.0)

    var = sc.array(dims=['x'], values=np.arange(4.), unit='m')
    d = make(var)
    mapview = getattr(d, mapping)
    mapview['a'] = sc.scalar(1.0)
    mapview['b'] = sc.scalar(2.0)
    mapview.update(extra_items())
    assert sc.identical(mapview['a'], sc.scalar(1.0))
    assert sc.identical(mapview['b'], sc.scalar(3.0))
    assert sc.identical(mapview['c'], sc.scalar(4.0))


@pytest.mark.parametrize(*PARAMS)
def test_update_from_kwargs_adds_items(make, mapping):
    var = sc.array(dims=['x'], values=np.arange(4.), unit='m')
    d = make(var)
    mapview = getattr(d, mapping)
    mapview['a'] = sc.scalar(1.0)
    mapview['b'] = sc.scalar(2.0)
    mapview.update(b=sc.scalar(3.0), c=sc.scalar(4.0))
    assert sc.identical(mapview['a'], sc.scalar(1.0))
    assert sc.identical(mapview['b'], sc.scalar(3.0))
    assert sc.identical(mapview['c'], sc.scalar(4.0))


@pytest.mark.parametrize(*PARAMS)
def test_update_from_kwargs_overwrites_other_dict(make, mapping):
    var = sc.array(dims=['x'], values=np.arange(4.), unit='m')
    d = make(var)
    mapview = getattr(d, mapping)
    mapview['a'] = sc.scalar(1.0)
    mapview.update({'b': sc.scalar(2.0)}, b=sc.scalar(3.0))
    assert sc.identical(mapview['a'], sc.scalar(1.0))
    assert sc.identical(mapview['b'], sc.scalar(3.0))


@pytest.mark.parametrize(*PARAMS)
def test_update_without_args_does_nothing(make, mapping):
    var = sc.array(dims=['x'], values=np.arange(4.), unit='m')
    d = make(var)
    mapview = getattr(d, mapping)
    mapview['a'] = sc.scalar(1.0)
    mapview['b'] = sc.scalar(2.0)
    mapview.update()
    assert sc.identical(mapview['a'], sc.scalar(1.0))
    assert sc.identical(mapview['b'], sc.scalar(2.0))


@pytest.mark.parametrize(*PARAMS)
def test_view_comparison_operators(make, mapping):
    var = sc.array(dims=['x'], values=np.arange(10.), unit='m')
    d1 = make(var)
    getattr(d1, mapping)['x'] = sc.array(dims=['x'], values=np.arange(10.0))
    d2 = make(var)
    getattr(d2, mapping)['x'] = sc.array(dims=['x'], values=np.arange(10.0))
    assert getattr(d1, mapping) == getattr(d2, mapping)


@pytest.mark.parametrize(*PARAMS)
def test_delitem_mapping(make, mapping):
    var = sc.Variable(dims=['x'], values=np.arange(4))
    d = make(var)
    mapview = getattr(d, mapping)
    mapview['x'] = var
    dref = d.copy()
    mapview['y'] = sc.scalar(1.0)
    assert not sc.identical(dref, d)
    del mapview['y']
    assert sc.identical(dref, d)


@pytest.mark.parametrize(*PARAMS)
def test_delitem_mappings(make, mapping):
    var = sc.Variable(dims=['x'], values=np.arange(4))
    d = make(var)
    mapview = getattr(d, mapping)
    mapview['x'] = var
    dref = d.copy()
    getattr(dref, mapping)['x'] = sc.Variable(dims=['x'], values=np.arange(1, 5))
    assert not sc.identical(d, dref)
    del getattr(dref, mapping)['x']
    assert not sc.identical(d, dref)
    getattr(dref, mapping)['x'] = mapview['x']
    assert sc.identical(d, dref)


@pytest.mark.parametrize(*PARAMS)
def test_copy_shallow(make, mapping):
    var = sc.Variable(dims=['x'], values=np.arange(4), unit='m')
    d = make(var)
    mapview = getattr(d, mapping)
    mapview['x'] = var.copy(deep=True)
    new_mapping = mapview.copy(deep=False)
    new_mapping['y'] = sc.scalar(3.0)
    assert 'y' in new_mapping
    assert 'y' not in mapview
    assert mapview['x'].unit == 'm'
    assert new_mapping['x'].unit == 'm'
    mapview['x'].unit = 's'
    assert new_mapping['x'].unit == 's'


@pytest.mark.parametrize(*PARAMS)
def test_copy_deep(make, mapping):
    var = sc.Variable(dims=['x'], values=np.arange(4), unit='m')
    d = make(var)
    mapview = getattr(d, mapping)
    mapview['x'] = var.copy(deep=True)
    new_mapping = mapview.copy(deep=True)
    new_mapping['y'] = sc.scalar(3.0)
    assert 'y' in new_mapping
    assert 'y' not in mapview
    assert mapview['x'].unit == 'm'
    assert new_mapping['x'].unit == 'm'
    mapview['x'].unit = 's'
    assert new_mapping['x'].unit == 'm'


@pytest.mark.parametrize(*PARAMS)
def test_popitem(make, mapping):
    var = sc.array(dims=['x'], values=np.arange(4.), unit='m')
    d = make(var)
    mapview = getattr(d, mapping)
    mapview['x'] = sc.scalar(1.0)
    mapview['y'] = sc.scalar(2.0)
    assert sc.identical(mapview.popitem(), sc.scalar(2.0))
    assert list(mapview.keys()) == ['x']
    assert sc.identical(mapview.popitem(), sc.scalar(1.0))
    assert len(list(mapview.keys())) == 0
