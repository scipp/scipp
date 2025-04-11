# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
from functools import partial

import numpy as np
import pytest

import scipp as sc
from scipp import curve_fit
from scipp.compat.xarray_compat import from_xarray, to_xarray
from scipp.testing import assert_allclose, assert_identical


@pytest.fixture
def rng():
    return np.random.default_rng(seed=1234)


def func(x, a, b):
    return a * sc.exp(-b * x)


def func2d(x, t, a, b):
    return a * sc.exp(-b * t / (1 + x))


def func3d(x, t, y, a, b):
    return a * y * sc.exp(-b * t / (1 + x))


def func_np(x, a, b):
    return a * np.exp(-b * x)


def func2d_np(x, t, a, b):
    return a * np.exp(-b * t / (1 + x))


def func3d_np(x, t, y, a, b):
    return a * y * np.exp(-b * t / (1 + x))


def array(coords, f, noise_scale, seed=1234):
    rng = np.random.default_rng(seed)
    da_coords = {
        c: sc.linspace(
            dim=kw.pop('dim', c),
            **kw,
        )
        for c, kw in coords.items()
    }
    data = f(**da_coords)
    # Noise is random but avoiding unbounded values to avoid flaky tests
    data.values += noise_scale * np.clip(rng.normal(size=data.values.shape), -1.5, 1.5)
    return sc.DataArray(data, coords=da_coords)


def array1d(*, a=1.2, b=1.3, noise_scale=0.1, size=50):
    return array(
        # Note, dim name different from coord name to make sure
        # the code doesn't depend on them being the same.
        coords={'x': {'dim': 'xx', 'start': -0.1, 'stop': 4.0, 'num': size}},
        f=partial(func, a=a, b=b),
        noise_scale=noise_scale,
    )


