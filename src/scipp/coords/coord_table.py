# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen

import dataclasses
from collections.abc import Generator, Iterable

from .coord import Coord
from .options import Options
from .rule import FetchRule, RenameRule, Rule, rule_output_names


class CoordTable:
    """
    Stores a dictionary of coordinates for use in coord transforms.

    Coords have an associated number of usages.
    When that number drops to 0, the coord is removed.
    """

    def __init__(self, rules: list[Rule], targets: set[str], options: Options):
        self._coords: dict[str, Coord] = {}
        self._total_usages = _apply_keep_options(
            _count_usages(rules), rules, targets, options
        )
        # Preserve all targets regardless of keep_* options.
        for name in targets:
            self._total_usages[name] = -1

    def add(self, name: str, coord: Coord) -> None:
        self._coords[name] = dataclasses.replace(coord, usages=self.total_usages(name))

    def consume(self, name: str) -> Coord:
        coord = self._coords[name]
        coord.aligned = False
        coord.use()
        if coord.usages == 0:
            # The coord's data is no longer needed in the table.
            # But the caller of `consume` does need it, so return `coord` as is.
            self._coords[name] = dataclasses.replace(coord, dense=None, event=None)
        return coord

    def total_usages(self, name: str) -> int:
        return self._total_usages.get(name, -1)

    def items(self) -> Iterable[tuple[str, Coord]]:
        yield from self._coords.items()


def _count_usages(rules: list[Rule]) -> dict[str, int]:
    usages: dict[str, int] = {}
    for rule in rules:
        for name in rule.dependencies:
            usages.setdefault(name, 0)
            usages[name] += 1
    return usages


def _apply_keep_options(
    usages: dict[str, int], rules: list[Rule], targets: set[str], options: Options
) -> dict[str, int]:
    def out_names(rule_type: type) -> Generator[str, None, None]:
        yield from filter(
            lambda name: name not in targets, rule_output_names(rules, rule_type)
        )

    def handle_in(names: set[str]) -> None:
        for name in names:
            usages[name] = -1

    inputs = set(out_names(FetchRule))
    aliases = set(out_names(RenameRule))
    all_inputs = {dep for rule in rules for dep in rule.dependencies}
    if options.keep_inputs:
        handle_in(inputs)
    if options.keep_intermediate:
        handle_in(all_inputs - inputs - aliases)
    if options.keep_aliases:
        handle_in(aliases)
    return usages
