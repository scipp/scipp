# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Terence Tsang

"""Basic implementation for (un-)pickling `scipp` objects.

This is for the benefit of e.g. interoperability with `multiprocessing`.

Caveats
-------
- This is NOT meant for persisting data; users are directed to either
  `scippnexus` or the HDF5-based scheme (`scipp.serialization`) for
  that purpose.
- There is no special provision for subclasses.
- The pickle serialization should be considered opaque; the schemas'
  consistency should NOT be relied upon, esp. between `scipp` versions.
"""

from __future__ import annotations

from collections.abc import Callable
from copyreg import pickle
from functools import partial
from operator import methodcaller
from typing import Any, ClassVar, cast, final

from . import __version__ as _scipp_version
from .compat.dict import from_dict, to_dict
from .core import (
    DataArray,
    Dataset,
    DType,
    Unit,
    Variable,
)


class VersionError(RuntimeError):
    """Guard against unpickling potentially incompatible blobs.

    Our (de-)serialization implementaions make use of :func:`to_dict`
    and :func:`from_dict`, which are not necessary consistent between
    versions. Hence, fail loudly if the versions aren't consistent.
    """

    def __init__(
        self, *_: Any, expected: Any = None, got: Any = None, **__: Any
    ) -> None:
        self.expected = expected
        self.got = got
        super().__init__(self.msg)

    @property
    def msg(self) -> str | None:
        chunks: list[str] = []
        if self.expected is not None:
            chunks.append(f'expected version {self.expected}')
        if self.got is not None:
            chunks.append(f'got version {self.got}')
        if chunks:
            return '; '.join(chunks)
        return None


@final
class _PickleHelper[T, S]:
    cls: type[T]
    _to_ser: Callable[[T], S] | None
    _from_ser: Callable[[S], T] | None
    _instances: ClassVar[dict[type[Any], _PickleHelper[Any, Any]]] = {}

    def __new__(cls_, cls: type[T], /, *_: Any, **__: Any) -> _PickleHelper[T, S]:
        """Always return the same instance for the same ``cls``."""
        return cls_._instances.setdefault(cls, object.__new__(cls_))

    def __init__(
        self,
        cls: type[T],
        /,
        serializer: Callable[[T], S] | None = None,
        deserializer: Callable[[S], T] | None = None,
    ) -> None:
        self.cls = cls
        if serializer is not None:
            self._to_ser = serializer
        if deserializer is not None:
            self._from_ser = deserializer

    def __reduce__(
        self,
    ) -> tuple[
        Callable[[Any, Any], _PickleHelper[T, S]],
        tuple[Any, Any],
    ]:
        # Note: since we overrode `.__new__()`, we need a bit of elbow
        # grease to restore pickle-ability
        return partial(type(self), self.cls), (self._to_ser, self._from_ser)

    def deserialize(self, version: Any, serialization: S) -> T:
        """Check the version info before actual deserialization."""
        if version != _scipp_version:
            raise VersionError(expected=_scipp_version, got=version)
        if self._from_ser is None:
            raise AssertionError(f'No deserializer for {self.cls!r}')
        return self._from_ser(serialization)

    def serialize(self, obj: T) -> tuple[Any, S]:
        """Attach version info to the serialization."""
        if self._to_ser is None:
            raise AssertionError(f'No serializer for {self.cls!r}')
        return _scipp_version, self._to_ser(obj)

    def deserializer(self, impl: Callable[[S], T]) -> Callable[[S], T]:
        """Decorator for the deserializer implementation."""
        self._from_ser = impl
        return impl

    def serializer(self, impl: Callable[[T], S]) -> Callable[[T], S]:
        """Decorator for the serializer implementation."""
        self._to_ser = impl
        return impl

    def _get_reconstructor_and_args(
        self, obj: T
    ) -> tuple[
        Callable[[Any, S], T],
        tuple[Any, S],
    ]:
        """
        Returns
        -------
        deserializer: Callable[[Any, S], T]
            Callable for deserializing ``serialization``.
        serialization: tuple[Any, S]
            Serialization of ``obj`` as returned by :meth:`serialize`.
        """
        return self.deserialize, self.serialize(obj)

    @classmethod
    def register(cls) -> None:
        for cls_, helper in cls._instances.items():
            pickle(cls_, helper._get_reconstructor_and_args)


@_PickleHelper[Unit, dict[str, Any]](
    Unit,
    serializer=methodcaller('to_dict'),
).deserializer
def _unpickle_unit(u: dict[str, Any]) -> Unit:
    # Note: this is a workaround for `Unit.from_dict()` being
    # non-pickle-able.
    return Unit.from_dict(u)


@_PickleHelper[DType, str](
    DType,
    deserializer=partial(getattr, DType),
).serializer
def _pickle_dtype(dt: DType) -> str:
    name = str(dt)
    if getattr(DType, name, None) != dt:
        raise AssertionError(f'{dt=!r}: not a member on the DType class')
    return name


def _pickle_data(
    data: DataArray | Variable,
) -> tuple[
    dict[str, Any],
    dict[str, Any] | None,
]:
    if data.is_binned:
        raise NotImplementedError('FIXME: handle binned data')
    return to_dict(data), None


def _unpickle_data(
    serialization: tuple[dict[str, Any], dict[str, Any] | None],
) -> Any:
    dct, bins = serialization
    if bins is not None:
        raise NotImplementedError('FIXME: handle binned data')
    return from_dict(dct)


def _pickle_dataset(
    dataset: Dataset,
) -> tuple[
    dict[str, Any],
    dict[str, Any] | None,
]:
    if dataset.is_binned:
        raise NotImplementedError('FIXME: handle binned data')
    # FIXME: before we resolve #4012, let's decouple from `to_dict()`
    # for the time being
    result: dict[str, Any] = {}
    result['coords'] = {name: to_dict(c) for name, c in dataset.coords.items()}
    # We only need the data proper and the masks (if any); the coords
    # are stored above and the names are the same as the dict keys
    result['data'] = {}
    for name, d in dataset.items():
        d_dct = to_dict(d)
        pop_keys = {'name', 'coords'}
        if not d_dct.get('masks'):
            pop_keys.add('masks')
        for key in pop_keys:
            d_dct.pop(key, None)
        result['data'][name] = d_dct
    return result, None


def _unpickle_dataset(
    serialization: tuple[dict[str, Any], dict[str, Any] | None],
) -> Dataset:
    dct, bins = serialization
    if bins is not None:
        raise NotImplementedError('FIXME: handle binned data')
    coords: dict[str, Variable] = {
        name: cast(Variable, from_dict(d)) for name, d in dct['coords'].items()
    }
    data: dict[str, Variable | DataArray] = {}
    # Only explicitly create `DataArray`s if masks are needed
    for name, d in dct['data'].items():
        var = cast(Variable, from_dict(d['data']))
        if 'masks' in d:
            data[name] = DataArray(
                data=var,
                masks={name: from_dict(m) for name, m in d['masks'].items()},
                coords=coords,
            )
        else:
            data[name] = var
    return Dataset(data, coords=coords)


_VAR_HELPER = _PickleHelper[
    Variable,
    tuple[dict[str, Any], dict[str, Any] | None],
](Variable, _pickle_data, _unpickle_data)
_DA_HELPER = _PickleHelper[
    DataArray,
    tuple[dict[str, Any], dict[str, Any] | None],
](DataArray, _pickle_data, _unpickle_data)
_DS_HELPER = _PickleHelper[
    Dataset,
    tuple[dict[str, Any], dict[str, Any] | None],
](Dataset, _pickle_dataset, _unpickle_dataset)