def array2d(*, a=1.2, b=1.3, noise_scale=0.1, size=20):
    return array(
        coords={
            'x': {'dim': 'xx', 'start': -0.1, 'stop': 4.0, 'num': size},
            't': {'dim': 'tt', 'start': 0.0, 'stop': 1.0, 'num': size // 2},
        },
        f=partial(func2d, a=a, b=b),
        noise_scale=noise_scale,
    )


def array3d(*, a=1.2, b=1.3, noise_scale=0.1, size=10):
    return array(
        coords={
            'x': {'dim': 'xx', 'start': -0.1, 'stop': 4.0, 'num': size},
            't': {'dim': 'tt', 'start': 0.0, 'stop': 1.0, 'num': size // 2},
            'y': {'dim': 'yy', 'start': 2.0, 'stop': 4.0, 'num': size},
        },
        f=partial(func3d, a=a, b=b),
        noise_scale=noise_scale,
    )


def array1d_from_vars(*, a, b, noise_scale=0.1, size=50):
    return array(
        coords={
            'x': {'dim': 'xx', 'start': 0.1, 'stop': 4.0, 'num': size, 'unit': 'm'}
        },
        f=partial(func, a=a, b=b),
        noise_scale=noise_scale,
    )


def test_should_not_raise_given_function_with_dimensionless_params_and_1d_array() -> (
    None
):
    curve_fit(['x'], func, array1d())


@pytest.mark.parametrize(
    ('p0', 'params', 'bounds'),
    [
        (None, {'a': 1.2, 'b': 1.3}, None),
        ({'a': 3, 'b': 2}, {'a': 1.2, 'b': 1.3}, None),
        ({'a': 0.2, 'b': -1}, {'a': 1.2, 'b': 1.3}, {'a': (-3, 3), 'b': (-2, 2)}),
        ({'a': 0.2, 'b': -1}, {'a': 1.2, 'b': 1.3}, {'a': (-3, 1.1), 'b': (-2, 1.1)}),
    ],
)
@pytest.mark.parametrize(
    "dims",
    [
        {'x': 10, 't': 10, 'y': 10},
        {'x': 5, 't': 8, 'y': 7},
    ],
)
@pytest.mark.parametrize(
    ('coords', 'reduce_dims'),
    [
        (['x'], []),
        (['x'], ['y']),
        (['x'], ['t', 'y']),
        (['x', 't'], []),
        (['x', 't'], ['y']),
        (['x', 't', 'y'], []),
    ],
)
def test_compare_to_curve_fit_xarray(
    dims, coords, reduce_dims, p0, params, bounds
) -> None:
    _ = pytest.importorskip('xarray')
    f, fxarray = {
        1: (func, func_np),
        2: (func2d, lambda x, a, b: func2d_np(*x, a, b)),
        3: (func3d, lambda x, a, b: func3d_np(*x, a, b)),
    }[len(coords)]
    da = array(
        {c: {'start': 1, 'stop': 3, 'num': n} for c, n in dims.items()},
        partial(func3d, **params),
        noise_scale=0.0,
    )

    result, _ = curve_fit(
        coords,
        f,
        da,
        p0=p0,
        bounds=bounds,
        reduce_dims=reduce_dims,
    )
    xresult = from_xarray(
        to_xarray(da).curvefit(
            coords,
            fxarray,
            p0=p0,
            bounds=bounds,
            reduce_dims=reduce_dims,
        )['curvefit_coefficients']
    )
    for param_name in result:
        assert sc.allclose(
            result[param_name].data,
            xresult['param', sc.scalar(param_name)].data,
        )


@pytest.mark.parametrize(
    ('p0', 'params', 'bounds'),
    [
        (None, {'a': 1.2, 'b': 1.3}, None),
        ({'a': 3, 'b': 2}, {'a': 1.2, 'b': 1.3}, None),
        ({'a': 0.2, 'b': -1}, {'a': 1.2, 'b': 1.3}, {'a': (-3, 3), 'b': (-2, 2)}),
        ({'a': 0.2, 'b': -1}, {'a': 1.2, 'b': 1.3}, {'a': (-3, 1.1), 'b': (-2, 1.1)}),
    ],
)
@pytest.mark.parametrize(
    "dims",
    [
        {'x': 10, 't': 6, 'y': 3},
        {'t': 5, 'x': 8, 'y': 7},
    ],
)
@pytest.mark.parametrize('workers', [1, 2])
def test_compare_to_curve_fit_xarray_multiple_curves(
    dims, p0, params, bounds, workers
) -> None:
    _ = pytest.importorskip('xarray')
    da = array(
        {c: {'start': 1, 'stop': 3, 'num': n} for c, n in dims.items()},
        partial(func3d, **params),
        noise_scale=0.0,
    )
    result, _ = curve_fit(
        ['x'],
        func,
        da,
        p0=p0,
        bounds=bounds,
        workers=workers,
    )
    xresult = from_xarray(
        to_xarray(da).curvefit(
            ['x'],
            func_np,
            p0=p0,
            bounds=bounds,
        )['curvefit_coefficients']
    )
    for param_name in result:
        assert sc.allclose(
            result[param_name].data,
            xresult['param', sc.scalar(param_name)].data,
        )


def test_dimensions_present_in_reduce_dims_argument_are_not_present_in_output() -> None:
    popt, _ = curve_fit(['x'], func3d, array3d())
    assert 'tt' in popt['a'].dims
    assert 'yy' in popt['a'].dims

    popt, _ = curve_fit(['x'], func3d, array3d(), reduce_dims=['tt'])
    assert 'tt' not in popt['a'].dims
    assert 'yy' in popt['a'].dims

    popt, _ = curve_fit(['x'], func3d, array3d(), reduce_dims=['tt', 'yy'])
    assert 'tt' not in popt['a'].dims
    assert 'yy' not in popt['a'].dims


def test_should_not_raise_given_function_with_dimensionful_params_and_1d_array() -> (
    None
):
    curve_fit(
        ['x'],
        func,
        array1d_from_vars(a=sc.scalar(1.2, unit='s'), b=sc.scalar(100.0, unit='1/m')),
        p0={'a': sc.scalar(1.0, unit='s'), 'b': sc.scalar(1.0, unit='1/m')},
    )


def test_should_raise_TypeError_when_xdata_given_as_param() -> None:
    with pytest.raises(TypeError):
        curve_fit(['x'], func, array1d(), xdata=np.arange(4))


def test_should_raise_TypeError_when_ydata_given_as_param() -> None:
    with pytest.raises(TypeError):
        curve_fit(['x'], func, array1d(), ydata=np.arange(4))


def test_should_raise_TypeError_when_sigma_given_as_param() -> None:
    with pytest.raises(TypeError):
        curve_fit(['x'], func, array1d(), sigma=np.arange(4))


def test_should_raise_ValueError_when_sigma_contains_zeros(rng) -> None:
    da = array1d(size=50)
    da.variances = rng.normal(0.0, 0.1, size=50) ** 2
    da['xx', 21].variance = 0.0
    with pytest.raises(ValueError, match='0 in the input variances'):
        curve_fit(['x'], func, da)


def test_does_not_raise_when_sigma_contains_zeros_that_is_masked(rng) -> None:
    da = array1d(size=50)
    da.variances = rng.normal(0.0, 0.1, size=50) ** 2
    da.masks['m'] = sc.full(value=False, sizes=da.sizes)
    da['xx', 21].variance = 0.0
    da.masks['m']['xx', 21] = True
    curve_fit(['x'], func, da)


def test_should_raise_KeyError_when_data_array_has_no_coord() -> None:
    da = array1d()
    for c in tuple(da.coords):
        del da.coords[c]
    with pytest.raises(KeyError):
        curve_fit(['x'], func, da)


def test_should_raise_BinEdgeError_when_data_array_is_histogram() -> None:
    da = array1d()
    hist = da[1:].copy()
    hist.coords['x'] = da.coords['x']
    with pytest.raises(sc.BinEdgeError):
        curve_fit(['x'], func, hist)


def test_masks_are_not_ignored() -> None:
    da = array1d(size=20)
    unmasked, _ = curve_fit(['x'], func, da)
    da.masks['mask'] = sc.zeros(sizes=da.sizes, dtype=bool)
    da.masks['mask'][-5:] = sc.scalar(True)
    masked, _ = curve_fit(['x'], func, da)
    assert not sc.identical(masked['a'], unmasked['a'])
    assert not sc.identical(masked['b'], unmasked['b'])


@pytest.mark.parametrize(
    ('f', 'array', 'coords'),
    [
        (func, array1d, ['x']),
        (func2d, array2d, ['x', 't']),
        (func3d, array3d, ['x', 't', 'y']),
    ],
)
@pytest.mark.parametrize(
    "noise_scale",
    [1e-1, 1e-2, 1e-3, 1e-6, 1e-9],
)
def test_optimized_params_approach_real_params_as_data_noise_decreases(
    noise_scale, f, array, coords
):
    popt, _ = curve_fit(coords, f, array(a=1.7, b=1.5, noise_scale=noise_scale))
    assert sc.allclose(
        popt['a'].data, sc.scalar(1.7), rtol=sc.scalar(2.0 * noise_scale)
    )
    assert sc.allclose(
        popt['b'].data, sc.scalar(1.5), rtol=sc.scalar(2.0 * noise_scale)
    )


@pytest.mark.parametrize(
    ('f', 'array', 'coords'),
    [
        (func3d, array3d, ['x', 't', 'y']),
        (func3d, array3d, ['y', 'x', 't']),
        (func3d, array3d, ['t', 'y', 'x']),
    ],
)
@pytest.mark.parametrize(
    "noise_scale",
    [1e-1, 1e-2, 1e-3, 1e-6, 1e-9],
)
def test_order_of_coords_does_not_matter(noise_scale, f, array, coords) -> None:
    popt, _ = curve_fit(coords, f, array(a=1.7, b=1.5, noise_scale=noise_scale))
    assert sc.allclose(
        popt['a'].data, sc.scalar(1.7), rtol=sc.scalar(2.0 * noise_scale)
    )
    assert sc.allclose(
        popt['b'].data, sc.scalar(1.5), rtol=sc.scalar(2.0 * noise_scale)
    )


@pytest.mark.parametrize(
    ('f', 'fnp', 'array', 'coords'),
    [
        (func, func_np, array1d, ['x']),
        (func2d, func2d_np, array2d, ['x', 't']),
        (func3d, func3d_np, array3d, ['x', 't', 'y']),
    ],
)
def test_scipp_fun_and_numpy_fun_finds_same_optimized_params(
    f, fnp, array, coords
) -> None:
    data = array(a=1.7, b=1.5, noise_scale=1e-2)
    popt, _ = curve_fit(coords, f, data)
    popt_np, _ = curve_fit(coords, fnp, data, unsafe_numpy_f=True)

    for p, p_np in zip(popt.values(), popt_np.values(), strict=True):
        assert sc.allclose(p.data, p_np.data, rtol=sc.scalar(2.0 * 1e-2))


def test_optimized_params_variances_are_diag_of_covariance_matrix() -> None:
    popt, pcov = curve_fit(['x'], func, array1d(a=1.7, b=1.5))
    assert popt['a'].variances == pcov['a']['a'].data.values
    assert popt['b'].variances == pcov['b']['b'].data.values


@pytest.mark.parametrize("mask_pos", [0, 1, -3])
@pytest.mark.parametrize("mask_size", [1, 2])
def test_masked_points_are_treated_as_if_they_were_removed(mask_pos, mask_size) -> None:
    da = array1d(size=10)
    da.masks['mask'] = sc.zeros(sizes=da.sizes, dtype=bool)
    da.masks['mask'][mask_pos : mask_pos + mask_size] = sc.scalar(True)
    masked, _ = curve_fit(['x'], func, da)
    removed, _ = curve_fit(
        ['x'], func, sc.concat([da[:mask_pos], da[mask_pos + mask_size :]], da.dim)
    )
    assert sc.identical(masked['a'], removed['a'])
    assert sc.identical(masked['b'], removed['b'])


@pytest.mark.parametrize(
    ('variance', 'expected'),
    [(1e9, 1.0), (1, 2.0), (1 / 3, 3.0), (1e-9, 5.0)],
    ids=['disabled', 'equal', 'high', 'dominant'],
)
def test_variances_determine_weights(variance, expected) -> None:
    x = sc.array(dims=['x'], values=[1, 2, 3, 4])
    y = sc.array(
        dims=['x'], values=[1.0, 5.0, 1.0, 1.0], variances=[1.0, 1.0, 1.0, 1.0]
    )
    da = sc.DataArray(data=y, coords={'x': x})
    da.variances[1] = variance
    # Fit a constant to highlight influence of weights
    popt, _ = curve_fit(['x'], lambda x, *, a: a, da)
    assert popt['a'].value == pytest.approx(expected)


def test_fit_function_with_dimensionful_params_raises_UnitError_when_no_p0_given() -> (
    None
):
    def f(x, *, a, b):
        return a * sc.exp(-b * x)

    with pytest.raises(sc.UnitError):
        curve_fit(
            ['x'],
            f,
            array1d_from_vars(
                a=sc.scalar(1.2, unit='s'), b=sc.scalar(100.0, unit='1/m')
            ),
        )


def test_fit_function_with_dimensionful_params_yields_outputs_with_units() -> None:
    def f(x, *, a, b):
        return a * sc.exp(-b * x)

    x = sc.linspace(dim='x', start=0.5, stop=2.0, num=10, unit='m')
    da = sc.DataArray(f(x, a=1.2, b=1.3 / sc.Unit('m')), coords={'x': x})
    popt, pcov = curve_fit(['x'], f, da, p0={'a': 1.1, 'b': 1.2 / sc.Unit('m')})
    assert popt['a'].unit == sc.units.one
    assert popt['b'].unit == sc.Unit('1/m')
    assert not isinstance(pcov['a']['a'], sc.Variable)
    assert pcov['a']['b'].unit == sc.Unit('1/m')
    assert pcov['b']['a'].unit == sc.Unit('1/m')
    assert pcov['b']['b'].unit == sc.Unit('1/m**2')


def test_default_params_with_initial_guess_are_used_for_fit() -> None:
    noise_scale = 1e-3
    popt, _ = curve_fit(
        ['x'],
        partial(func, b=1.5),
        array1d(a=1.7, b=1.5, noise_scale=noise_scale),
        p0={'b': 1.1},
    )
    assert sc.allclose(
        popt['a'].data, sc.scalar(1.7), rtol=sc.scalar(2.0 * noise_scale)
    )
    assert sc.allclose(
        popt['b'].data, sc.scalar(1.5), rtol=sc.scalar(2.0 * noise_scale)
    )


def test_bounds_limit_param_range_without_units() -> None:
    data = array1d(a=40.0, b=30.0)
    unconstrained, _ = curve_fit(['x'], func, data, p0={'a': 1.0, 'b': 1.0})
    # Fit approaches correct value more closely than with the bound below.
    assert sc.abs(unconstrained['a']).value > 3.0
    assert sc.abs(unconstrained['b']).value > 2.0

    constrained, _ = curve_fit(
        ['x'],
        func,
        data,
        p0={'a': 1.0, 'b': 1.0},
        bounds={'a': (-3, 3), 'b': (sc.scalar(-2), sc.scalar(2))},
    )
    assert sc.abs(constrained['a']).value < 3.0
    assert sc.abs(constrained['b']).value < 2.0


def test_bounds_limit_param_range_with_units() -> None:
    data = array1d_from_vars(a=sc.scalar(20.0, unit='s'), b=sc.scalar(10.0, unit='1/m'))
    unconstrained, _ = curve_fit(
        ['x'],
        func,
        data,
        p0={'a': sc.scalar(1.0, unit='s'), 'b': sc.scalar(1.0, unit='1/m')},
    )
    # Fit approaches correct value more closely than with the bound below.
    assert (abs(unconstrained['a']) > sc.scalar(3.0, unit='s')).value
    assert (abs(unconstrained['b']) > sc.scalar(2.0, unit='1/m')).value

    constrained, _ = curve_fit(
        ['x'],
        func,
        data,
        p0={'a': sc.scalar(1.0, unit='s'), 'b': sc.scalar(1.0, unit='1/m')},
        bounds={
            'a': (sc.scalar(-3.0, unit='s'), sc.scalar(3.0, unit='s')),
            'b': (sc.scalar(-2, unit='1/m'), sc.scalar(2, unit='1/m')),
        },
    )

    assert (abs(constrained['a']) < sc.scalar(3.0, unit='s')).value
    assert (abs(constrained['b']) < sc.scalar(2.0, unit='1/m')).value


def test_bounds_limit_only_given_parameters_param_range() -> None:
    data = array1d_from_vars(a=sc.scalar(20.0, unit='s'), b=sc.scalar(10.0, unit='1/m'))
    unconstrained, _ = curve_fit(
        ['x'],
        func,
        data,
        p0={'a': sc.scalar(1.0, unit='s'), 'b': sc.scalar(1.0, unit='1/m')},
    )

    # Fit approaches correct value more closely than with the bound below.
    assert (abs(unconstrained['a']) > sc.scalar(5.0, unit='s')).value
    assert (abs(unconstrained['b']) > sc.scalar(2.0, unit='1/m')).value

    constrained, _ = curve_fit(
        ['x'],
        func,
        data,
        p0={'a': sc.scalar(1.0, unit='s'), 'b': sc.scalar(1.0, unit='1/m')},
        bounds={'b': (sc.scalar(-2, unit='1/m'), sc.scalar(2, unit='1/m'))},
    )

    # assert (abs(constrained['a']) > sc.scalar(5.0, unit='s')).value
    assert (abs(constrained['b']) <= sc.scalar(2.0, unit='1/m')).value


def test_bounds_must_have_unit_convertable_to_param_unit() -> None:
    data = array1d_from_vars(a=sc.scalar(1.2, unit='s'), b=sc.scalar(10.0, unit='1/m'))
    with pytest.raises(sc.UnitError):
        curve_fit(
            ['x'],
            func,
            data,
            p0={'a': sc.scalar(1.0, unit='s'), 'b': sc.scalar(1.0, unit='1/m')},
            bounds={'a': (sc.scalar(-10.0, unit='s'), sc.scalar(10.0, unit='kg'))},
        )


def test_jac_is_not_implemented() -> None:
    # replace this with an actual test once jac is implemented
    with pytest.raises(NotImplementedError):
        curve_fit(['x'], func, array1d(), jac=np.array([[1, 2], [3, 4]]))


def test_can_pass_extra_kwargs() -> None:
    data = array1d()

    # Does not raise
    curve_fit(['x'], func, data, method='lm')

    with pytest.raises(ValueError, match='method'):
        curve_fit(['x'], func, data, method='bad_method')


def test_can_rename_coords() -> None:
    def func(apple, *, a, b):
        return a * sc.exp(-b * apple)

    curve_fit({'apple': 'x'}, func, array1d())


def test_can_use_non_coord_in_fit() -> None:
    data = array1d()
    z = data.coords['x'].copy()
    curve_fit({'x': z}, func, data)


def test_1d_mask() -> None:
    noise_scale = 0.01
    da = array1d(a=1.2, b=1.3, noise_scale=noise_scale)
    mask = (da.coords['x'] > 1) & (da.coords['x'] < 2)
    da.data += sc.where(mask, sc.scalar(1000_000), sc.scalar(0))
    da.masks['high'] = mask
    res, _ = curve_fit(['x'], func, da)
    assert sc.allclose(
        res['a'].data, sc.scalar(1.2), atol=sc.scalar(noise_scale, unit='dimensionless')
    )
    assert sc.allclose(
        res['b'].data, sc.scalar(1.3), atol=sc.scalar(noise_scale, unit='dimensionless')
    )


def test_several_masks() -> None:
    noise_scale = 0.01
    da = array1d(a=1.2, b=1.3, noise_scale=noise_scale)
    mask1 = (da.coords['x'] > 1) & (da.coords['x'] <= 1.5)
    mask2 = (da.coords['x'] > 1.5) & (da.coords['x'] <= 2)
    da.data += sc.where(mask1 | mask2, sc.scalar(1000_000), sc.scalar(0))
    da.masks['high1'] = mask1
    da.masks['high2'] = mask2
    res, _ = curve_fit(['x'], func, da)
    assert sc.allclose(
        res['a'].data, sc.scalar(1.2), atol=sc.scalar(noise_scale, unit='dimensionless')
    )
    assert sc.allclose(
        res['b'].data, sc.scalar(1.3), atol=sc.scalar(noise_scale, unit='dimensionless')
    )


def test_2d_mask() -> None:
    noise_scale = 0.01
    da = array2d(a=1.2, b=1.3, noise_scale=noise_scale)
    mask = (
        (da.coords['x'] > 1)
        & (da.coords['x'] < 2)
        & (da.coords['t'] > 0.4)
        & (da.coords['t'] < 0.6)
    )
    da.data += sc.where(mask, sc.scalar(1000_000), sc.scalar(0))
    da.masks['high'] = mask
    res, _ = curve_fit(['x', 't'], func2d, da)
    assert sc.allclose(
        res['a'].data, sc.scalar(1.2), atol=sc.scalar(noise_scale, unit='dimensionless')
    )
    assert sc.allclose(
        res['b'].data, sc.scalar(1.3), atol=sc.scalar(noise_scale, unit='dimensionless')
    )


def test_mask_persists_only_if_fit_is_not_over_mask_dimension() -> None:
    noise_scale = 0.01
    da = array2d(a=1.2, b=1.3, noise_scale=noise_scale)
    mask = (da.coords['t'] > 0.4) & (da.coords['t'] < 0.6)
    da.masks['high'] = mask
    da.masks['mixed_dims'] = mask & (da.coords['x'] > 0.4)
    res, _ = curve_fit(['x'], func, da)

    assert 'high' in res['a'].masks
    assert_identical(res['a'].masks['high'], mask)

    assert 'mixed_dims' not in res['a'].masks


def test_param_values_set_to_nan_if_too_few_to_fit() -> None:
    da = array1d(a=1.2, b=1.3, noise_scale=0.01)

    # Mask everything
    da.masks['mask'] = da.coords['x'] > float('-inf')

    res, cov = curve_fit(['x'], func, da)
    assert sc.isnan(res['a'].data).all()
    assert sc.isnan(cov['a']['b'].data).all()

    # Unmask two entries
    da.masks['mask'][:2] = sc.scalar(False)
    res, cov = curve_fit(['x'], func, da)
    assert not sc.isnan(res['a'].data).any()
    assert not sc.isnan(cov['a']['b'].data).any()


def test_param_values_set_to_nan_masked() -> None:
    da = array2d(a=1.2, b=1.3, noise_scale=0.01)

    # Mask everything
    da.masks['mask'] = da.coords['t'] > float('-inf')

    res, cov = curve_fit(['x'], func, da)
    assert sc.isnan(res['a'].data).all()
    assert sc.isnan(cov['a']['b'].data).all()

    # Unmask two entries
    da.masks['mask'][:2] = sc.scalar(False)
    res, cov = curve_fit(['x'], func, da)
    assert sc.isnan(res['a'].data).any()
    assert not sc.isnan(res['a'].data).all()


def test_array_valued_initial_guess() -> None:
    expected = sc.linspace('xx', 1, 2, 20)
    da = array2d(a=expected, b=1.3, noise_scale=0.001)

    res, cov = curve_fit(['t'], func2d, da, p0={'a': sc.linspace('xx', 2, 3, 20)})
    res['a'].data.variances = None
    assert_allclose(res['a'].data, expected, rtol=sc.scalar(1e-2))

    with pytest.raises(sc.DimensionError):
        curve_fit(['t'], func2d, da, p0={'a': sc.linspace('yy', 2, 3, 20)})


def test_array_valued_bounds() -> None:
    expected = sc.linspace('xx', 1, 2, 20)
    da = array2d(a=expected, b=1.3, noise_scale=0.001)

    res, cov = curve_fit(
        ['t'],
        func2d,
        da,
        p0={'a': 3.1},
        bounds={'a': (sc.linspace('xx', 2.1, 3.1, 20), None)},
    )
    res['a'].data.variances = None
    assert_allclose(res['a'].data - sc.scalar(1.1), expected, rtol=sc.scalar(1e-2))

    with pytest.raises(sc.DimensionError):
        curve_fit(
            ['t'],
            func2d,
            da,
            p0={'a': 3},
            bounds={'a': (sc.linspace('yy', 2, 3, 20), None)},
        )


@pytest.mark.parametrize(
    ('bound', 'expected'),
    [
        ((None, 1), 1.0),
        ((None, sc.scalar(1)), 1.0),
        ((float('-inf'), 1), 1.0),
        ((1, None), 1.2),
        ((sc.scalar(1.0), None), 1.2),
        ((sc.scalar(1.0), sc.scalar(float('inf'))), 1.2),
    ],
)
def test_bound_one_sided_with_None(bound, expected) -> None:
    da = array1d(a=1.2, b=1.3, noise_scale=0.001)

    res, cov = curve_fit(
        ['x'],
        func,
        da,
        p0={'a': 1},
        bounds={'a': bound},
    )
    res['a'].data.variances = None
    assert_allclose(res['a'].data, sc.scalar(expected), rtol=sc.scalar(1e-2))


@pytest.mark.parametrize(
    'bound',
    [
        (-5, sc.scalar(1.0)),
        (sc.scalar(1.0), 5),
    ],
)
def test_bound_fails_with_mixed_types(bound) -> None:
    da = array1d(a=1.2, b=1.3, noise_scale=0.001)
    with pytest.raises(ValueError, match=r"Bounds cannot.*"):
        curve_fit(
            ['x'],
            func,
            da,
            bounds={'a': bound},
        )


def test_partial() -> None:
    da = array1d(a=1.2, b=1.3, noise_scale=0.01)
    res, cov = curve_fit(['x'], partial(func, b=1.3), da)
    assert 'a' in res
    assert 'b' not in res
    assert 'b' not in cov


def test_ignores_default_arguments() -> None:
    da = array1d(a=1.2, b=1.3, noise_scale=0.01)
    res, cov = curve_fit(['x'], lambda x, a, b=1.3: func(x, a, b=b), da)
    assert 'b' not in res
    res, cov = curve_fit(['x'], lambda x, a, *, b=1.3: func(x, a, b=b), da)
    assert 'b' not in res
    res, cov = curve_fit(
        ['x'], lambda x, a, *, b=1.3: func(x, a, b=b), da, p0={'b': sc.scalar(1.3)}
    )
    assert 'b' in res


def test_with_callable_class() -> None:
    da = array1d(a=1.2, b=1.3, noise_scale=0.01)

    class Model:
        def __call__(self, x, a, b):
            return func(x, a, b)

    popt, _ = curve_fit(['x'], Model(), da)
    assert sc.allclose(popt['a'].data, sc.scalar(1.2), rtol=sc.scalar(2.0 * 0.01))
    assert sc.allclose(popt['b'].data, sc.scalar(1.3), rtol=sc.scalar(2.0 * 0.01))


# TODO: https://github.com/pytest-dev/pytest/issues/10965
# Pool on Windows/macOS hangs on github action runners
# def test_with_nonpickle_function() -> None:
#     da = array2d(a=1.2, b=1.3, noise_scale=0.01)

#     def local_function(x, a, b):
#         return func(x, a, b)

#     # Running without multiprocessing lets you use a local function
#     curve_fit(['x'], local_function, da, workers=1)
#     with pytest.raises(ValueError, match='The provided fit function is not'):
#         # Running with multiprocessing fails if the function is not pickleable
#         curve_fit(['x'], local_function, da, workers=2)

#     # Function in __main__ scope works with multiprocessing
#     curve_fit(['x'], func, da, workers=2)
