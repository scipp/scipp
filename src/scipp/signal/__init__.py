# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
"""Sub-package for signal processing such as smoothing.

This subpackage provides wrappers for a subset of functions from
:py:mod:`scipy.signal`.
"""
from dataclasses import dataclass
from numpy import ndarray

from ..core import array, identical, islinspace, DataArray, Variable
from ..core import UnitError, CoordError
from ..units import one
from ..compat.wrapping import wrap1d


@dataclass
class SOS:
    """
    Second-order sections representation returned by :py:func:`scipp.signal.butter`.

    This is used as input to :py:func:`scipp.signal.sosfiltfilt`.
    """
    coord: Variable
    sos: ndarray


def _frequency(coord: Variable) -> Variable:
    if not islinspace(coord).value:
        raise CoordError("Data is not regularly sampled, cannot compute frequency.")
    return (len(coord) - 1) / (coord[-1] - coord[0])


def butter(da: DataArray, dim: str, *, N, Wn, **kwargs):
    coord = da.coords[dim]
    fs = _frequency(coord).value
    if not Wn.unit == one / coord.unit:
        raise UnitError(
            f"Critical frequency unit '{Wn.unit}' does not match sampling unit "
            f"'{one / coord.unit}'")
    import scipy.signal
    return SOS(coord=coord.copy(),
               sos=scipy.signal.butter(N=N, Wn=Wn.value, fs=fs, output='sos', **kwargs))


@wrap1d(keep_coords=True)
def sosfiltfilt(da: DataArray, dim: str, *, sos: SOS, **kwargs) -> DataArray:
    """Filter data along one dimension using cascaded second-order sections.
    """  # noqa #501
    if not identical(da.coords[dim], sos.coord):
        raise CoordError(f"Coord\n{da.coords[dim]}\nof filter dimension '{dim}' does "
                         f"not match coord\n{sos.coord}\nused for creating the "
                         "second-order sections representation by scipp.signal.butter.")
    import scipy.signal
    data = array(dims=da.dims,
                 unit=da.unit,
                 values=scipy.signal.sosfiltfilt(sos.sos, da.values, **kwargs))
    return DataArray(data=data)


__all__ = ['butter', 'sosfiltfilt']
