# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
from collections.abc import Callable, Generator
from typing import Any

import numpy as np
import pytest

import scipp as sc


def make_data_array(var: sc.Variable, **kwargs: Any) -> sc.DataArray:
    return sc.DataArray(data=var, **kwargs)


def make_dataset(var: sc.Variable, **kwargs: Any) -> sc.Dataset:
    return sc.Dataset(data={'a': var}, **kwargs)


PARAMS = [
    "make,mapping",
    [
        (make_data_array, "coords"),
        (make_data_array, "masks"),
        (make_dataset, "coords"),
    ],
]


@pytest.mark.parametrize(*PARAMS)  # type: ignore[arg-type]
def test_setitem(make: Callable[..., sc.DataArray | sc.Dataset], mapping: str) -> None:
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


@pytest.mark.parametrize(*PARAMS)  # type: ignore[arg-type]
def test_contains(make: Callable[..., sc.DataArray | sc.Dataset], mapping: str) -> None:
    var = sc.array(dims=['x'], values=np.arange(4.0), unit='m')
    d = make(var)
    mapview = getattr(d, mapping)
    assert 'x' not in mapview
    mapview['x'] = sc.scalar(1.0)
    assert 'x' in mapview


@pytest.mark.parametrize(*PARAMS)  # type: ignore[arg-type]
def test_get(make: Callable[..., sc.DataArray | sc.Dataset], mapping: str) -> None:
    var = sc.array(dims=['x'], values=np.arange(4.0), unit='m')
    d = make(var)
    mapview = getattr(d, mapping)
    mapview['x'] = sc.scalar(1.0)
    assert sc.identical(mapview.get('x', sc.scalar(0.0)), mapview['x'])
    assert sc.identical(mapview.get('z', mapview['x']), mapview['x'])
    assert mapview.get('z', None) is None
    assert mapview.get('z') is None


@pytest.mark.parametrize(*PARAMS)  # type: ignore[arg-type]
def test_pop(make: Callable[..., sc.DataArray | sc.Dataset], mapping: str) -> None:
    var = sc.array(dims=['x'], values=np.arange(4.0), unit='m')
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


@pytest.mark.parametrize(*PARAMS)  # type: ignore[arg-type]
def test_clear(make: Callable[..., sc.DataArray | sc.Dataset], mapping: str) -> None:
    var = sc.array(dims=['x'], values=np.arange(4.0), unit='m')
    d = make(var)
    mapview = getattr(d, mapping)
    assert len(mapview) == 0
    mapview['x'] = sc.array(dims=['x'], values=3.3 * np.arange(4.0), unit='m')
    mapview['y'] = sc.array(dims=['x'], values=-51.0 * np.arange(4.0), unit='m')
    assert len(mapview) == 2
    mapview.clear()
    assert len(mapview) == 0


@pytest.mark.parametrize(*PARAMS)  # type: ignore[arg-type]
def test_update_from_dict_adds_items(
    make: Callable[..., sc.DataArray | sc.Dataset], mapping: str
) -> None:
    var = sc.array(dims=['x'], values=np.arange(4.0), unit='m')
    d = make(var)
    mapview = getattr(d, mapping)
    mapview['a'] = sc.scalar(1.0)
    mapview.update({'b': sc.scalar(2.0)})
    assert sc.identical(mapview['a'], sc.scalar(1.0))
    assert sc.identical(mapview['b'], sc.scalar(2.0))


@pytest.mark.parametrize(*PARAMS)  # type: ignore[arg-type]
def test_update_from_mapping_adds_items(
    make: Callable[..., sc.DataArray | sc.Dataset], mapping: str
) -> None:
    var = sc.array(dims=['x'], values=np.arange(4.0), unit='m')
    d = make(var)
    mapview = getattr(d, mapping)
    mapview['a'] = sc.scalar(1.0)
    mapview['b'] = sc.scalar(2.0)
    other = sc.Dataset(coords={'b': sc.scalar(3.0), 'c': sc.scalar(4.0)})
    mapview.update(other.coords)
    assert sc.identical(mapview['a'], sc.scalar(1.0))
    assert sc.identical(mapview['b'], sc.scalar(3.0))
    assert sc.identical(mapview['c'], sc.scalar(4.0))


