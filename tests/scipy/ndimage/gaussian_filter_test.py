# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
import numpy as np
import pytest
import scipy.ndimage

import scipp as sc
from scipp.scipy.ndimage import gaussian_filter


def make_histogram2d():
    da = sc.DataArray(sc.array(dims=['x', 'y'], values=np.random.rand(100, 100)))
    da.coords['x'] = sc.linspace('x', 0.0, 10.0, num=101, unit='mm')
    da.coords['y'] = sc.linspace('y', 0.0, 50.0, num=101, unit='mm')
    return da


def make_data2d():
    da = sc.DataArray(sc.array(dims=['x', 'y'], values=np.random.rand(100, 100)))
    da.coords['x'] = sc.arange('x', 0.0, 10.0, 0.1, unit='mm')
    da.coords['y'] = sc.arange('y', 0.0, 50.0, 0.5, unit='mm')
    return da


def test_raises_TypeError_when_output_arg_given() -> None:
    da = make_histogram2d()
    with pytest.raises(TypeError):
        gaussian_filter(da, sigma=1.5, output=da.values)


def test_raises_VariancesError_when_data_has_variances() -> None:
    da = make_histogram2d()
    da.variances = da.values
    with pytest.raises(sc.VariancesError):
        gaussian_filter(da, sigma=1.5)


def test_raises_ValueError_when_data_has_masks() -> None:
    da = make_histogram2d()
    da.masks['mask'] = da.coords['x'] == da.coords['x']
    with pytest.raises(ValueError, match='mask'):
        gaussian_filter(da, sigma=1.5)


def test_raises_TypeError_if_sigma_given_as_array_like() -> None:
    da = make_histogram2d()
    with pytest.raises(TypeError):
        gaussian_filter(da, sigma=[1.5, 2.5])


def test_raises_KeyError_if_sigma_has_missing_labels() -> None:
    da = make_histogram2d()
    with pytest.raises(KeyError):
        gaussian_filter(da, sigma={'x': 1.5})


def test_raises_KeyError_if_sigma_has_extra_labels() -> None:
    da = make_histogram2d()
    with pytest.raises(KeyError):
        gaussian_filter(da, sigma={'x': 1.5, 'y': 1.5, 'z': 1.5})


def test_raises_KeyError_if_order_has_missing_labels() -> None:
    da = make_histogram2d()
    with pytest.raises(KeyError):
        gaussian_filter(da, sigma=4, order={'x': 0})


def test_raises_KeyError_if_order_has_extra_labels() -> None:
    da = make_histogram2d()
    with pytest.raises(KeyError):
        gaussian_filter(da, sigma=4, order={'x': 0, 'y': 0, 'z': 0})


def test_raises_CoordError_with_label_based_sigma_if_coord_is_not_linspace() -> None:
    da = make_histogram2d()
    da.coords['x'][-1] *= 1.1
    with pytest.raises(sc.CoordError):
        gaussian_filter(da, sigma=sc.scalar(1.2, unit='mm'))


def test_raises_KeyError_with_label_based_sigma_if_coord_missing() -> None:
    da = make_histogram2d()
    del da.coords['y']
    with pytest.raises(KeyError):
        gaussian_filter(da, sigma=sc.scalar(1.2, unit='mm'))


def test_input_with_non_linspace_coord_accepted_if_sigma_is_positional() -> None:
    da = make_histogram2d()
    da.coords['x'][-1] *= 1.1
    gaussian_filter(da, sigma=3.4)


def test_input_with_missing_coord_accepted_if_sigma_is_positional() -> None:
    da = make_histogram2d()
    del da.coords['x']
    gaussian_filter(da, sigma=3.4)


def test_sigma_accepts_mixed_label_based_and_positional_param() -> None:
    da = make_histogram2d()
    gaussian_filter(da, sigma={'x': sc.scalar(1.5, unit='mm'), 'y': 3.7})


def test_label_based_sigma_equivalent_to_positional_sigma_given_bin_edge_coord() -> (
    None
):
    da = make_histogram2d()
    result = gaussian_filter(
        da, sigma={'x': sc.scalar(1.0, unit='mm'), 'y': sc.scalar(5.0, unit='mm')}
    )
    reference = gaussian_filter(da, sigma=10)
    assert sc.identical(result, reference)


def test_label_based_sigma_equivalent_to_positional_sigma() -> None:
    da = make_data2d()
    result = gaussian_filter(
        da, sigma={'x': sc.scalar(1.0, unit='mm'), 'y': sc.scalar(5.0, unit='mm')}
    )
    reference = gaussian_filter(da, sigma=10)
    assert sc.identical(result, reference)


@pytest.mark.parametrize('order', [0, 1, {'x': 0, 'y': 1}])
def test_order_is_equivalent_to_scipy_order(order) -> None:
    da = make_histogram2d()
    result = gaussian_filter(da, sigma=1.5, order=order)
    reference = da.copy()
    reference.values = scipy.ndimage.gaussian_filter(
        reference.values,
        sigma=1.5,
        order=order if isinstance(order, int) else order.values(),
    )
    assert sc.identical(result, reference)


@pytest.mark.parametrize('sigma', [2, 1.7, 3.33])
def test_sigma_is_equivalent_to_scipy_sigma(sigma) -> None:
    da = make_histogram2d()
    result = gaussian_filter(da, sigma=sigma)
    reference = da.copy()
    reference.values = scipy.ndimage.gaussian_filter(reference.values, sigma=sigma)
    assert sc.identical(result, reference)


def test_coordinates_are_propagated() -> None:
    da = make_histogram2d()
    result = gaussian_filter(da, sigma=3.4)
    assert set(result.coords) == {'x', 'y'}
    assert sc.identical(result.coords['x'], da.coords['x'])
    assert sc.identical(result.coords['y'], da.coords['y'])


def test_input_is_not_modified() -> None:
    original = make_histogram2d()
    da = original.copy()
    gaussian_filter(da, sigma=3.4)
    assert sc.identical(da, original)
