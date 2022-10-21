# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen
"""Unit helpers and predefined units.

The following predefined units are available:

Dimensionless (those two are aliases for each other):
 - dimensionless
 - one

Common units:
 - angstrom
 - counts
 - deg
 - kg
 - K
 - meV
 - m
 - rad
 - s
 - us
 - ns
 - mm

Special:
 - default_unit (used by some functions to deduce a unit)

.. seealso::
    :py:class:`scipp.Unit` to construct other units.
"""

from contextlib import contextmanager
from typing import Union

from .._scipp.core.units import (angstrom, counts, default_unit, deg, dimensionless, kg,
                                 K, meV, m, one, rad, s, us, ns, mm, add_unit_alias,
                                 remove_unit_alias, clear_unit_aliases)
from ..core.cpp_classes import Unit


@contextmanager
def unit_aliases(**aliases: Union[str, Unit]):
    for name, unit in aliases.items():
        add_unit_alias(name=name, unit=unit)
    yield
    for name in aliases:
        remove_unit_alias(name=name)


__all__ = [
    'angstrom', 'counts', 'default_unit', 'deg', 'dimensionless', 'kg', 'K', 'meV', 'm',
    'one', 'rad', 's', 'us', 'ns', 'mm', 'add_unit_alias', 'remove_unit_alias',
    'clear_unit_aliases', 'unit_aliases'
]
