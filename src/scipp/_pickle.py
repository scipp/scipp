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
from typing import Any, ClassVar, final

from . import __version__ as _scipp_version
from .core import (  # noqa: F401
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

    def __new__(
        cls_, cls: type[T], /, *_: Any, **__: Any
    ) -> _PickleHelper[T, S]:
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

    def __reduce__(self) -> tuple[
        Callable[[Any, Any], _PickleHelper[T, S]], tuple[Any, Any],
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

    def _get_reconstructor_and_args(self, obj: T) -> tuple[
        Callable[[Any, S], T], tuple[Any, S],
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
    Unit, serializer=methodcaller('to_dict'),
).deserializer
def _unpickle_unit(u: dict[str, Any]) -> Unit:
    # Note: this is a workaround for `Unit.from_dict()` being
    # non-pickle-able.
    return Unit.from_dict(u)


@_PickleHelper[DType, str](
    DType, deserializer=partial(getattr, DType),
).serializer
def _pickle_dtype(dt: DType) -> str:
    name = str(dt)
    if getattr(DType, name, None) != dt:
        raise AssertionError(f'{dt=!r}: not a member on the DType class')
    return name
