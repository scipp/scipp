# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen
import pytest
from hypothesis import given, settings
from hypothesis import strategies as st
from hypothesis.errors import InvalidArgument

import scipp as sc
from scipp.testing import strategies as scst

N_EXAMPLES = 10


@settings(max_examples=N_EXAMPLES)
@given(scst.dims())
def test_dims_produces_strs(dim):
    assert isinstance(dim, str)


@settings(max_examples=N_EXAMPLES)
@given(scst.sizes_dicts())
def test_sizes_dicts_produces_small_enough_dicts(sizes):
    assert isinstance(sizes, dict)
    assert len(sizes) < 5


@settings(max_examples=N_EXAMPLES)
@given(scst.sizes_dicts(0))
def test_sizes_dicts_with_ndim0_is_empty(sizes):
    assert len(sizes) == 0


@settings(max_examples=N_EXAMPLES)
@given(scst.sizes_dicts(1))
def test_sizes_dicts_with_ndim1_has_len_1(sizes):
    assert len(sizes) == 1


@settings(max_examples=N_EXAMPLES)
@given(scst.sizes_dicts(3))
def test_sizes_dicts_with_ndim3_has_len_3(sizes):
    assert len(sizes) == 3


@settings(max_examples=N_EXAMPLES)
@given(scst.sizes_dicts(2))
def test_sizes_dicts_maps_str_to_int(sizes):
    assert isinstance(next(iter(sizes.keys())), str)
    assert isinstance(next(iter(sizes.values())), int)


@settings(max_examples=N_EXAMPLES)
@given(scst.units())
def test_units_produces_valid_unit_str(unit):
    assert isinstance(unit, str)
    _ = sc.Unit(unit)  # does not raise exception


@settings(max_examples=N_EXAMPLES)
@given(scst.variables(ndim=0))
def test_variables_can_set_ndim_to_0(var):
    assert var.ndim == 0


@settings(max_examples=N_EXAMPLES)
@given(scst.variables(ndim=2))
def test_variables_can_set_ndim_to_2(var):
    assert var.ndim == 2


@settings(max_examples=N_EXAMPLES)
@given(scst.variables(ndim=st.just(3)))
def test_variables_can_set_ndim_strategy(var):
    assert var.ndim == 3


@settings(max_examples=N_EXAMPLES)
@given(scst.variables(sizes={}))
def test_variables_can_set_sizes_empty(var):
    assert var.sizes == {}


@settings(max_examples=N_EXAMPLES)
@given(scst.variables(sizes={'abc': 2}))
def test_variables_can_set_sizes_1d(var):
    assert var.sizes == {'abc': 2}


@settings(max_examples=N_EXAMPLES)
@given(scst.variables(sizes=st.just({'def': 3})))
def test_variables_can_set_sizes_strategy(var):
    assert var.sizes == {'def': 3}


def test_variables_ndim_and_sizes_are_mutually_exclusive():
    with pytest.raises(InvalidArgument):

        @settings(max_examples=1, database=None)
        @given(scst.variables(ndim=1, sizes={'a': 2}))
        def draw_from_strategy(_):
            pass


@settings(max_examples=N_EXAMPLES)
@given(scst.variables(unit='one'))
def test_variables_can_set_unit_to_one(var):
    assert var.unit == 'one'


@settings(max_examples=N_EXAMPLES)
@given(scst.variables(unit='m'))
def test_variables_can_set_unit_to_m(var):
    assert var.unit == 'm'


@settings(max_examples=N_EXAMPLES)
@given(scst.variables(unit=st.just('s')))
def test_variables_can_set_unit_strategy(var):
    assert var.unit == 's'


@settings(max_examples=N_EXAMPLES)
@given(scst.variables(dtype='int64'))
def test_variables_can_set_dtype_to_int64(var):
    assert var.dtype == 'int64'


@settings(max_examples=N_EXAMPLES)
@given(scst.variables(dtype=float))
def test_variables_can_set_dtype_to_float(var):
    assert var.dtype == 'float64'


@settings(max_examples=N_EXAMPLES)
@given(scst.variables(dtype=st.just('int64')))
def test_variables_can_set_dtype_strategy(var):
    assert var.dtype == 'int64'


@settings(max_examples=N_EXAMPLES)
@given(scst.variables(with_variances=True, dtype=scst.floating_dtypes()))
def test_variables_can_toggle_variances_on(var):
    assert var.variances is not None


@settings(max_examples=N_EXAMPLES)
@given(scst.variables(with_variances=False, dtype=scst.floating_dtypes()))
def test_variables_can_toggle_variances_off(var):
    assert var.variances is None


@settings(max_examples=N_EXAMPLES)
@given(scst.variables(with_variances=st.just(True), dtype=scst.floating_dtypes()))
def test_variables_can_set_variances_strategy(var):
    assert var.variances is not None


@settings(max_examples=N_EXAMPLES)
@given(scst.n_variables(2))
def test_n_variables_all_have_same_parameters(variables):
    a, b = variables
    assert a.sizes == b.sizes
    assert a.unit == b.unit
    assert a.dtype == b.dtype
    assert (a.variances is None) == (b.variances is None)


@settings(max_examples=N_EXAMPLES)
@given(
    scst.dataarrays(data_args={'dtype': int, 'sizes': {'a': 2, 'b': 3}},
                    coords=['b'],
                    coord_args=None))
def test_dataarrays(da):
    assert da.sizes == {'a': 2, 'b': 3}
    assert da.dtype == int
    assert da.coords['b'].sizes == {'a': 2, 'b': 3}
