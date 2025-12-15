# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
"""Sub-package for signal processing such as smoothing.

This subpackage provides wrappers for a subset of functions from
:py:mod:`scipy.signal`.
"""

from collections.abc import Sequence
from dataclasses import dataclass
from typing import Any, TypeVar

import numpy.typing as npt

from ...compat.wrapping import wrap1d
from ...core import (
    CoordError,
    DataArray,
    UnitError,
    Variable,
    array,
    identical,
    islinspace,
    to_unit,
)
from ...units import one


@dataclass
class SOS:
    """
    Second-order sections representation returned by :py:func:`scipp.signal.butter`.

    This is used as input to :py:func:`scipp.signal.sosfiltfilt`.
    """

    coord: Variable
    sos: npt.NDArray[Any]

    def filtfilt(self, obj: Variable | DataArray, dim: str, **kwargs: Any) -> DataArray:
        """
        Forwards to :py:func:`scipp.signal.sosfiltfilt` with sos argument set to the SOS
        instance.
        """
        return sosfiltfilt(obj, dim=dim, sos=self, **kwargs)


def _frequency(coord: Variable) -> Variable:
    if not islinspace(coord).value:
        raise CoordError("Data is not regularly sampled, cannot compute frequency.")
    return (len(coord) - 1) / (coord[-1] - coord[0])


def butter(coord: Variable, *, N: int, Wn: Variable, **kwargs: Any) -> SOS:
    """
    Butterworth digital and analog filter design.

    Design an Nth-order digital or analog Butterworth filter and return the filter
    coefficients.

    This is intended for use with :py:func:`scipp.scipy.signal.sosfiltfilt`. See there
    for an example.

    This is a wrapper around :py:func:`scipy.signal.butter`. See there for a
    complete description of parameters. The differences are:

    - Instead of a sampling frequency fs, this wrapper takes a variable ``coord`` as
      input. The sampling frequency is then computed from this coordinate. Only data
      sampled at regular intervals are supported.
    - The critical frequency or frequencies must be provided as a variable with correct
      unit.
    - Only 'sos' output is supported.

    :seealso: :py:func:`scipp.scipy.signal.sosfiltfilt`
    """
    fs = _frequency(coord).value
    try:
        Wn = to_unit(Wn, one / coord.unit)
    except UnitError:
        raise UnitError(
            f"Critical frequency unit '{Wn.unit}' incompatible with sampling unit "
            f"'{one / coord.unit}'"
        ) from None
    import scipy.signal

    return SOS(
        coord=coord.copy(),
        sos=scipy.signal.butter(N=N, Wn=Wn.values, fs=fs, output='sos', **kwargs),
    )


@wrap1d(keep_coords=True)
def _sosfiltfilt(da: DataArray, dim: str, *, sos: SOS, **kwargs: Any) -> DataArray:
    if not identical(da.coords[dim], sos.coord):
        raise CoordError(
            f"Coord\n{da.coords[dim]}\nof filter dimension '{dim}' does "
            f"not match coord\n{sos.coord}\nused for creating the "
            "second-order sections representation by "
            "scipp.scipy.signal.butter."
        )
    import scipy.signal

    data = array(
        dims=da.dims,
        unit=da.unit,
        values=scipy.signal.sosfiltfilt(sos.sos, da.values, **kwargs),
    )
    return DataArray(data=data)


def sosfiltfilt(
    obj: Variable | DataArray, dim: str, *, sos: SOS, **kwargs: Any
) -> DataArray:
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

    .. plot:: :context: close-figs

      >>> from scipp.scipy.signal import butter, sosfiltfilt
      >>> x = sc.linspace(dim='x', start=1.1, stop=4.0, num=1000, unit='m')
      >>> y = sc.sin(x * sc.scalar(1.0, unit='rad/m'))
      >>> y += sc.sin(x * sc.scalar(400.0, unit='rad/m'))
      >>> da = sc.DataArray(data=y, coords={'x': x})
      >>> sos = butter(da.coords['x'], N=4, Wn=20 / x.unit)
      >>> out = sosfiltfilt(da, 'x', sos=sos)
      >>> sc.plot({'input':da, 'sosfiltfilt':out})

    Instead of calling sosfiltfilt the more convenient filtfilt method of
    :py:class:`scipp.scipy.signal.SOS` can be used:

      >>> out = butter(da.coords['x'], N=4, Wn=20 / x.unit).filtfilt(da, 'x')
    """
    da = (
        obj
        if isinstance(obj, DataArray)
        else DataArray(data=obj, coords={dim: sos.coord})
    )
    return _sosfiltfilt(da, dim=dim, sos=sos, **kwargs)


ArrayLike = TypeVar('ArrayLike', DataArray, Variable)


def find_peaks(
    da: ArrayLike,
    *,
    height: tuple[Variable, Variable] | Variable | None = None,
    threshold: tuple[Variable, Variable] | Variable | None = None,
    rel_height: Variable | None = None,
    **kwargs: Any,
) -> Variable:
    """
    A routine that locates "peaks" in a 1D signal.

    This is a wrapper around :py:func:`scipy.signal.find_peaks`. See there for a
    complete description of parameters.

    Returns
    --------
    :
        Indices of peaks in the signal that satisfy all given conditions.

    Examples
    --------

       >>> from scipp.scipy.signal import find_peaks
       >>> x = sc.linspace('x', -3.14, 3.14, 101, unit='rad')
       >>> y = sc.DataArray(sc.cos(5 * x), coords={'x': x})
       >>> find_peaks(y)
       <scipp.Variable> (x: 5)      int64        <no unit>  [10, 30, ..., 70, 90]

    """

    from scipy.signal import find_peaks

    if da.ndim != 1 or da.is_binned:
        raise ValueError('Can only find peaks in 1D arrays.')

    def to_numpy(v: Variable) -> Any:
        return v.value if v.ndim == 0 else v.values

    if height is not None:
        if isinstance(height, Sequence) and len(height) == 2:
            kwargs['height'] = [
                to_numpy(h.to(unit=da.unit, dtype=da.dtype)) for h in height
            ]
        else:
            kwargs['height'] = to_numpy(height.to(unit=da.unit, dtype=da.dtype))

    if threshold is not None:
        if isinstance(threshold, Sequence) and len(threshold) == 2:
            kwargs['threshold'] = [
                to_numpy(t.to(unit=da.unit, dtype=da.dtype)) for t in threshold
            ]
        else:
            kwargs['threshold'] = to_numpy(threshold.to(unit=da.unit, dtype=da.dtype))

    if rel_height is not None:
        kwargs['rel_height'] = rel_height.to(unit='dimensionless').values

    peaks, _ = find_peaks(da.values, **kwargs)

    return array(dims=da.dims, values=peaks, unit=None)


__all__ = ['butter', 'find_peaks', 'sosfiltfilt']
