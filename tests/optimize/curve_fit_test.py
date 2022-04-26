# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
import numpy as np
import scipp as sc
from scipp.optimize import curve_fit

import pytest
from functools import partial


def func(x, *, a, b):
    return a * sc.exp(-(b / x.unit) * x)


def array1d(*, a=1.2, b=1.3, noise_scale=0.1, size=50):
    x = sc.linspace(dim='xx', start=-0.1, stop=4.0, num=size, unit='m')
    y = func(x, a=a, b=b)
    rng = np.random.default_rng()
    # Noise is random but avoiding unbounded values to avoid flaky tests
    y.values += noise_scale * np.clip(rng.normal(size=size), -2.0, 2.0)
    return sc.DataArray(y, coords={'xx': x})


# This function can handle a and b specified as dimensionful Variables.
def func_with_vars(x, *, a, b):
    return a * sc.sqrt(b / x)


def array1d_from_vars(*, a, b, noise_scale=0.1, size=50):
    x = sc.linspace(dim='xx', start=0.1, stop=4.0, num=size, unit='m')
    y = func_with_vars(x, a=a, b=b)
    rng = np.random.default_rng()
    # Noise is random but avoiding unbounded values to avoid flaky tests
    y.values += noise_scale * np.clip(rng.normal(size=size), -2.0, 2.0)
    return sc.DataArray(y, coords={'xx': x})


def test_should_not_raise_given_function_with_dimensionless_params_and_1d_array():
    curve_fit(func, array1d())


def test_should_not_raise_given_function_with_dimensionful_params_and_1d_array():
    data = array1d_from_vars(a=sc.scalar(1.2, unit='s'), b=sc.scalar(100.0, unit='m'))
    curve_fit(func_with_vars,
              data,
              p0={
                  'a': sc.scalar(1.0, unit='s'),
                  'b': sc.scalar(1.0, unit='m')
              })


def test_should_raise_TypeError_when_xdata_given_as_param():
    with pytest.raises(TypeError):
        curve_fit(func, array1d(), xdata=np.arange(4))


def test_should_raise_TypeError_when_ydata_given_as_param():
    with pytest.raises(TypeError):
        curve_fit(func, array1d(), ydata=np.arange(4))


def test_should_raise_TypeError_when_sigma_given_as_param():
    with pytest.raises(TypeError):
        curve_fit(func, array1d(), sigma=np.arange(4))


def test_should_raise_KeyError_when_data_array_has_no_coord():
    da = array1d()
    del da.coords[da.dim]
    with pytest.raises(KeyError):
        curve_fit(func, da)


def test_should_raise_BinEdgeError_when_data_array_is_histogram():
    da = array1d()
    hist = da[1:].copy()
    hist.coords[hist.dim] = da.coords[hist.dim]
    with pytest.raises(sc.BinEdgeError):
        curve_fit(func, hist)


def test_should_raise_DimensionError_when_data_array_is_not_1d():
    da = sc.concat([array1d(), array1d()], 'extra_dim')
    with pytest.raises(sc.DimensionError):
        curve_fit(func, da)


def test_masks_are_not_ignored():
    da = array1d(size=20)
    unmasked, _ = curve_fit(func, da)
    da.masks['mask'] = sc.zeros(sizes=da.sizes, dtype=bool)
    da.masks['mask'][-5:] = sc.scalar(True)
    masked, _ = curve_fit(func, da)
    assert not sc.identical(masked['a'], unmasked['a'])
    assert not sc.identical(masked['b'], unmasked['b'])


@pytest.mark.parametrize("noise_scale", [1e-1, 1e-2, 1e-3, 1e-6, 1e-9])
def test_optimized_params_approach_real_params_as_data_noise_decreases(noise_scale):
    popt, _ = curve_fit(func, array1d(a=1.7, b=1.5, noise_scale=noise_scale))
    assert sc.allclose(popt['a'], sc.scalar(1.7), rtol=sc.scalar(2.0 * noise_scale))
    assert sc.allclose(popt['b'], sc.scalar(1.5), rtol=sc.scalar(2.0 * noise_scale))


