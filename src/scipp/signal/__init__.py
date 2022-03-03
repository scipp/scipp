# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
"""Sub-package for signal processing such as smoothing.

This subpackage provides wrappers for a subset of functions from
:py:mod:`scipy.signal`.
"""
from dataclasses import dataclass
from numpy import ndarray
from typing import Union

from ..core import array, identical, islinspace, to_unit, DataArray, Variable
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

    def filtfilt(self, obj: Union[Variable, DataArray], dim: str,
                 **kwargs) -> DataArray:
        """
        Forwards to :py:func:`scipp.signal.sosfiltfilt` with sos argument set to the SOS
        instance.
        """
        return sosfiltfilt(obj, dim=dim, sos=self, **kwargs)


def _frequency(coord: Variable) -> Variable:
    if not islinspace(coord).value:
        raise CoordError("Data is not regularly sampled, cannot compute frequency.")
    return (len(coord) - 1) / (coord[-1] - coord[0])


def butter(coord: Variable, *, N: int, Wn: Variable, **kwargs) -> SOS:
    """
    Butterworth digital and analog filter design.

    Design an Nth-order digital or analog Butterworth filter and return the filter
    coefficients.

    This is intended for use with :py:func:`scipp.signal.sosfiltfilt`. See there for
    an example.

    This is a wrapper around :py:func:`scipy.signal.butter`. See there for a
    complete description of parameters. The differences are:

    - Instead of a sampling frequency fs, this wrapper takes a variable ``coord`` as
      input. The sampling frequency is then computed from this coordinate. Only data
      sampled at regular intervals are supported.
    - The critical frequency or frequencies must be provided as a variable with correct
      unit.
    - Only 'sos' output is supported.

    :seealso: :py:func:`scipp.signal.sosfiltfilt`
    """
    fs = _frequency(coord).value
    try:
        Wn = to_unit(Wn, one / coord.unit)
    except UnitError:
        raise UnitError(
            f"Critical frequency unit '{Wn.unit}' incompatible with sampling unit "
            f"'{one / coord.unit}'")
    import scipy.signal
    return SOS(coord=coord.copy(),
               sos=scipy.signal.butter(N=N, Wn=Wn.values, fs=fs, output='sos',
                                       **kwargs))


@wrap1d(keep_coords=True)
def _sosfiltfilt(da: DataArray, dim: str, *, sos: SOS, **kwargs) -> DataArray:
    if not identical(da.coords[dim], sos.coord):
        raise CoordError(f"Coord\n{da.coords[dim]}\nof filter dimension '{dim}' does "
                         f"not match coord\n{sos.coord}\nused for creating the "
                         "second-order sections representation by scipp.signal.butter.")
    import scipy.signal
    data = array(dims=da.dims,
                 unit=da.unit,
                 values=scipy.signal.sosfiltfilt(sos.sos, da.values, **kwargs))
    return DataArray(data=data)


def sosfiltfilt(obj: Union[Variable, DataArray], dim: str, *, sos: SOS,
                **kwargs) -> DataArray:
    """
    A forward-backward digital filter using cascaded second-order sections.

    This is a wrapper around :py:func:`scipy.signal.sosfiltfilt`. See there for a
    complete description of parameters. The differences are:

    - Instead of an array ``x`` and an optional axis, the input must be a data array
      (or variable) and dimension label. If it is a variable, the coord used for setting
      up the second-order sections is used as output coordinate.
    - The array of second-order filter coefficients ``sos`` includes the coordinate
      used for computing the frequency used for creating the coefficients. This is
      compared to the corresponding coordinate of the input data array to ensure that
      compatible coefficients are used.

    Examples:

      >>> from scipp.signal import butter, sosfiltfilt
      >>> x = sc.linspace(dim='x', start=1.1, stop=4.0, num=1000, unit='m')
      >>> y = sc.sin(x * sc.scalar(1.0, unit='rad/m'))
      >>> y += sc.sin(x * sc.scalar(400.0, unit='rad/m'))
      >>> da = sc.DataArray(data=y, coords={'x': x})
      >>> sos = butter(da.coords['x'], N=4, Wn=20 / x.unit)
      >>> out = sosfiltfilt(da, 'x', sos=sos)

    Instead of calling sosfiltfilt the more convenient filtfilt method of
    :py:class:`scipp.signal.SOS` can be used:

      >>> out = butter(da.coords['x'], N=4, Wn=20 / x.unit).filtfilt(da, 'x')
    """
    da = obj if isinstance(obj, DataArray) else DataArray(data=obj,
                                                          coords={dim: sos.coord})
    return _sosfiltfilt(da, dim=dim, sos=sos, **kwargs)


__all__ = ['butter', 'sosfiltfilt']
