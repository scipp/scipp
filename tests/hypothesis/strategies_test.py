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
def test_dims_produces_strs(dim) -> None:
    assert isinstance(dim, str)


@settings(max_examples=N_EXAMPLES)
@given(scst.sizes_dicts())
def test_sizes_dicts_produces_small_enough_dicts(sizes) -> None:
    assert isinstance(sizes, dict)
    assert len(sizes) < 5


@settings(max_examples=N_EXAMPLES)
@given(scst.sizes_dicts(0))
def test_sizes_dicts_with_ndim0_is_empty(sizes) -> None:
    assert len(sizes) == 0


@settings(max_examples=N_EXAMPLES)
@given(scst.sizes_dicts(1))
def test_sizes_dicts_with_ndim1_has_len_1(sizes) -> None:
    assert len(sizes) == 1


@settings(max_examples=N_EXAMPLES)
@given(scst.sizes_dicts(3))
def test_sizes_dicts_with_ndim3_has_len_3(sizes) -> None:
    assert len(sizes) == 3


@settings(max_examples=N_EXAMPLES)
@given(scst.sizes_dicts(2))
def test_sizes_dicts_maps_str_to_int(sizes) -> None:
    assert isinstance(next(iter(sizes.keys())), str)
    assert isinstance(next(iter(sizes.values())), int)


@settings(max_examples=N_EXAMPLES)
@given(scst.units())
def test_units_produces_valid_unit_str(unit) -> None:
    assert isinstance(unit, str)
    _ = sc.Unit(unit)  # does not raise exception


@settings(max_examples=N_EXAMPLES)
@given(scst.variables(ndim=0))
def test_variables_can_set_ndim_to_0(var) -> None:
    assert var.ndim == 0


@settings(max_examples=N_EXAMPLES)
@given(scst.variables(ndim=2))
def test_variables_can_set_ndim_to_2(var) -> None:
    assert var.ndim == 2


@settings(max_examples=N_EXAMPLES)
@given(scst.variables(ndim=st.just(3)))
def test_variables_can_set_ndim_strategy(var) -> None:
    assert var.ndim == 3


@settings(max_examples=N_EXAMPLES)
@given(scst.variables(sizes={}))
def test_variables_can_set_sizes_empty(var) -> None:
    assert var.sizes == {}


@settings(max_examples=N_EXAMPLES)
@given(scst.variables(sizes={'abc': 2}))
def test_variables_can_set_sizes_1d(var) -> None:
    assert var.sizes == {'abc': 2}


@settings(max_examples=N_EXAMPLES)
@given(scst.variables(sizes=st.just({'def': 3})))
def test_variables_can_set_sizes_strategy(var) -> None:
    assert var.sizes == {'def': 3}


def test_variables_ndim_and_sizes_are_mutually_exclusive() -> None:
    with pytest.raises(InvalidArgument):

        @settings(max_examples=1, database=None)
        @given(scst.variables(ndim=1, sizes={'a': 2}))
        def draw_from_strategy(_):
            pass


@settings(max_examples=N_EXAMPLES)
@given(scst.variables(unit='one'))
def test_variables_can_set_unit_to_one(var) -> None:
    assert var.unit == 'one'


@settings(max_examples=N_EXAMPLES)
@given(scst.variables(unit='m'))
def test_variables_can_set_unit_to_m(var) -> None:
    assert var.unit == 'm'


@settings(max_examples=N_EXAMPLES)
@given(scst.variables(unit=st.just('s')))
def test_variables_can_set_unit_strategy(var) -> None:
    assert var.unit == 's'


@settings(max_examples=N_EXAMPLES)
@given(scst.variables(dtype='int64'))
def test_variables_can_set_dtype_to_int64(var) -> None:
    assert var.dtype == 'int64'


@settings(max_examples=N_EXAMPLES)
@given(scst.variables(dtype=float))
def test_variables_can_set_dtype_to_float(var) -> None:
    assert var.dtype == 'float64'


@settings(max_examples=N_EXAMPLES)
@given(scst.variables(dtype=st.just('int64')))
def test_variables_can_set_dtype_strategy(var) -> None:
    assert var.dtype == 'int64'


@settings(max_examples=N_EXAMPLES)
@given(scst.variables(with_variances=True, dtype=scst.floating_dtypes()))
def test_variables_can_toggle_variances_on(var) -> None:
    assert var.variances is not None


@settings(max_examples=N_EXAMPLES)
@given(scst.variables(with_variances=False, dtype=scst.floating_dtypes()))
def test_variables_can_toggle_variances_off(var) -> None:
    assert var.variances is None


@settings(max_examples=N_EXAMPLES)
@given(scst.variables(with_variances=True, dtype=scst.floating_dtypes()))
def test_variable_variances_are_non_negative(var) -> None:
    assert sc.all(sc.variances(var) >= 0.0 * var.unit**2)


@settings(max_examples=N_EXAMPLES)
@given(scst.variables(with_variances=st.just(True), dtype=scst.floating_dtypes()))
def test_variables_can_set_variances_strategy(var) -> None:
    assert var.variances is not None


@settings(max_examples=N_EXAMPLES)
@given(scst.n_variables(2))
def test_n_variables_all_have_same_parameters(variables) -> None:
    a, b = variables
    assert a.sizes == b.sizes
    assert a.unit == b.unit
    assert a.dtype == b.dtype
    assert (a.variances is None) == (b.variances is None)


@settings(max_examples=N_EXAMPLES)
@given(
    scst.dataarrays(
        data_args={'dtype': int, 'sizes': {'a': 2, 'b': 3}},
    )
)
def test_dataarrays(da) -> None:
    assert da.sizes == {'a': 2, 'b': 3}
    assert da.dtype == int


@settings(max_examples=N_EXAMPLES)
@given(
    scst.dataarrays(
        data_args={'ndim': 0},
    )
)
def test_dataarrays_scalar(da) -> None:
    assert da.sizes == {}


@settings(max_examples=N_EXAMPLES)
@given(scst.dataarrays(data_args={'ndim': st.integers(min_value=2, max_value=4)}))
def test_dataarrays_makes_1d_coords_and_masks(da) -> None:
    for name, coord in da.coords.items():
        assert coord.ndim == 1, f"coord {name = }"
    for name, mask in da.masks.items():
        assert mask.ndim == 1, f"mask {name = }"


@settings(max_examples=N_EXAMPLES)
@given(
    scst.dataarrays(
        data_args={'dtype': int, 'sizes': {'a': 2, 'b': 3}},
        coords=False,
    )
)
def test_dataarrays_without_coords(da) -> None:
    assert not da.coords


@settings(max_examples=N_EXAMPLES)
@given(
    scst.dataarrays(
        data_args={'dtype': int, 'sizes': {'a': 2, 'b': 3}},
        masks=False,
    )
)
def test_dataarrays_without_masks(da) -> None:
    assert not da.masks


@settings(max_examples=N_EXAMPLES)
@given(
    scst.dataarrays(
        data_args={'dtype': int, 'sizes': {'a': 2, 'b': 3}},
        bin_edges=False,
    )
)
def test_dataarrays_no_bin_edges(da) -> None:
    for name in da.coords:
        assert not da.coords.is_edges(name)
    for name in da.masks:
        assert not da.masks.is_edges(name)