def test_optimized_params_variances_are_diag_of_covariance_matrix():
    popt, pcov = curve_fit(func, array1d(a=1.7, b=1.5))
    assert popt['a'].variance == pcov['a']['a']
    assert popt['b'].variance == pcov['b']['b']


@pytest.mark.parametrize("mask_pos", [0, 1, -3])
@pytest.mark.parametrize("mask_size", [1, 2])
def test_masked_points_are_treated_as_if_they_were_removed(mask_pos, mask_size):
    da = array1d(size=10)
    da.masks['mask'] = sc.zeros(sizes=da.sizes, dtype=bool)
    da.masks['mask'][mask_pos:mask_pos + mask_size] = sc.scalar(True)
    masked, _ = curve_fit(func, da)
    removed, _ = curve_fit(
        func, sc.concat([da[:mask_pos], da[mask_pos + mask_size:]], da.dim))
    assert sc.identical(masked['a'], removed['a'])
    assert sc.identical(masked['b'], removed['b'])


@pytest.mark.parametrize("variance,expected", [(1e9, 1.0), (1, 2.0), (1 / 3, 3.0),
                                               (1e-9, 5.0)],
                         ids=['disabled', 'equal', 'high', 'dominant'])
def test_variances_determine_weights(variance, expected):
    x = sc.array(dims=['x'], values=[1, 2, 3, 4])
    y = sc.array(dims=['x'], values=[1., 5., 1., 1.], variances=[1., 1., 1., 1.])
    da = sc.DataArray(data=y, coords={'x': x})
    da.variances[1] = variance
    # Fit a constant to highlight influence of weights
    popt, _ = curve_fit(lambda x, *, a: sc.scalar(a), da)
    assert popt['a'].value == pytest.approx(expected)


def test_fit_function_with_dimensionful_params_raises_UnitError_when_no_p0_given():
    def f(x, *, a, b):
        return a * sc.exp(-b * x)

    with pytest.raises(sc.UnitError):
        curve_fit(f, array1d())


def test_fit_function_with_dimensionful_params_yields_outputs_with_units():
    def f(x, *, a, b):
        return a * sc.exp(-b * x)

    x = sc.linspace(dim='x', start=0.5, stop=2.0, num=10, unit='m')
    da = sc.DataArray(f(x, a=1.2, b=1.3 / sc.Unit('m')), coords={'x': x})
    popt, pcov = curve_fit(f, da, p0={'a': 1.1, 'b': 1.2 / sc.Unit('m')})
    assert popt['a'].unit == sc.units.one
    assert popt['b'].unit == sc.Unit('1/m')
    assert not isinstance(pcov['a']['a'], sc.Variable)
    assert pcov['a']['b'].unit == sc.Unit('1/m')
    assert pcov['b']['a'].unit == sc.Unit('1/m')
    assert pcov['b']['b'].unit == sc.Unit('1/m**2')


def test_default_params_are_not_used_for_fit():
    noise_scale = 1e-3
    popt, _ = curve_fit(partial(func, b=1.5),
                        array1d(a=1.7, b=1.5, noise_scale=noise_scale))
    assert 'b' not in popt
    assert sc.allclose(popt['a'], sc.scalar(1.7), rtol=sc.scalar(2.0 * noise_scale))


def test_default_params_with_initial_guess_are_used_for_fit():
    noise_scale = 1e-3
    popt, _ = curve_fit(partial(func, b=1.5),
                        array1d(a=1.7, b=1.5, noise_scale=noise_scale),
                        p0={'b': 1.1})
    assert sc.allclose(popt['a'], sc.scalar(1.7), rtol=sc.scalar(2.0 * noise_scale))
    assert sc.allclose(popt['b'], sc.scalar(1.5), rtol=sc.scalar(2.0 * noise_scale))


