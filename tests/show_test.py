# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import numpy as np
import pytest

import scipp as sc


def test_large_variable() -> None:
    for n in [10, 100, 1000, 10000]:
        var = sc.zeros(dims=['x', 'y'], shape=(n, n))
    assert len(sc.make_svg(var)) < 100000


def test_too_many_variable_dimensions() -> None:
    var = sc.zeros(dims=['x', 'y', 'z', 'time'], shape=(1, 1, 1, 1))
    with pytest.raises(RuntimeError):
        sc.make_svg(var)


def test_too_many_dataset_dimensions() -> None:
    d = sc.Dataset(
        data={
            'a': sc.zeros(dims=['x', 'y', 'z', 'time'], shape=(1, 1, 1, 1)),
        }
    )
    with pytest.raises(RuntimeError):
        sc.make_svg(d)


def test_with_sliced_bin_edge_coords() -> None:
    da = sc.DataArray(
        data=sc.array(dims=['z', 'y', 'x'], values=np.random.random((5, 6, 7))),
        coords={
            'x': sc.arange('x', 8.0),
            'y': sc.arange('y', 7.0),
            'z': sc.arange('z', 6.0),
        },
    )
    sc.make_svg(da['z', 0])
