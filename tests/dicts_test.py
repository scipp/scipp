# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
import numpy as np
import pytest

import scipp as sc


def make_data_array(var, **kwargs):
    return sc.DataArray(data=var, **kwargs)


def make_dataset(var, **kwargs):
    return sc.Dataset(data={'a': var}, **kwargs)


@pytest.mark.parametrize("make", [make_data_array, make_dataset])
def test_coord_setitem(make):
    var = sc.array(dims=['x'], values=np.arange(4))
    d = make(var, coords={'x': var})
    with pytest.raises(RuntimeError):
        d['x', 2:3].coords['y'] = sc.scalar(1.0)
    assert 'y' not in d.coords
    d.coords['y'] = sc.scalar(1.0)
    assert len(d.coords) == 2
    assert sc.identical(d.coords['y'], sc.scalar(1.0))


@pytest.mark.parametrize("make", [make_data_array, make_dataset])
def test_contains_coord(make):
    var = sc.array(dims=['x'], values=np.arange(4.), unit='m')
    d = make(var)
    assert 'x' not in d.coords
    d.coords['x'] = sc.scalar(1.0)
    assert 'x' in d.coords


@pytest.mark.parametrize("make", [make_data_array, make_dataset])
def test_coords_keys(make):
    var = sc.array(dims=['x'], values=np.arange(4.), unit='m')
    d = make(var)
    d.coords['x'] = sc.scalar(1.0)
    assert len(d.coords.keys()) == 1
    assert 'x' in d.coords.keys()


@pytest.mark.parametrize("make", [make_data_array, make_dataset])
def test_coords_get(make):
    var = sc.array(dims=['x'], values=np.arange(4.), unit='m')
    d = make(var)
    d.coords['x'] = sc.scalar(1.0)
    assert sc.identical(d.coords.get('x', sc.scalar(0.0)), d.coords['x'])
    assert sc.identical(d.coords.get('z', d.coords['x']), d.coords['x'])
    assert d.coords.get('z', None) is None
    assert d.coords.get('z') is None


@pytest.mark.parametrize("make", [make_data_array, make_dataset])
def test_coords_pop(make):
    var = sc.array(dims=['x'], values=np.arange(4.), unit='m')
    d = make(var)
    d.coords['x'] = sc.scalar(1.0)
    d.coords['y'] = sc.scalar(2.0)
    assert sc.identical(d.coords.pop('x'), sc.scalar(1.0))
    assert list(d.coords.keys()) == ['y']
    assert sc.identical(d.coords.pop('z', sc.scalar(3.0)), sc.scalar(3.0))
    assert list(d.coords.keys()) == ['y']
    assert sc.identical(d.coords.pop('y'), sc.scalar(2.0))
    assert len(list(d.coords.keys())) == 0


def test_attrs_pop():
    var = sc.array(dims=['x'], values=np.arange(4.), unit='m')
    d = sc.DataArray(var)
    d.attrs['x'] = sc.scalar(1.0)
    d.attrs['y'] = sc.scalar(2.0)
    assert sc.identical(d.attrs.pop('x'), sc.scalar(1.0))
    assert list(d.attrs.keys()) == ['y']
    assert sc.identical(d.attrs.pop('z', sc.scalar(3.0)), sc.scalar(3.0))
    assert list(d.attrs.keys()) == ['y']
    assert sc.identical(d.attrs.pop('y'), sc.scalar(2.0))
    assert len(list(d.attrs.keys())) == 0


def test_masks_pop():
    var = sc.array(dims=['x'], values=np.arange(4.), unit='m')
    d = sc.DataArray(var)
    d.masks['x'] = sc.scalar(True)
    d.masks['y'] = sc.scalar(False)
    assert sc.identical(d.masks.pop('x'), sc.scalar(True))
    assert list(d.masks.keys()) == ['y']
    assert sc.identical(d.masks.pop('z', sc.scalar(3.0)), sc.scalar(3.0))
    assert list(d.masks.keys()) == ['y']
    assert sc.identical(d.masks.pop('y'), sc.scalar(False))
    assert len(list(d.masks.keys())) == 0


@pytest.mark.parametrize("make", [make_data_array, make_dataset])
def test_coords_clear(make):
    var = sc.array(dims=['x'], values=np.arange(4.), unit='m')
    d = make(var)
    assert len(d.coords) == 0
    d.coords['x'] = sc.array(dims=['x'], values=3.3 * np.arange(4.), unit='m')
    d.coords['y'] = sc.array(dims=['x'], values=-51.0 * np.arange(4.), unit='m')
    assert len(d.coords) == 2
    d.coords.clear()
    assert len(d.coords) == 0


def test_attrs_clear():
    var = sc.array(dims=['x'], values=np.arange(4.), unit='m')
    d = sc.DataArray(var)
    assert len(d.attrs) == 0
    d.attrs['x'] = sc.array(dims=['x'], values=3.3 * np.arange(4.), unit='m')
    d.attrs['y'] = sc.array(dims=['x'], values=-51.0 * np.arange(4.), unit='m')
    assert len(d.attrs) == 2
    d.attrs.clear()
    assert len(d.attrs) == 0


def test_masks_clear():
    var = sc.array(dims=['x'], values=np.arange(2.), unit='m')
    d = sc.DataArray(var)
    assert len(d.masks) == 0
    d.masks['x'] = sc.array(dims=['x'], values=[True, False])
    d.masks['y'] = sc.array(dims=['x'], values=[False, True])
    assert len(d.masks) == 2
    d.masks.clear()
    assert len(d.masks) == 0