@pytest.mark.parametrize(*PARAMS)  # type: ignore[arg-type]
def test_update_from_sequence_of_tuples_adds_items(
    make: Callable[..., sc.DataArray | sc.Dataset], mapping: str
) -> None:
    var = sc.array(dims=['x'], values=np.arange(4.0), unit='m')
    d = make(var)
    mapview = getattr(d, mapping)
    mapview['a'] = sc.scalar(1.0)
    mapview['b'] = sc.scalar(2.0)
    mapview.update([('b', sc.scalar(3.0)), ('c', sc.scalar(4.0))])
    assert sc.identical(mapview['a'], sc.scalar(1.0))
    assert sc.identical(mapview['b'], sc.scalar(3.0))
    assert sc.identical(mapview['c'], sc.scalar(4.0))


@pytest.mark.parametrize(*PARAMS)  # type: ignore[arg-type]
def test_update_from_iterable_of_tuples_adds_items(
    make: Callable[..., sc.DataArray | sc.Dataset], mapping: str
) -> None:
    def extra_items() -> Generator[tuple[str, sc.Variable], None, None]:
        yield 'b', sc.scalar(3.0)
        yield 'c', sc.scalar(4.0)

    var = sc.array(dims=['x'], values=np.arange(4.0), unit='m')
    d = make(var)
    mapview = getattr(d, mapping)
    mapview['a'] = sc.scalar(1.0)
    mapview['b'] = sc.scalar(2.0)
    mapview.update(extra_items())
    assert sc.identical(mapview['a'], sc.scalar(1.0))
    assert sc.identical(mapview['b'], sc.scalar(3.0))
    assert sc.identical(mapview['c'], sc.scalar(4.0))


@pytest.mark.parametrize(*PARAMS)  # type: ignore[arg-type]
def test_update_from_kwargs_adds_items(
    make: Callable[..., sc.DataArray | sc.Dataset], mapping: str
) -> None:
    var = sc.array(dims=['x'], values=np.arange(4.0), unit='m')
    d = make(var)
    mapview = getattr(d, mapping)
    mapview['a'] = sc.scalar(1.0)
    mapview['b'] = sc.scalar(2.0)
    mapview.update(b=sc.scalar(3.0), c=sc.scalar(4.0))
    assert sc.identical(mapview['a'], sc.scalar(1.0))
    assert sc.identical(mapview['b'], sc.scalar(3.0))
    assert sc.identical(mapview['c'], sc.scalar(4.0))


@pytest.mark.parametrize(*PARAMS)  # type: ignore[arg-type]
def test_update_from_kwargs_overwrites_other_dict(
    make: Callable[..., sc.DataArray | sc.Dataset], mapping: str
) -> None:
    var = sc.array(dims=['x'], values=np.arange(4.0), unit='m')
    d = make(var)
    mapview = getattr(d, mapping)
    mapview['a'] = sc.scalar(1.0)
    mapview.update({'b': sc.scalar(2.0)}, b=sc.scalar(3.0))
    assert sc.identical(mapview['a'], sc.scalar(1.0))
    assert sc.identical(mapview['b'], sc.scalar(3.0))


@pytest.mark.parametrize(*PARAMS)  # type: ignore[arg-type]
def test_update_without_args_does_nothing(
    make: Callable[..., sc.DataArray | sc.Dataset], mapping: str
) -> None:
    var = sc.array(dims=['x'], values=np.arange(4.0), unit='m')
    d = make(var)
    mapview = getattr(d, mapping)
    mapview['a'] = sc.scalar(1.0)
    mapview['b'] = sc.scalar(2.0)
    mapview.update()
    assert sc.identical(mapview['a'], sc.scalar(1.0))
    assert sc.identical(mapview['b'], sc.scalar(2.0))