def test_bounds_limit_param_range_without_units():
    data = array1d(a=40.0, b=30.0)
    unconstrained, _ = curve_fit(func, data, p0={'a': 1.0, 'b': 1.0})
    # Fit approaches correct value more closely than with the bound below.
    assert sc.abs(unconstrained['a']).value > 3.0
    assert sc.abs(unconstrained['b']).value > 2.0

    constrained, _ = curve_fit(func,
                               data,
                               p0={
                                   'a': 1.0,
                                   'b': 1.0
                               },
                               bounds={
                                   'a': (-3, 3),
                                   'b': (sc.scalar(-2), sc.scalar(2))
                               })
    assert sc.abs(constrained['a']).value < 3.0
    assert sc.abs(constrained['b']).value < 2.0


def test_bounds_limit_param_range_with_units():
    data = array1d_from_vars(a=sc.scalar(20.0, unit='s'), b=sc.scalar(10.0, unit='m'))
    unconstrained, _ = curve_fit(func_with_vars,
                                 data,
                                 p0={
                                     'a': sc.scalar(1.0, unit='s'),
                                     'b': sc.scalar(1.0, unit='m')
                                 })
    # Fit approaches correct value more closely than with the bound below.
    assert (abs(unconstrained['a']) > sc.scalar(3.0, unit='s')).value
    assert (abs(unconstrained['b']) > sc.scalar(2.0, unit='m')).value

    constrained, _ = curve_fit(func_with_vars,
                               data,
                               p0={
                                   'a': sc.scalar(1.0, unit='s'),
                                   'b': sc.scalar(1.0, unit='m')
                               },
                               bounds={
                                   'a': (sc.scalar(-3.0,
                                                   unit='s'), sc.scalar(3.0, unit='s')),
                                   'b': (sc.scalar(-2, unit='m'), sc.scalar(2,
                                                                            unit='m')),
                               })

    assert (abs(constrained['a']) < sc.scalar(3.0, unit='s')).value
    assert (abs(constrained['b']) < sc.scalar(2.0, unit='m')).value


def test_bounds_limit_only_given_parameters_param_range():
    data = array1d_from_vars(a=sc.scalar(20.0, unit='s'), b=sc.scalar(10.0, unit='m'))
    unconstrained, _ = curve_fit(func_with_vars,
                                 data,
                                 p0={
                                     'a': sc.scalar(1.0, unit='s'),
                                     'b': sc.scalar(1.0, unit='m')
                                 })
    # Fit approaches correct value more closely than with the bound below.
    assert (abs(unconstrained['a']) > sc.scalar(5.0, unit='s')).value
    assert (abs(unconstrained['b']) > sc.scalar(2.0, unit='m')).value

    constrained, _ = curve_fit(
        func_with_vars,
        data,
        p0={
            'a': sc.scalar(1.0, unit='s'),
            'b': sc.scalar(1.0, unit='m')
        },
        bounds={'b': (sc.scalar(-2, unit='m'), sc.scalar(2, unit='m'))})

    assert (abs(constrained['a']) > sc.scalar(5.0, unit='s')).value
    assert (abs(constrained['b']) < sc.scalar(2.0, unit='m')).value


def test_bounds_must_have_unit_convertable_to_param_unit():
    data = array1d_from_vars(a=sc.scalar(1.2, unit='s'), b=sc.scalar(10.0, unit='m'))
    with pytest.raises(sc.UnitError):
        curve_fit(
            func_with_vars,
            data,
            p0={
                'a': sc.scalar(1.0, unit='s'),
                'b': sc.scalar(1.0, unit='m')
            },
            bounds={'a': (sc.scalar(-10.0, unit='s'), sc.scalar(10.0, unit='kg'))})


def test_jac_is_not_implemented():
    # replace this with an actual test once jac is implemented
    with pytest.raises(NotImplementedError):
        curve_fit(func, array1d(), jac=np.array([[1, 2], [3, 4]]))


def test_can_pass_extra_kwargs():
    data = array1d()
    data[2] = np.inf

    # does not raise
    curve_fit(func, data, check_finite=False)

    with pytest.raises(ValueError):
        curve_fit(func, data, check_finite=True)
