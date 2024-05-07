# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

from typing import TypeVar

from ..typing import Variable

_ValueType = TypeVar('_ValueType', str, Variable)


def combine_dict_args(
    arg: dict[str, _ValueType] | None, kwargs: dict[str, _ValueType]
) -> dict[str, _ValueType]:
    pos_dict = {} if arg is None else arg

    overlapped = set(pos_dict).intersection(kwargs)
    if overlapped:
        raise ValueError(
            'The names passed in the dict and as keyword arguments must be distinct. '
            f'Following names are used in both arguments: {overlapped}'
        )

    return {**pos_dict, **kwargs}
