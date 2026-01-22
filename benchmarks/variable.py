# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
from typing import ClassVar

import numpy as np

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


class AssignFromNumpy:
    """
    Benchmark assigning values from numpy arrays to scipp Variables.
    """

    params: ClassVar[list[int]] = [10**5, 10**6, 10**7, 10**8]
    param_names: ClassVar[list[str]] = ['size']

    def setup(self, size: int) -> None:
        self.array = np.arange(size, dtype=np.float64)
        self.var = sc.Variable(dims=['x'], values=self.array)

    def time_assign_1d(self, size: int) -> None:
        self.var.values = self.array
