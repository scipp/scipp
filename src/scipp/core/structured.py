# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from collections.abc import Iterator
from typing import Any

from .._scipp.core import _element_keys, _get_elements, _set_elements
from .cpp_classes import DataArray, Dataset, DType, DTypeError, Variable


def _prop(key: str) -> property:
    def getter(self: Any) -> Variable:
        return _get_elements(self._var, key)  # type: ignore[no-any-return]

    def setter(self: Any, x: Variable) -> None:
        _set_elements(self._var, key, x)

    return property(getter, setter)


def is_structured(obj: Variable | DataArray) -> bool:
    """Check whether a variable has a structured dtype."""
    if obj.is_binned:
        data = obj.bins.constituents['data']
        if isinstance(data, Dataset):
            raise DTypeError(
                "Datasets cannot have a structured dtype, "
                "got a variable with Datasets in bins."
            )
        return is_structured(data)
    return obj.dtype in [DType.vector3, DType.linear_transform3]


class Fields:
    """Accessor for fields of a structured variable."""

    def __init__(self, keys: list[str], var: Variable) -> None:
        self._var = var
        self._keys = keys

    def __contains__(self, key: str) -> bool:
        return key in self._keys

    def __iter__(self) -> Iterator[str]:
        return self._keys.__iter__()

    def __getitem__(self, key: str) -> Variable:
        return _get_elements(self._var, key)  # type: ignore[no-any-return]

    def __setitem__(self, key: str, x: Variable) -> None:
        _set_elements(self._var, key, x)

    def keys(self) -> Iterator[str]:
        yield from self._keys

    def values(self) -> Iterator[Variable]:
        yield from (self[key] for key in self)

    def items(self) -> Iterator[tuple[str, Variable]]:
        yield from ((key, self[key]) for key in self)


# Make a new 'Fields' type to dynamically insert properties.
# This means that the return annotation has to be `Any` and not `Fields`,
# because otherwise, using the properties would result in type errors.
def _fields(obj: Variable) -> Any:
    fields = type('Fields', Fields.__bases__, dict(Fields.__dict__))
    if is_structured(obj):
        keys = _element_keys(obj)
        for key in keys:
            setattr(fields, key, _prop(key))
        return fields(keys=keys, var=obj)
    return None
