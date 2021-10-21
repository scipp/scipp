# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from ..core import array, bin, linspace, ones
from ..core import DataArray


def table_xyz(nrow: int) -> DataArray:
    """
    Return a 1-D data array ("table") with x, y, and z coord columns.
    """
    from numpy.random import default_rng
    rng = default_rng(seed=1234)
    x = array(dims=['row'], unit='m', values=rng.random(nrow))
    y = array(dims=['row'], unit='m', values=rng.random(nrow))
    z = array(dims=['row'], unit='m', values=rng.random(nrow))
    data = ones(dims=['row'], unit='K', shape=[nrow])
    data.values += 0.1 * rng.random(nrow)
    return DataArray(data=data, coords={'x': x, 'y': y, 'z': z})


def binned_x(nevent: int, nbin: int) -> DataArray:
    """
    Return data array binned along 1 dimension.
    """
    table = table_xyz(nevent)
    x = linspace(dim='x', unit='m', start=0.0, stop=1.0, num=nbin + 1)
    return bin(table, edges=[x])