@pytest.mark.parametrize(*PARAMS)  # type: ignore[arg-type]
def test_view_comparison_operators(
    make: Callable[..., sc.DataArray | sc.Dataset], mapping: str
) -> None:
    var = sc.array(dims=['x'], values=np.arange(10.0), unit='m')
    d1 = make(var)
    getattr(d1, mapping)['x'] = sc.array(dims=['x'], values=np.arange(10.0))
    d2 = make(var)
    getattr(d2, mapping)['x'] = sc.array(dims=['x'], values=np.arange(10.0))
    assert getattr(d1, mapping) == getattr(d2, mapping)


@pytest.mark.parametrize(*PARAMS)  # type: ignore[arg-type]
def test_delitem_mapping(
    make: Callable[..., sc.DataArray | sc.Dataset], mapping: str
) -> None:
    var = sc.Variable(dims=['x'], values=np.arange(4))
    d = make(var)
    mapview = getattr(d, mapping)
    mapview['x'] = var
    dref = d.copy()
    mapview['y'] = sc.scalar(1.0)
    assert not sc.identical(dref, d)
    del mapview['y']
    assert sc.identical(dref, d)


@pytest.mark.parametrize(*PARAMS)  # type: ignore[arg-type]
def test_delitem_mappings(
    make: Callable[..., sc.DataArray | sc.Dataset], mapping: str
) -> None:
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


@pytest.mark.parametrize(*PARAMS)  # type: ignore[arg-type]
def test_copy_shallow(
    make: Callable[..., sc.DataArray | sc.Dataset], mapping: str
) -> None:
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


@pytest.mark.parametrize(*PARAMS)  # type: ignore[arg-type]
def test_copy_deep(
    make: Callable[..., sc.DataArray | sc.Dataset], mapping: str
) -> None:
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


@pytest.mark.parametrize(*PARAMS)  # type: ignore[arg-type]
def test_popitem(make: Callable[..., sc.DataArray | sc.Dataset], mapping: str) -> None:
    var = sc.array(dims=['x'], values=np.arange(4.0), unit='m')
    d = make(var)
    mapview = getattr(d, mapping)
    mapview['x'] = sc.scalar(1.0)
    mapview['y'] = sc.scalar(2.0)
    item = mapview.popitem()
    assert item[0] == 'y'
    assert sc.identical(item[1], sc.scalar(2.0))
    assert list(mapview.keys()) == ['x']
    item = mapview.popitem()
    assert item[0] == 'x'
    assert sc.identical(item[1], sc.scalar(1.0))
    assert len(list(mapview.keys())) == 0


@pytest.mark.parametrize(*PARAMS)  # type: ignore[arg-type]
def test_views_len(
    make: Callable[..., sc.DataArray | sc.Dataset], mapping: str
) -> None:
    d = make(sc.array(dims=['x'], values=np.arange(4.0), unit='m'))
    mapview = getattr(d, mapping)
    assert len(mapview.keys()) == 0
    assert len(mapview.values()) == 0
    assert len(mapview.items()) == 0

    mapview['x'] = sc.scalar(1.0)
    assert len(mapview.keys()) == 1
    assert len(mapview.values()) == 1
    assert len(mapview.items()) == 1

    mapview['a'] = sc.scalar(2.0)
    assert len(mapview.keys()) == 2
    assert len(mapview.values()) == 2
    assert len(mapview.items()) == 2


@pytest.mark.parametrize(*PARAMS)  # type: ignore[arg-type]
def test_views_convert_to_bool(
    make: Callable[..., sc.DataArray | sc.Dataset], mapping: str
) -> None:
    d = make(sc.array(dims=['x'], values=np.arange(4.0), unit='m'))
    mapview = getattr(d, mapping)
    assert not mapview.keys()
    assert not mapview.values()
    assert not mapview.items()

    mapview['x'] = sc.scalar(1.0)
    assert mapview.keys()
    assert mapview.values()
    assert mapview.items()


