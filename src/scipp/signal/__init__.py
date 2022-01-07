# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
"""Signal processing based on :py:module:`scipy.signal`"""

from ..core import array, islinspace, DataArray, UnitError
from ..units import one
from ..compat.wrapping import wrap1d


# TODO
# - like this there is no guarantee that sosfilt is applied to data with same coord
# - Rename/replace fs argument? Can we pass in `da` and `dim`?
def butter(N, Wn, *, fs, output='sos', **kwargs):
    if not islinspace(fs).value:
        raise ValueError("Data is not regularly sampled.")
    if not Wn.unit == one / fs.unit:
        raise UnitError(
            f"Critical frequency unit '{Wn.unit}' does not match sampling unit "
            f"'{one / fs.unit}'"
        )
    fs = (len(fs) - 1) / (fs[-1] - fs[0]).value
    import scipy.signal
    return scipy.signal.butter(N=N, Wn=Wn.value, fs=fs, output=output, **kwargs)


@wrap1d(keep_coords=True)
def sosfiltfilt(da: DataArray, dim: str, *, sos, **kwargs) -> DataArray:
    """Filter data along one dimension using cascaded second-order sections.
    """  # noqa #501
    import scipy.signal
    data = array(dims=da.dims,
                 unit=da.unit,
                 values=scipy.signal.sosfiltfilt(sos, da.values, **kwargs))
    return DataArray(data=data)


__all__ = ['butter', 'sosfiltfilt']
