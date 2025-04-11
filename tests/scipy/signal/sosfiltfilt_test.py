# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
import numpy as np
import pytest

import scipp as sc
from scipp.scipy.signal import butter, sosfiltfilt


def array1d_linspace(*, coord=None):
    if coord is None:
        dim = 'xx'
        size = 100
        coord = sc.linspace(dim=dim, start=-0.1, stop=4.0, num=size, unit='m')
    else:
        dim = coord.dim
        size = len(coord)
    y = coord.copy()
    y.unit = 'rad'
    y = sc.sin(y)
    rng = np.random.default_rng()
    y.values += 0.1 * rng.normal(size=size)
    return sc.DataArray(y, coords={dim: coord})


def test_raises_CoordError_if_filter_coord_incompatible() -> None:
    dim = 'xx'
    coord = sc.linspace(dim=dim, start=-0.1, stop=4.0, num=100, unit='m')
    da = array1d_linspace(coord=coord)
    sos = butter(da.coords[dim], N=4, Wn=4 / sc.Unit('m'))
    da.coords[dim].unit = 's'
    with pytest.raises(sc.CoordError):
        sosfiltfilt(da, dim, sos=sos)
    da.coords[dim] = sc.linspace(dim=dim, start=-0.1, stop=4.1, num=100, unit='m')
    with pytest.raises(sc.CoordError):
        sosfiltfilt(da, dim, sos=sos)


def test_raises_DimensionError_if_masks_along_filter_dim() -> None:
    da = array1d_linspace()
    dim = da.dim
    da = sc.concat([da, da + da], 'extra_dim')
    da.masks['conflicting_mask'] = sc.zeros(sizes=da.coords[dim].sizes, dtype='bool')
    sos = butter(da.coords[dim], N=4, Wn=4 / da.coords[dim].unit)
    with pytest.raises(sc.DimensionError):
        sosfiltfilt(da, dim, sos=sos)


def test_unrelated_masks_are_preserved() -> None:
    da = array1d_linspace()
    dim = da.dim
    da = sc.concat([da, da + da], 'extra_dim')
    mask = sc.array(dims=['extra_dim'], values=[False, True])
    da.masks['mask'] = mask.copy()
    sos = butter(da.coords[dim], N=4, Wn=4 / da.coords[dim].unit)
    out = sosfiltfilt(da, dim, sos=sos)
    assert sc.identical(out.masks['mask'], mask)


def test_output_properties_match_input_properties() -> None:
    da = array1d_linspace()
    dim = da.dim
    da = sc.concat([da, da + da], 'extra_dim')
    da.coords['extra_dim'] = sc.array(dims=['new_dim'], values=[1, 2])
    sos = butter(da.coords[dim], N=4, Wn=4 / da.coords[dim].unit)
    out = sosfiltfilt(da, dim, sos=sos)
    assert out.dims == da.dims
    assert out.unit == da.unit
    assert out.coords == da.coords


def test_low_frequency_signal_unchanged_by_lowpass_filter() -> None:
    dim = 'xx'
    x = sc.linspace(dim=dim, start=-0.1, stop=4.0, num=1000, unit='m')
    y = sc.sin(x * sc.scalar(1.0, unit='rad/m'))
    da = sc.DataArray(data=y, coords={dim: x})
    sos = butter(da.coords[dim], N=4, Wn=20 / x.unit)
    out = sosfiltfilt(da, dim, sos=sos)
    assert sc.allclose(out.data, da.data, atol=sc.scalar(0.0004))


def test_high_frequency_components_removed_by_lowpass_filter() -> None:
    dim = 'xx'
    x = sc.linspace(dim=dim, start=-0.1, stop=4.0, num=1000, unit='m')
    y = sc.sin(x * sc.scalar(1.0, unit='rad/m'))
    da = sc.DataArray(data=y, coords={dim: x})
    low_freq = da.copy()
    da += sc.sin(x * sc.scalar(400.0, unit='rad/m'))
    sos = butter(da.coords[dim], N=4, Wn=20 / x.unit)
    out = sosfiltfilt(da, dim, sos=sos)
    assert sc.allclose(out[20:-20].data, low_freq[20:-20].data, atol=sc.scalar(0.02))


def test_low_and_high_frequency_components_removed_by_bandpass_filter() -> None:
    dim = 'xx'
    x = sc.linspace(dim=dim, start=-0.1, stop=40.0, num=10000, unit='m')
    y = sc.sin(x * sc.scalar(20.0, unit='rad/m'))
    da = sc.DataArray(data=y, coords={dim: x})
    mid_freq = da.copy()
    da += sc.sin(x * sc.scalar(1.0, unit='rad/m'))
    da += sc.sin(x * sc.scalar(400.0, unit='rad/m'))
    band = sc.array(dims=['ignored'], values=[2, 10], unit=sc.units.one / x.unit)
    sos = butter(da.coords[dim], N=6, Wn=band, btype='bandpass')
    out = sosfiltfilt(da, dim, sos=sos)
    assert sc.allclose(
        out[400:-400].data, mid_freq[400:-400].data, atol=sc.scalar(0.01)
    )


def test_filtering_inner_and_outer_dimension_yields_equivalent_results() -> None:
    dim = 'xx'
    x = sc.linspace(dim=dim, start=-0.1, stop=4.0, num=1000, unit='m')
    y = sc.sin(x * sc.scalar(1.0, unit='rad/m'))
    da = sc.DataArray(data=y, coords={dim: x})
    da += sc.sin(x * sc.scalar(400.0, unit='rad/m'))
    da = sc.concat([da, da + da], 'extra_dim')
    sos = butter(da.coords[dim], N=4, Wn=20 / x.unit)
    out1 = sosfiltfilt(da, dim, sos=sos)
    out2 = sosfiltfilt(da.transpose().copy(), dim, sos=sos)
    assert sc.identical(out1, out2.transpose())


def array2d(dim, extra_dim):
    x = sc.linspace(dim=dim, start=-0.1, stop=4.0, num=1000, unit='m')
    y = sc.sin(x * sc.scalar(1.0, unit='rad/m'))
    da = sc.DataArray(data=y, coords={dim: x})
    da += sc.sin(x * sc.scalar(400.0, unit='rad/m'))
    return sc.concat([da, da + da], extra_dim)


@pytest.mark.parametrize(
    "da",
    [
        array2d(dim='xx', extra_dim='yy'),
        array2d(dim='xx', extra_dim='yy').transpose().copy(),
    ],
)
def test_filtering_slice_identical_to_slice_of_filtered(da) -> None:
    sos = butter(da.coords['xx'], N=4, Wn=20 / da.coords['xx'].unit)
    out2d = sosfiltfilt(da, 'xx', sos=sos)
    assert sc.identical(out2d['yy', 0], sosfiltfilt(da['yy', 0], 'xx', sos=sos))
    assert sc.identical(out2d['yy', 1], sosfiltfilt(da['yy', 1], 'xx', sos=sos))


def test_given_variable_uses_coord_passed_to_butter() -> None:
    da = array1d_linspace()
    sos = butter(da.coords[da.dim], N=4, Wn=4 / da.coords[da.dim].unit)
    assert sc.identical(
        sosfiltfilt(da.data, da.dim, sos=sos), sosfiltfilt(da, da.dim, sos=sos)
    )


def test_SOS_filtfilt_same_as_sosfiltfilt() -> None:
    da = array1d_linspace()
    sos = butter(da.coords[da.dim], N=4, Wn=4 / da.coords[da.dim].unit)
    assert sc.identical(sos.filtfilt(da, da.dim), sosfiltfilt(da, da.dim, sos=sos))
