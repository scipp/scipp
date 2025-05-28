# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
import scipp as sc


class Variable:
    """
    Benchmark different aspects of a Scipp variable
    """

    def setup(self) -> None:
        self.var1 = sc.array(dims=['x'], values=[1, 2, 3, 4, 5])
        self.var2 = sc.array(dims=['x'], values=[1, 2, 3, 4, 5])

    def time_shallow_copy(self) -> None:
        self.var1.copy(deep=False)

    def time_deep_copy(self) -> None:
        self.var1.copy()

    def time_variable_inplace_operation(self) -> None:
        self.var1 += self.var2

    def time_variable_non_inplace_operation(self) -> None:
        self.var1 + self.var2
