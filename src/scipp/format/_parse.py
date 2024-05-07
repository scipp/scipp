# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen

import dataclasses
import enum
import re


class FormatType(enum.Enum):
    default = None
    compact = 'c'

    def __str__(self) -> str:
        match self:
            case FormatType.default:
                return ''
            case FormatType.compact:
                return 'c'


class Selection(enum.Enum):
    edges = '^'
    begin = '<'
    end = '>'

    def __str__(self) -> str:
        match self:
            case Selection.edges:
                return '^'
            case Selection.begin:
                return '<'
            case Selection.end:
                return '>'


@dataclasses.dataclass(frozen=True, slots=True)
class FormatSpec:
    format_type: FormatType
    _selection: dataclasses.InitVar[str | None]
    _length: dataclasses.InitVar[str | None]
    _nested: dataclasses.InitVar[str | None]

    selection: Selection = dataclasses.field(init=False, default=Selection.edges)
    length: int = dataclasses.field(init=False, default=4)
    nested: str = dataclasses.field(init=False, default='')

    # Track whether the user specified a value or whether the default is used.
    has_selection: bool = dataclasses.field(init=False, default=False)
    has_length: bool = dataclasses.field(init=False, default=False)
    has_nested: bool = dataclasses.field(init=False, default=False)

    def __post_init__(
        self, _selection: str | None, _length: str | None, _nested: str | None
    ) -> None:
        if _selection is not None:
            object.__setattr__(self, 'selection', Selection(_selection))
            object.__setattr__(self, 'has_selection', True)
        if _length is not None:
            object.__setattr__(self, 'length', int(_length))
            object.__setattr__(self, 'has_length', True)
        if _nested is not None:
            object.__setattr__(self, 'nested', _nested)
            object.__setattr__(self, 'has_nested', True)

    def __str__(self) -> str:
        sel = str(self.selection) if self.has_selection else ''
        length = f'#{self.length}' if self.has_length else ''
        typ = str(self.format_type) if self.format_type != FormatType.default else ''
        nested = f':{self.nested}' if self.has_nested else ''
        return f'{sel}{length}{typ}{nested}'


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
