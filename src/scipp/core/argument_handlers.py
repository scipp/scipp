# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025 Scipp contributors (https://github.com/scipp)

from collections.abc import Iterable, Mapping
from typing import TypeAlias, TypeVar

_V = TypeVar('_V')
IntoStrDict: TypeAlias = Mapping[str, _V] | Iterable[tuple[str, _V]]


def combine_dict_args(
    arg: IntoStrDict[_V] | None, kwargs: Mapping[str, _V]
) -> dict[str, _V]:
    pos_dict = {} if arg is None else dict(arg)

    overlapped = set(pos_dict).intersection(kwargs)
    if overlapped:
        raise TypeError(
            'The names passed in the dict and as keyword arguments must be distinct. '
            f'Following names are used in both arguments: {overlapped}'
        )

    return {**pos_dict, **kwargs}
