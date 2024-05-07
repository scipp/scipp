# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen

import dataclasses
import enum
import re


def _dataclass_with_slots(**kwargs):
    try:
        return dataclasses.dataclass(slots=True, **kwargs)
    except TypeError:
        # Fallback for Python < 3.10
        return dataclasses.dataclass(**kwargs)


class FormatType(enum.Enum):
    default = None
    compact = 'c'


@_dataclass_with_slots(frozen=True)
class FormatSpec:
    format_type: FormatType
    _selection: str | None
    _length: int | None
    _nested: str | None

    @property
    def selection(self) -> str:
        return '^' if self._selection is None else self._selection

    @property
    def length(self) -> int:
        return 4 if self._length is None else int(self._length)

    @property
    def nested(self) -> str:
        return '' if self._nested is None else self._nested

    @property
    def has_selection(self) -> bool:
        return self._selection is not None

    @property
    def has_length(self) -> bool:
        return self._length is not None

    @property
    def has_nested(self) -> bool:
        return self._nested is not None

    def __str__(self) -> str:
        return (
            self.selection
            if self.has_selection
            else '' + f'#{self.length}'
            if self.has_length
            else '' + str(self.format_type)
            if self.format_type != FormatType.default
            else '' + f':{self.nested}'
            if self.has_nested
            else ''
        )


_FORMAT_PATTERN = re.compile(
    '^(?P<selection>[><^])?'
    r'(?:#(?P<length>\d+))?'
    '(?P<type>[c])?'
    '(?::(?P<nested>.*))?$'
)


def parse(raw_spec: str, cls: type) -> FormatSpec:
    match = _FORMAT_PATTERN.match(raw_spec)
    if match is None:
        raise ValueError(f"Invalid format spec '{raw_spec}' for type '{cls}'")

    return FormatSpec(
        format_type=FormatType(match['type']),
        _selection=match['selection'],
        _length=match['length'],
        _nested=match['nested'],
    )
