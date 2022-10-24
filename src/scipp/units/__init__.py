# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen
"""Unit helpers and predefined units.

The following predefined units are available:

Dimensionless (those two names are equivalent):
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
from typing import Dict, Iterable, Tuple, Union

from .._scipp.core.units import (angstrom, counts, default_unit, deg, dimensionless, kg,
                                 K, meV, m, one, rad, s, us, ns, mm)

from .._scipp.core import (add_unit_alias as _add_unit_alias, clear_unit_aliases as
                           _clear_unit_aliases)
from ..core.cpp_classes import Unit


class UnitAliases:
    """Manager for unit aliases.

    Aliases override how units are converted to and from strings.
    The table is similar to a :class:`dict` and maps alias names to units.
    But unlike a dict, no guarantees are made about the order of aliases or their
    priority in string formatting.
    And there may be only one alias for each unit at a time.

    Attention
    ---------
    This class is a singleton and should never be instantiated by user code.
    Instead, use it through :attr:`scipp.units.aliases`.
    """

    def __init__(self):
        if any(map(lambda x: isinstance(x, UnitAliases), globals().values())):
            raise RuntimeError("There can be only one instance of _Aliases")
        self._aliases: Dict[str, Unit] = {}

    def __setitem__(self, alias: str, unit: Union[str, Unit]):
        """Define a new unit alias."""
        unit = unit if isinstance(unit, Unit) else Unit(unit)
        if unit in self.values():
            raise ValueError(f"There already is an alias for unit '{unit!r}'")
        self._aliases[alias] = unit
        _add_unit_alias(name=alias, unit=unit)

    def __delitem__(self, alias: str):
        """Remove an existing alias."""
        del self._aliases[alias]
        _clear_unit_aliases()
        for name, unit in self._aliases.items():
            _add_unit_alias(name=name, unit=unit)

    def clear(self):
        """Remove all aliases."""
        self._aliases.clear()
        _clear_unit_aliases()

    @contextmanager
    def temporary(self, **kwargs: Union[str, Unit]):
        """Contextmanager to define temporary aliases.

        Defines new aliases based on ``kwargs`` for the duration of the context.
        When exiting the context, all temporary aliases are removed.

        It is possible to define additional aliases in the context.
        They are not removed when the context manager exits unless they override
        temporary aliases. (See examples.)

        Warning
        -------
        This context manager is not thread-safe.
        Aliases defined here affect all threads and other threads can define different
        aliases which affect the managed context.

        Parameters
        ----------
        **kwargs
            Map from names to units for aliases to define.

        Examples
        --------
        Define temporary aliases:

          >>> with sc.units.aliases.temporary(speed='m/s'):
          ...     str(sc.Unit('m/s'))
          'speed'

        Previously defined aliases still apply:

          >>> sc.units.aliases.clear()
          >>> sc.units.aliases['dogyear'] = '4492800s'
          >>> with sc.units.aliases.temporary(speed='m/s'):
          ...     str(sc.Unit('4492800s'))
          'dogyear'

        Previous aliases can be overriden and are restored after the context:

          >>> sc.units.aliases.clear()
          >>> sc.units.aliases['speed'] = 'km/s'
          >>> with sc.units.aliases.temporary(speed='m/s'):
          ...     sc.Unit('speed') == 'm/s'
          True
          >>> sc.Unit('speed') == 'km/s'
          True

        Aliases defined within the context remain active
        unless they clash with previous alises:

          >>> sc.units.aliases.clear()
          >>> sc.units.aliases['speed'] = 'km/s'
          >>> with sc.units.aliases.temporary(speed='m/s'):
          ...     sc.units.aliases['speed'] = 'mm/s'
          ...     sc.units.aliases['dogyear'] = '4492800s'
          >>> str(sc.Unit('4492800s'))
          'dogyear'
          >>> sc.Unit('speed') == 'km/s'
          True
        """
        overridden = {
            name: unit
            for name, unit in self._aliases.items() if name in kwargs
        }
        for name, unit in kwargs.items():
            self[name] = unit
        yield
        for name in kwargs:
            del self[name]
        for name, unit in overridden.items():
            self[name] = unit

    def __iter__(self) -> Iterable[str]:
        """Iterator over alias names."""
        yield from self.keys()

    def keys(self) -> Iterable[str]:
        """Iterator over alias names."""
        yield from self._aliases.keys()

    def values(self) -> Iterable[Unit]:
        """Iterator over aliased units."""
        yield from self._aliases.values()

    def items(self) -> Iterable[Tuple[str, Unit]]:
        """Iterator over pairs of alias names and units."""
        yield from self._aliases.items()

    # Making copies would allow _Alias's internal map and
    # LLNL/Unit's global map to get out of sync.
    def __copy__(self):
        raise TypeError("UnitAliases is a singleton and must not be copied")

    def __deepcopy__(self):
        raise TypeError("UnitAliases is a singleton and must not be copied")


aliases = UnitAliases()
"""Table of unit aliases."""

__all__ = [
    'angstrom', 'counts', 'default_unit', 'deg', 'dimensionless', 'kg', 'K', 'meV', 'm',
    'one', 'rad', 's', 'us', 'ns', 'mm'
]
