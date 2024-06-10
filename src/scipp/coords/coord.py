# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen

import dataclasses

from ..core import Variable


@dataclasses.dataclass
class Coord:
    dense: Variable | None  # for dense variable or bin-coord
    event: Variable | None
    aligned: bool
    usages: int = -1  # negative for unlimited usages

    @property
    def has_dense(self) -> bool:
        return self.dense is not None

    @property
    def has_event(self) -> bool:
        return self.event is not None

    def use(self):
        if self.usages > 0:
            self.usages -= 1

    @property
    def used_up(self) -> bool:
        return self.usages == 0

    def has_dim(self, dim: str) -> bool:
        return dim in (self.dense.dims if self.has_dense else self.event.dims)
