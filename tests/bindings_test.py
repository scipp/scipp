# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 Scipp contributors (https://github.com/scipp)
"""Binding behavior affected by the switch from pybind11 to nanobind."""

import pickle
import weakref
from collections.abc import Iterator
from typing import Generic, TypeVar

import numpy as np
import pytest

import scipp as sc


def test_bytes_are_accepted_as_str() -> None:
    da = sc.DataArray(sc.ones(sizes={b'x': 1}), coords={b'c': sc.scalar(1, unit=b'm')})
    da.name = bytearray(b'a')
    assert da.dims == ('x',)
    assert da.name == 'a'
    assert b'c' in da.coords
    assert sc.identical(da.coords[b'c'], sc.scalar(1, unit='m'))


@pytest.mark.parametrize('index', [1, True, np.int64(1), sc.index(1), np.float32(1.5)])
def test_positional_index_conversion(index: object) -> None:
    var = sc.arange('x', 4)
    assert sc.identical(var[index], var['x', 1])


def test_selector_items_are_read_by_position() -> None:
    class Selector:
        def __len__(self) -> int:
            return 2

        def __getitem__(self, i: int) -> object:
            return ('x', 1)[i]

        def __iter__(self) -> Iterator[str]:
            yield 'x'  # Disagrees with len() and indexing.

    var = sc.arange('x', 4)
    assert sc.identical(var[Selector()], var['x', 1])


class IntOnly:
    def __int__(self) -> int:
        return 4


@pytest.mark.parametrize(
    ('operand', 'expected'),
    [(True, 2), (np.float32(1.5), 2.5), (2**70, 2.0**70), (IntOnly(), 5)],
)
def test_scalar_operands(operand: object, expected: float) -> None:
    assert sc.identical(sc.scalar(1) + operand, sc.scalar(expected))


def test_sequence_arguments() -> None:
    sizes = {'x': 2, 'y': 1}
    expected = sc.zeros(sizes=sizes)
    assert sc.identical(sc.zeros(dims=sizes.keys(), shape=sizes.values()), expected)
    assert sc.identical(sc.squeeze(expected, (d for d in {'y'})), expected['y', 0])

    def dims():
        yield 'y'
        raise LookupError('cannot read dims')

    with pytest.raises(LookupError, match='cannot read dims'):
        sc.squeeze(expected, dims())


def test_none_arguments() -> None:
    var = sc.array(dims=['x'], values=[1.0, 2.0])
    assert np.shares_memory(var.copy(deep=None).values, var.values)
    var['x', 0] = None
    assert np.isnan(var.values[0])
    objects = sc.array(dims=['x'], values=[{}, {}], dtype=sc.DType.PyObject)
    objects.values[0] = None
    assert objects.values[0] is None


@pytest.mark.parametrize('value', [0.0, 0.5, np.inf, np.nan])
def test_bool_scalar_from_float_uses_truth_value(value: float) -> None:
    assert sc.scalar(value, dtype=bool).value == bool(value)


def test_weakrefs_and_hashing() -> None:
    da = sc.DataArray(sc.scalar(1.0), coords={'x': sc.scalar(2.0)})
    objects = (da.data, da, da.coords, da.coords.keys(), sc.DType.float64)
    for obj in (*objects, sc.Dataset({'a': da}), sc.units.m):
        assert weakref.ref(obj)() is obj
    for obj in objects:
        with pytest.raises(TypeError):
            hash(obj)


def test_inplace_operators_return_self() -> None:
    class Tagged(sc.Variable):
        pass

    var = alias = Tagged(dims=(), values=2.0)
    var += 1
    var **= 2
    assert var is alias
    assert var.value == 9.0


def test_readonly_element_value_is_readonly() -> None:
    matrix = sc.spatial.linear_transform(value=np.arange(9.0).reshape(3, 3))
    element = sc.broadcast(matrix, dims=['x'], shape=[2])['x', 0].value
    assert not element.flags.writeable
    np.testing.assert_array_equal(element, matrix.value)


@pytest.mark.parametrize('dtype', [str, object])
def test_element_view_index_out_of_range(dtype: type) -> None:
    view = sc.array(dims=['x'], values=['a', 'b', 'c'], dtype=dtype)['x', 1:].values
    view[-1] = 'd'
    assert (view[0], view[1]) == ('b', 'd')
    for i in (-3, 2):
        with pytest.raises(IndexError):
            _ = view[i]
        with pytest.raises(IndexError):
            view[i] = 'e'


def test_failed_implicit_conversion_is_silent(capfd: pytest.CaptureFixture) -> None:
    assert sc.units.m != 'not a unit'
    assert capfd.readouterr().err == ''


@pytest.mark.parametrize('mapping', ['coords', 'masks'])
def test_iterated_metadata_follows_its_mapping(mapping: str) -> None:
    meta = getattr(sc.DataArray(sc.ones(sizes={'x': 2})), mapping)
    meta['a'] = sc.array(dims=['x'], values=[True, True])
    value, keyed = next(iter(meta.values())), meta['a']
    _, item = next(iter(meta.items()))
    assert value is next(iter(meta.values()))
    assert value in meta.values()
    with pytest.raises(sc.DimensionError):
        _ = keyed in meta.values()  # Not the iterated object, so compared by value.
    if mapping == 'coords':
        meta.set_aligned('a', False)
        assert not value.aligned
        assert keyed.aligned
    replacement = sc.array(dims=['x'], values=[False, True])
    meta['a'] = replacement
    meta['b'] = sc.scalar(False)  # May reallocate the storage.
    assert sc.identical(value, replacement)
    del meta['a']
    assert sc.identical(value, replacement)
    assert sc.identical(item, replacement if mapping == 'masks' else keyed)


@pytest.mark.parametrize('mode', [None, 'previous'])
def test_lookup_pickles(mode: str | None) -> None:
    x = sc.array(dims=['x'], values=[0.0, 1.0, 2.0])
    table = sc.DataArray(sc.array(dims=['x'], values=[1, 2]), coords={'x': x})
    if mode is not None:
        table.coords['x'] = x[:-1]
    lut = sc.lookup(table, mode=mode)
    query = sc.array(dims=['e'], values=[0.5, 1.5])
    assert sc.identical(pickle.loads(pickle.dumps(lut))[query], lut[query])  # noqa: S301


Value = TypeVar('Value')


class Scope(Generic[Value]):
    """Like sciline.Scope, whose specialized constructor returns its argument."""

    def __new__(cls, value: Value) -> Value:
        return value


@pytest.mark.parametrize(
    'obj', [sc.scalar(1), sc.DataArray(sc.scalar(1)), sc.Dataset({'a': sc.scalar(1)})]
)
def test_python_subclass_with_custom_new(obj: object) -> None:
    class Scoped(Scope[Value], type(obj)):
        pass

    assert Scoped[int](obj) is obj
