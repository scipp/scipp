# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
import numpy as np
import scipp as sc
import scipy.ndimage
from scipp.ndimage import gaussian_filter

import pytest


def make_array2d():
    da = sc.DataArray(sc.array(dims=['x', 'y'], values=np.random.rand(100, 100)))
    da.coords['x'] = sc.linspace('x', 0.0, 10.0, num=100, unit='mm')
    da.coords['y'] = sc.linspace('y', 0.0, 50.0, num=100, unit='mm')
    return da


def test_raises_TypeError_when_output_arg_given():
    da = make_array2d()
    with pytest.raises(TypeError):
        gaussian_filter(da, sigma=1.5, output=da.values)


def test_raises_VariancesError_when_data_has_variances():
    da = make_array2d()
    da.variances = da.values
    with pytest.raises(sc.VariancesError):
        gaussian_filter(da, sigma=1.5)


def test_raises_ValueError_when_data_has_masks():
    da = make_array2d()
    da.masks['mask'] = da.coords['x'] == da.coords['x']
    with pytest.raises(ValueError):
        gaussian_filter(da, sigma=1.5)


def test_raises_TypeError_if_sigma_given_as_array_like():
    da = make_array2d()
    with pytest.raises(TypeError):
        gaussian_filter(da, sigma=[1.5, 2.5])


def test_various_options_for_sigma():
    da = make_array2d()
    gaussian_filter(da, sigma=1.5)
    gaussian_filter(da, sigma=sc.scalar(1.5, unit='mm'))
    gaussian_filter(da, sigma={'x': 1.5, 'y': 2.5})
    gaussian_filter(da, sigma={'x': sc.scalar(1.5, unit='mm'), 'y': 3.7})
    gaussian_filter(da,
                    sigma={
                        'x': sc.scalar(1.5, unit='mm'),
                        'y': sc.scalar(2.5, unit='mm')
                    })


@pytest.mark.parametrize('order', [0, 1, {'x': 0, 'y': 1}])
def test_order_is_equivalent_to_scipy_order(order):
    da = make_array2d()
    result = gaussian_filter(da, sigma=1.5, order=order)
    reference = da.copy()
    reference.values = scipy.ndimage.gaussian_filter(
        reference.values,
        sigma=1.5,
        order=order if isinstance(order, int) else order.values())
    assert sc.identical(result, reference)


@pytest.mark.parametrize('sigma', [2, 1.7, 3.33])
def test_sigma_is_equivalent_to_scipy_sigma(sigma):
    da = make_array2d()
    result = gaussian_filter(da, sigma=sigma)
    reference = da.copy()
    reference.values = scipy.ndimage.gaussian_filter(reference.values, sigma=sigma)
    assert sc.identical(result, reference)