@pytest.mark.parametrize(*PARAMS)  # type: ignore[arg-type]
def test_keys_elements(
    make: Callable[..., sc.DataArray | sc.Dataset], mapping: str
) -> None:
    d = make(sc.array(dims=['x'], values=np.arange(4.0), unit='m'))
    mapview = getattr(d, mapping)
    assert list(mapview.keys()) == []

    mapview['x'] = sc.scalar(1.0)
    assert list(mapview.keys()) == ['x']

    mapview['a'] = sc.scalar(2.0)
    assert list(mapview.keys()) == ['x', 'a']


@pytest.mark.parametrize(*PARAMS)  # type: ignore[arg-type]
def test_values_elements(
    make: Callable[..., sc.DataArray | sc.Dataset], mapping: str
) -> None:
    d = make(sc.array(dims=['x'], values=np.arange(4.0), unit='m'))
    mapview = getattr(d, mapping)
    assert list(mapview.values()) == []

    mapview['x'] = sc.scalar(1.0)
    assert list(mapview.values()) == [sc.scalar(1.0)]

    mapview['a'] = sc.scalar(2.0)
    assert list(mapview.values()) == [sc.scalar(1.0), sc.scalar(2.0)]


@pytest.mark.parametrize(*PARAMS)  # type: ignore[arg-type]
def test_items_elements(
    make: Callable[..., sc.DataArray | sc.Dataset], mapping: str
) -> None:
    d = make(sc.array(dims=['x'], values=np.arange(4.0), unit='m'))
    mapview = getattr(d, mapping)
    assert list(mapview.items()) == []

    mapview['x'] = sc.scalar(1.0)
    assert list(mapview.items()) == [('x', sc.scalar(1.0))]

    mapview['a'] = sc.scalar(2.0)
    assert list(mapview.items()) == [('x', sc.scalar(1.0)), ('a', sc.scalar(2.0))]


@pytest.mark.parametrize(*PARAMS)  # type: ignore[arg-type]
def test_keys_equality(
    make: Callable[..., sc.DataArray | sc.Dataset], mapping: str
) -> None:
    d0 = make(sc.array(dims=['x'], values=np.arange(4.0), unit='m'))
    d1 = make(sc.array(dims=['x'], values=np.arange(4.0), unit='m'))
    mapview0 = getattr(d0, mapping)
    mapview1 = getattr(d1, mapping)
    assert mapview0.keys() == mapview0.keys()
    assert mapview0.keys() == mapview1.keys()

    mapview0['x'] = sc.scalar(1.0)
    assert mapview0.keys() == mapview0.keys()
    assert not (mapview0.keys() == mapview1.keys())
    assert mapview0.keys() != mapview1.keys()

    mapview1['x'] = sc.scalar(2.0)
    assert mapview0.keys() == mapview0.keys()
    assert mapview0.keys() == mapview1.keys()

    mapview1['y'] = sc.scalar(3.0)
    assert mapview1.keys() == mapview1.keys()
    assert not (mapview0.keys() == mapview1.keys())
    assert mapview0.keys() != mapview1.keys()

    # mapview1 has (x, y), mapview0 has (y, x)
    del mapview0['x']
    mapview0['y'] = sc.scalar(4.0)
    mapview0['x'] = sc.scalar(1.0)
    assert mapview0.keys() == mapview0.keys()
    assert mapview0.keys() == mapview1.keys()


@pytest.mark.parametrize(*PARAMS)  # type: ignore[arg-type]
def test_values_equality(
    make: Callable[..., sc.DataArray | sc.Dataset], mapping: str
) -> None:
    d0 = make(sc.array(dims=['x'], values=np.arange(4.0), unit='m'))
    d1 = make(sc.array(dims=['x'], values=np.arange(4.0), unit='m'))
    mapview0 = getattr(d0, mapping)
    mapview1 = getattr(d1, mapping)
    assert mapview0.values() != mapview0.values()
    assert mapview0.values() != mapview1.values()

    mapview0['x'] = sc.scalar(1.0)
    assert mapview0.values() != mapview0.values()
    assert not (mapview0.values() == mapview1.values())
    assert mapview0.values() != mapview1.values()

    mapview1['y'] = sc.scalar(1.0)
    assert mapview1.values() != mapview1.values()
    assert not (mapview0.values() == mapview1.values())
    assert mapview0.values() != mapview1.values()