@pytest.mark.parametrize("make", [make_data_array, make_dataset])
def test_coords_update_from_dict_adds_items(make):
    var = sc.array(dims=['x'], values=np.arange(4.), unit='m')
    d = make(var)
    d.coords['a'] = sc.scalar(1.0)
    d.coords.update({'b': sc.scalar(2.0)})
    assert sc.identical(d.coords['a'], sc.scalar(1.0))
    assert sc.identical(d.coords['b'], sc.scalar(2.0))


@pytest.mark.parametrize("make", [make_data_array, make_dataset])
def test_coords_update_from_coords_adds_items(make):
    var = sc.array(dims=['x'], values=np.arange(4.), unit='m')
    d = make(var)
    d.coords['a'] = sc.scalar(1.0)
    d.coords['b'] = sc.scalar(2.0)
    other = sc.Dataset()
    other.coords['b'] = sc.scalar(3.0)
    other.coords['c'] = sc.scalar(4.0)
    d.coords.update(other.coords)
    assert sc.identical(d.coords['a'], sc.scalar(1.0))
    assert sc.identical(d.coords['b'], sc.scalar(3.0))
    assert sc.identical(d.coords['c'], sc.scalar(4.0))


@pytest.mark.parametrize("make", [make_data_array, make_dataset])
def test_coords_update_from_sequence_of_tuples_adds_items(make):
    var = sc.array(dims=['x'], values=np.arange(4.), unit='m')
    d = make(var)
    d.coords['a'] = sc.scalar(1.0)
    d.coords['b'] = sc.scalar(2.0)
    d.coords.update([('b', sc.scalar(3.0)), ('c', sc.scalar(4.0))])
    assert sc.identical(d.coords['a'], sc.scalar(1.0))
    assert sc.identical(d.coords['b'], sc.scalar(3.0))
    assert sc.identical(d.coords['c'], sc.scalar(4.0))


@pytest.mark.parametrize("make", [make_data_array, make_dataset])
def test_coords_update_from_iterable_of_tuples_adds_items(make):

    def extra_items():
        yield 'b', sc.scalar(3.0)
        yield 'c', sc.scalar(4.0)

    var = sc.array(dims=['x'], values=np.arange(4.), unit='m')
    d = make(var)
    d.coords['a'] = sc.scalar(1.0)
    d.coords['b'] = sc.scalar(2.0)
    d.coords.update(extra_items())
    assert sc.identical(d.coords['a'], sc.scalar(1.0))
    assert sc.identical(d.coords['b'], sc.scalar(3.0))
    assert sc.identical(d.coords['c'], sc.scalar(4.0))


@pytest.mark.parametrize("make", [make_data_array, make_dataset])
def test_coords_update_from_kwargs_adds_items(make):
    var = sc.array(dims=['x'], values=np.arange(4.), unit='m')
    d = make(var)
    d.coords['a'] = sc.scalar(1.0)
    d.coords['b'] = sc.scalar(2.0)
    d.coords.update(b=sc.scalar(3.0), c=sc.scalar(4.0))
    assert sc.identical(d.coords['a'], sc.scalar(1.0))
    assert sc.identical(d.coords['b'], sc.scalar(3.0))
    assert sc.identical(d.coords['c'], sc.scalar(4.0))


@pytest.mark.parametrize("make", [make_data_array, make_dataset])
def test_coords_update_from_kwargs_overwrites_other_dict(make):
    var = sc.array(dims=['x'], values=np.arange(4.), unit='m')
    d = make(var)
    d.coords['a'] = sc.scalar(1.0)
    d.coords.update({'b': sc.scalar(2.0)}, b=sc.scalar(3.0))
    assert sc.identical(d.coords['a'], sc.scalar(1.0))
    assert sc.identical(d.coords['b'], sc.scalar(3.0))


@pytest.mark.parametrize("make", [make_data_array, make_dataset])
def test_coords_update_without_args_does_nothing(make):
    var = sc.array(dims=['x'], values=np.arange(4.), unit='m')
    d = make(var)
    d.coords['a'] = sc.scalar(1.0)
    d.coords['b'] = sc.scalar(2.0)
    d.coords.update()
    assert sc.identical(d.coords['a'], sc.scalar(1.0))
    assert sc.identical(d.coords['b'], sc.scalar(2.0))


@pytest.mark.parametrize("make", [make_data_array, make_dataset])
def test_coords_view_comparison_operators(make):
    var = sc.array(dims=['x'], values=np.arange(10.), unit='m')
    d1 = make(var, coords={'x': sc.array(dims=['x'], values=np.arange(10.0))})
    d2 = make(var, coords={'x': sc.array(dims=['x'], values=np.arange(10.0))})
    assert d1.coords == d2.coords


@pytest.mark.parametrize("make", [make_data_array, make_dataset])
def test_coord_delitem(make):
    var = sc.Variable(dims=['x'], values=np.arange(4))
    d = make(var, coords={'x': var})
    dref = d.copy()
    d.coords['y'] = sc.scalar(1.0)
    assert not sc.identical(dref, d)
    del d.coords['y']
    assert sc.identical(dref, d)


@pytest.mark.parametrize("make", [make_data_array, make_dataset])
def test_coords_delitem(make):
    var = sc.Variable(dims=['x'], values=np.arange(4))
    d = make(var, coords={'x': var})
    dref = d.copy()
    dref.coords['x'] = sc.Variable(dims=['x'], values=np.arange(1, 5))
    assert not sc.identical(d, dref)
    del dref.coords['x']
    assert not sc.identical(d, dref)
    dref.coords['x'] = d.coords['x']
    assert sc.identical(d, dref)
