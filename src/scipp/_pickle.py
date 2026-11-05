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
- The pickle serialization should be considered opaque; the schemas'
  consistency should NOT be relied upon, esp. between `scipp` versions.
"""
from collections.abc import (  # noqa: F401
    Callable, Mapping, Sequence,
)
from copyreg import pickle
from functools import partial
from typing import Any, Concatenate

from . import __version__  # noqa: F401
from .core import Variable, DataArray, Dataset, DType, Unit


def _pickle_unit[U: Unit](u: U) -> tuple[Callable[[str], U], tuple[str]]:
    return _reconstruct_unit, (type(u), u.to_dict())


def _reconstruct_unit[U: Unit](
    UnitClass: type[U], unit_dict: Mapping[str, Any],
) -> U:
    """
    Note
    ----
    This mostly exists to get around :meth:`Unit.from_dict`'s being
    non-pickle-able.
    """
    return UnitClass.from_dict(unit_dict)


def _pickle_dtype[D: DType](d: D) -> (
    tuple[Callable[[str], D], tuple[str]] | tuple[Callable[[], D], tuple[()]]
):
    name = str(d)
    cls = type(d)
    if getattr(cls, name, None) == d:
        return partial(getattr, cls, name), ()
    # Fallback
    return cls, (name,)


def _register() -> None:
    pickle(Unit, _pickle_unit)
    pickle(DType, _pickle_dtype)
