# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
import scipp as sc


def test_cumsum() -> None:
    var = sc.arange('xx', 4, unit='m')
    assert sc.identical(
        sc.cumsum(var), sc.array(dims=['xx'], values=[0, 1, 3, 6], unit='m')
    )
    assert sc.identical(
        sc.cumsum(var, dim='xx'), sc.array(dims=['xx'], values=[0, 1, 3, 6], unit='m')
    )
    assert sc.identical(
        sc.cumsum(var, mode='exclusive'),
        sc.array(dims=['xx'], values=[0, 0, 1, 3], unit='m'),
    )
    assert sc.identical(
        sc.cumsum(var, dim='xx', mode='exclusive'),
        sc.array(dims=['xx'], values=[0, 0, 1, 3], unit='m'),
    )