@pytest.mark.parametrize(*PARAMS)  # type: ignore[arg-type]
def test_equality(make: Callable[..., sc.DataArray | sc.Dataset], mapping: str) -> None:
    d0 = make(sc.array(dims=['x'], values=np.arange(4.0), unit='m'))
    d1 = make(sc.array(dims=['x'], values=np.arange(4.0), unit='m'))
    mapview0 = getattr(d0, mapping)
    mapview1 = getattr(d1, mapping)
    assert mapview0 == mapview0
    assert mapview0 == mapview1

    mapview0['x'] = sc.scalar(1.0)
    assert mapview0 == mapview0
    assert not (mapview0 == mapview1)
    assert mapview0 != mapview1

    mapview1['x'] = sc.scalar(2.0)
    assert not (mapview0 == mapview1)
    assert mapview0 != mapview1

    mapview1['x'] = sc.scalar(1.0)
    assert mapview0 == mapview1

    mapview1['y'] = sc.scalar(3.0)
    assert mapview1 == mapview1
    assert not (mapview0 == mapview1)
    assert mapview0 != mapview1

    # mapview0 has (x, y), mapview1 has (y, x)
    del mapview0['x']
    mapview0['y'] = sc.scalar(4.0)
    mapview0['x'] = sc.scalar(1.0)
    assert mapview0 == mapview0
    assert mapview0 != mapview1

    mapview0['y'] = sc.scalar(3.0)
    assert mapview0 == mapview1


@pytest.mark.parametrize(*PARAMS)  # type: ignore[arg-type]
def test_items_equality(
    make: Callable[..., sc.DataArray | sc.Dataset], mapping: str
) -> None:
    d0 = make(sc.array(dims=['x'], values=np.arange(4.0), unit='m'))
    d1 = make(sc.array(dims=['x'], values=np.arange(4.0), unit='m'))
    mapview0 = getattr(d0, mapping)
    mapview1 = getattr(d1, mapping)
    assert mapview0.items() == mapview0.items()
    assert mapview0.items() == mapview1.items()

    mapview0['x'] = sc.scalar(1.0)
    assert mapview0.items() == mapview0.items()
    assert not (mapview0.items() == mapview1.items())
    assert mapview0.items() != mapview1.items()

    mapview1['x'] = sc.scalar(2.0)
    assert not (mapview0.items() == mapview1.items())
    assert mapview0.items() != mapview1.items()

    mapview1['x'] = sc.scalar(1.0)
    assert mapview0.items() == mapview1.items()

    mapview1['y'] = sc.scalar(3.0)
    assert mapview1.items() == mapview1.items()
    assert not (mapview0.items() == mapview1.items())
    assert mapview0.items() != mapview1.items()

    # mapview0 has (x, y), mapview1 has (y, x)
    del mapview0['x']
    mapview0['y'] = sc.scalar(4.0)
    mapview0['x'] = sc.scalar(1.0)
    assert mapview0.items() == mapview0.items()
    assert mapview0.items() != mapview1.items()

    mapview0['y'] = sc.scalar(3.0)
    assert mapview0.items() == mapview1.items()


@pytest.mark.parametrize(
    ('make', 'mapping'),
    [
        (make_data_array, "coords"),
        (make_dataset, "coords"),
    ],
)
def test_set_aligned(
    make: Callable[..., sc.DataArray | sc.Dataset], mapping: str
) -> None:
    d = make(sc.arange('x', 4.0, unit='m'))
    mapview = getattr(d, mapping)
    mapview['x'] = sc.scalar(1.0)
    mapview['y'] = sc.scalar(2.0)

    assert mapview['x'].aligned
    assert mapview['y'].aligned

    mapview.set_aligned('x', False)
    assert not mapview['x'].aligned
    assert mapview['y'].aligned

    mapview.set_aligned('x', True)
    assert mapview['x'].aligned
    assert mapview['y'].aligned
