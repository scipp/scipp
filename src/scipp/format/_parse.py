# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen

import dataclasses
import enum


def _dataclass_with_slots(**kwargs):
    try:
        return dataclasses.dataclass(slots=True, **kwargs)
    except TypeError:
        # Fallback for Python < 3.10
        return dataclasses.dataclass(**kwargs)


class FormatType(enum.Enum):
    default = enum.auto()
    compact = enum.auto()


@_dataclass_with_slots(frozen=True)
class FormatSpec:
    format_type: FormatType
    nested: str


def parse(raw_spec: str, cls: type) -> FormatSpec:
    pieces = raw_spec.split(':', 1)
    raw_scipp_spec = pieces[0]
    nested = pieces[1] if len(pieces) == 2 else ''

    if raw_scipp_spec not in ('', 'c'):
        raise ValueError(f"Unknown format spec '{raw_spec}' for type '{cls}'")

    format_type = FormatType.default if raw_scipp_spec == '' else FormatType.compact

    return FormatSpec(format_type=format_type, nested=nested)
