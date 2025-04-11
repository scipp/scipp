# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
import numpy as np
import pytest
import scipy.ndimage

import scipp as sc
from scipp.scipy.ndimage import (
    generic_filter,
    maximum_filter,
    median_filter,
    minimum_filter,
    percentile_filter,
    rank_filter,
)

filters = (
    generic_filter,
    maximum_filter,
    median_filter,
    minimum_filter,
    percentile_filter,
    rank_filter,
)


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


@pytest.fixture(
    scope="module",
    params=[
        generic_filter,
        maximum_filter,
        median_filter,
        minimum_filter,
        percentile_filter,
        rank_filter,
    ],
)
def filter_func(request):
    return request.param


@pytest.fixture(scope="module", params=[maximum_filter, median_filter, minimum_filter])
def simple_filter_func(request):
    return request.param


def test_raises_TypeError_when_output_arg_given(filter_func) -> None:
    da = make_histogram2d()
    with pytest.raises(TypeError):
        filter_func(da, size=2, output=da.values)


def test_raises_VariancesError_when_data_has_variances(filter_func) -> None:
    da = make_histogram2d()
    da.variances = da.values
    with pytest.raises(sc.VariancesError):
        filter_func(da, size=2)


def test_raises_ValueError_when_data_has_masks(filter_func) -> None:
    da = make_histogram2d()
    da.masks['mask'] = da.coords['x'] == da.coords['x']
    with pytest.raises(ValueError, match='mask'):
        filter_func(da, size=2)


def test_raises_TypeError_if_size_given_as_array_like(filter_func) -> None:
    da = make_histogram2d()
    with pytest.raises(TypeError):
        filter_func(da, size=[2, 3])


def test_raises_TypeError_if_origin_given_as_array_like(filter_func) -> None:
    da = make_histogram2d()
    with pytest.raises(TypeError):
        filter_func(da, size=[2, 3])


def test_raises_TypeError_if_size_is_not_integral(filter_func) -> None:
    da = make_histogram2d()
    with pytest.raises(TypeError):
        filter_func(da, size=1.5)
    with pytest.raises(TypeError):
        filter_func(da, size={'x': 2.5, 'y': 3.5})


def test_raises_TypeError_if_origin_is_not_integral(filter_func) -> None:
    da = make_histogram2d()
    with pytest.raises(TypeError):
        filter_func(da, size=3, origin=1.5)
    with pytest.raises(TypeError):
        filter_func(da, size=3, origin={'x': 1.5, 'y': 0.5})


def test_raises_KeyError_if_size_has_missing_labels(filter_func) -> None:
    da = make_histogram2d()
    with pytest.raises(KeyError):
        filter_func(da, size={'x': 4})


def test_raises_KeyError_if_size_has_extra_labels(filter_func) -> None:
    da = make_histogram2d()
    with pytest.raises(KeyError):
        filter_func(da, size={'x': 4, 'y': 4, 'z': 4})


def test_raises_KeyError_if_origin_has_missing_labels(filter_func) -> None:
    da = make_histogram2d()
    with pytest.raises(KeyError):
        filter_func(da, size=4, origin={'x': -1})


def test_raises_KeyError_if_origin_has_extra_labels(filter_func) -> None:
    da = make_histogram2d()
    with pytest.raises(KeyError):
        filter_func(da, size=4, origin={'x': -1, 'y': -1, 'z': -1})


def test_raises_CoordError_with_label_based_size_or_origin_if_coord_not_linspace(
    filter_func,
):
    da = make_histogram2d()
    da.coords['x'][-1] *= 1.1
    with pytest.raises(sc.CoordError):
        filter_func(da, size=sc.scalar(1.2, unit='mm'))
    with pytest.raises(sc.CoordError):
        filter_func(da, size=2, origin=sc.scalar(1.2, unit='mm'))


def test_raises_KeyError_with_label_based_size_if_coord_missing(filter_func) -> None:
    da = make_histogram2d()
    del da.coords['y']
    with pytest.raises(KeyError):
        filter_func(da, size=sc.scalar(1.2, unit='mm'))
    with pytest.raises(KeyError):
        filter_func(da, size=2, origin=sc.scalar(1.2, unit='mm'))


def test_input_with_non_linspace_coord_accepted_if_size_is_positional(
    simple_filter_func,
):
    da = make_histogram2d()
    da.coords['x'][-1] *= 1.1
    simple_filter_func(da, size=3)


def test_input_with_missing_coord_accepted_if_size_is_positional(
    simple_filter_func,
) -> None:
    da = make_histogram2d()
    del da.coords['x']
    simple_filter_func(da, size=3)


def test_size_accepts_mixed_label_based_and_positional_param(
    simple_filter_func,
) -> None:
    da = make_histogram2d()
    simple_filter_func(da, size={'x': sc.scalar(1.5, unit='mm'), 'y': 2})


def test_label_based_size_equivalent_to_positional_size_given_bin_edge_coord(
    simple_filter_func,
):
    da = make_histogram2d()
    result = simple_filter_func(
        da, size={'x': sc.scalar(1.0, unit='mm'), 'y': sc.scalar(5.0, unit='mm')}
    )
    reference = simple_filter_func(da, size=10)
    assert sc.identical(result, reference)


def test_label_based_size_equivalent_to_positional_size(simple_filter_func) -> None:
    da = make_data2d()
    result = simple_filter_func(
        da, size={'x': sc.scalar(1.0, unit='mm'), 'y': sc.scalar(5.0, unit='mm')}
    )
    reference = simple_filter_func(da, size=10)
    assert sc.identical(result, reference)


@pytest.mark.parametrize('origin', [0, 1, -1, {'x': 0, 'y': 1}])
def test_origin_is_equivalent_to_scipy_origin(origin) -> None:
    da = make_histogram2d()
    result = median_filter(da, size=4, origin=origin)
    reference = da.copy()
    reference.values = scipy.ndimage.median_filter(
        reference.values,
        size=4,
        origin=origin if isinstance(origin, int) else origin.values(),
    )
    assert sc.identical(result, reference)


@pytest.mark.parametrize('size', [2, 3, {'x': 2, 'y': 1}])
def test_size_is_equivalent_to_scipy_size(size) -> None:
    da = make_histogram2d()
    result = median_filter(da, size=size)
    reference = da.copy()
    reference.values = scipy.ndimage.median_filter(
        reference.values, size=size if isinstance(size, int) else size.values()
    )
    assert sc.identical(result, reference)


def test_coordinates_are_propagated(simple_filter_func) -> None:
    da = make_histogram2d()
    result = simple_filter_func(da, size=2)
    assert set(result.coords) == {'x', 'y'}
    assert sc.identical(result.coords['x'], da.coords['x'])
    assert sc.identical(result.coords['y'], da.coords['y'])


def test_input_is_not_modified(simple_filter_func) -> None:
    original = make_histogram2d()
    da = original.copy()
    simple_filter_func(da, size=2)
    assert sc.identical(da, original)
