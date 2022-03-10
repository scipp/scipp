# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen
from functools import partial
from typing import Optional, Sequence, Union

from hypothesis.errors import InvalidArgument
from hypothesis import strategies as st
from hypothesis.extra import numpy as npst

from ..core import variable as creation
from ..core import DataArray, DType


# TODO sometimes checks fail with (seen in test_astype_int_to_float)
#  hypothesis.errors.FailedHealthCheck: Examples routinely exceeded the max allowable size. (20 examples overran while generating 9 valid ones). Generating examples this large will usually lead to bad results. You could try setting max_size parameters on your collections and turning max_leaves down on recursive() calls.
#  See https://hypothesis.readthedocs.io/en/latest/healthchecks.html for more information about this. If you want to disable just this health check, add HealthCheck.data_too_large to the suppress_health_check settings for this test.
#
#  Maybe the variable strat has too many parameters or too many nested strats?


def dims() -> st.SearchStrategy:
    # Allowing all graphic utf-8 characters and control characters
    # except NULL, which causes problems in C and C++ code (e.g. HDF5).
    return st.text(st.characters(
        whitelist_categories=['L', 'M', 'N', 'P', 'S', 'Zs', 'Cc'],
        blacklist_characters='\0'),
                   min_size=0,
                   max_size=10)


def sizes_dicts(
        ndim: Optional[Union[int, st.SearchStrategy]] = None) -> st.SearchStrategy:
    if isinstance(ndim, st.SearchStrategy):
        return ndim.flatmap(lambda n: sizes_dicts(ndim=n))
    keys = dims()
    values = st.integers(min_value=1, max_value=10)
    if ndim is None:
        # The constructor of sc.Variable in Python only supports
        # arrays with <= 4 dimensions.
        return st.dictionaries(keys=keys, values=values, min_size=0, max_size=4)
    return st.dictionaries(keys=keys, values=values, min_size=ndim, max_size=ndim)


def units() -> st.SearchStrategy:
    return st.sampled_from(('one', 'm', 'kg', 's', 'A', 'K', 'count'))


def integer_dtypes(sizes: Sequence[int] = (32, 64)) -> st.SearchStrategy:
    return st.sampled_from([f'int{size}' for size in sizes])


def floating_dtypes(sizes: Sequence[int] = (32, 64)) -> st.SearchStrategy:
    return st.sampled_from([f'float{size}' for size in sizes])


def scalar_numeric_dtypes() -> st.SearchStrategy:
    return st.sampled_from((integer_dtypes, floating_dtypes)).flatmap(lambda f: f())


def use_variances(dtype) -> st.SearchStrategy:
    if dtype in (DType.float32, DType.float64):
        return st.booleans()
    return st.just(False)


def _variables_from_fixed_args(args) -> st.SearchStrategy:

    def make_array():
        return npst.arrays(args['dtype'],
                           tuple(args['sizes'].values()),
                           elements=args['elements'],
                           fill=args['fill'],
                           unique=args['unique'])

    return st.builds(partial(creation.array,
                             dims=list(args['sizes'].keys()),
                             unit=args['unit']),
                     values=make_array(),
                     variances=make_array() if args['with_variances'] else st.none())


@st.composite
def variable_args(draw,
                  *,
                  ndim=None,
                  sizes=None,
                  unit=None,
                  dtype=None,
                  with_variances=None,
                  elements=None,
                  fill=None,
                  unique=None) -> dict:
    if ndim is not None:
        if sizes is not None:
            raise InvalidArgument('Arguments `ndim` and `sizes` cannot both be used. '
                                  f'Got ndim={ndim}, sizes={sizes}.')
    if sizes is None:
        sizes = sizes_dicts(ndim)
    if isinstance(sizes, st.SearchStrategy):
        sizes = draw(sizes)

    if unit is None:
        unit = units()
    if isinstance(unit, st.SearchStrategy):
        unit = draw(unit)

    if dtype is None:
        # TODO other dtypes?
        dtype = scalar_numeric_dtypes()
    if isinstance(dtype, st.SearchStrategy):
        dtype = draw(dtype)

    if with_variances is None:
        with_variances = use_variances(dtype)
    if isinstance(with_variances, st.SearchStrategy):
        with_variances = draw(with_variances)

    return dict(sizes=sizes,
                unit=unit,
                dtype=dtype,
                with_variances=with_variances,
                elements=elements,
                fill=fill,
                unique=unique)


def variables(*,
              ndim=None,
              sizes=None,
              unit=None,
              dtype=None,
              with_variances=None,
              elements=None,
              fill=None,
              unique=None) -> st.SearchStrategy:
    args = variable_args(ndim=ndim,
                         sizes=sizes,
                         unit=unit,
                         dtype=dtype,
                         with_variances=with_variances,
                         elements=elements,
                         fill=fill,
                         unique=unique)
    return args.flatmap(lambda a: _variables_from_fixed_args(a))


def n_variables(n: int,
                *,
                ndim=None,
                sizes=None,
                unit=None,
                dtype=None,
                with_variances=None,
                elements=None,
                fill=None,
                unique=None) -> st.SearchStrategy:
    args = variable_args(ndim=ndim,
                         sizes=sizes,
                         unit=unit,
                         dtype=dtype,
                         with_variances=with_variances,
                         elements=elements,
                         fill=fill,
                         unique=unique)
    return args.flatmap(
        lambda a: st.tuples(*(_variables_from_fixed_args(a) for _ in range(n))))


@st.composite
def _make_vectors(draw, sizes):
    values = draw(npst.arrays(float, (*sizes.values(), 3)))
    return creation.vectors(dims=tuple(sizes), values=values, unit=draw(units()))


@st.composite
def vectors(draw, ndim=None) -> st.SearchStrategy:
    if ndim is None:
        ndim = draw(st.integers(0, 3))
    return draw(sizes_dicts(ndim).flatmap(lambda s: _make_vectors(s)))


@st.composite
def coord_dicts_1d(draw, *, coords, sizes, args=None) -> dict:
    args = (args or {})
    args['sizes'] = sizes
    try:
        del args['ndim']
    except KeyError:
        pass
    return {dim: draw(variables(**args)) for dim in coords}


class _NotSetType:

    def __repr__(self):
        return 'Default'

    def __bool__(self):
        return False


_NotSet = _NotSetType()


@st.composite
def dataarrays(draw,
               data_args=None,
               coords=_NotSet,
               coord_args=None) -> st.SearchStrategy:
    data = draw(variables(**(data_args or {})))
    if coords is _NotSet:
        coords = data.dims
    if coords is not None:
        coord_dict = draw(
            coord_dicts_1d(coords=coords, sizes=data.sizes, args=coord_args))
    else:
        coord_dict = {}
    return DataArray(data, coords=coord_dict)
