# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen
"""
Search strategies for hypothesis to generate inputs for tests.
"""

from collections.abc import Callable, Sequence
from functools import partial
from typing import Any

import numpy as np
from hypothesis import strategies as st
from hypothesis.core import Ex  # type: ignore[attr-defined]
from hypothesis.errors import InvalidArgument
from hypothesis.extra import numpy as npst

from ..core import DataArray, DType, Unit, Variable
from ..core import variable as creation


def dims() -> st.SearchStrategy:
    # Allowing all graphic utf-8 characters and control characters
    # except NULL, which causes problems in C and C++ code (e.g. HDF5).
    return st.text(
        st.characters(
            whitelist_categories=['L', 'M', 'N', 'P', 'S', 'Zs', 'Cc'],
            blacklist_characters='\0',
        ),
        min_size=0,
        max_size=50,
    )


def sizes_dicts(
    ndim: int | st.SearchStrategy | None = None,
) -> st.SearchStrategy:
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


def _variables_from_fixed_args(args: dict[str, Any]) -> st.SearchStrategy:
    def make_array(variances: bool) -> st.SearchStrategy:
        elements = args['elements']
        if elements is None and variances:
            # Make sure that variances are non-negative and
            # let the user decide otherwise.
            elements = st.floats(
                min_value=0.0, width=32 if np.dtype(args['dtype']) == np.float32 else 64
            )

        return npst.arrays(
            args['dtype'],
            tuple(args['sizes'].values()),
            elements=elements,
            fill=args['fill'],
            unique=args['unique'],
        )

    return st.builds(
        partial(creation.array, dims=list(args['sizes'].keys()), unit=args['unit']),
        values=make_array(False),
        variances=make_array(True) if args['with_variances'] else st.none(),
    )


class _ConditionallyWithVariances:
    def __init__(self) -> None:
        self._strategy = st.booleans()

    def __call__(self, draw: st.DrawFn, dtype: DType) -> bool:
        if dtype in (DType.float32, DType.float64):
            return draw(self._strategy)
        return False


@st.composite
def _concrete_args(
    draw: st.DrawFn, args: dict[str, st.SearchStrategy | Any]
) -> dict[str, Any]:
    def _draw(x: st.SearchStrategy[Ex] | Ex) -> Ex:
        return draw(x) if isinstance(x, st.SearchStrategy) else x

    concrete = {key: _draw(val) for key, val in args.items()}
    if isinstance(concrete['with_variances'], _ConditionallyWithVariances):
        concrete['with_variances'] = concrete['with_variances'](draw, concrete['dtype'])
    return concrete


def _variable_arg_strategies(
    *,
    ndim: int | st.SearchStrategy | None = None,
    sizes: dict[str, int] | st.SearchStrategy | None = None,
    unit: str | Unit | st.SearchStrategy | None = None,
    dtype: str | DType | type | st.SearchStrategy | None = None,
    with_variances: bool
    | st.SearchStrategy
    | _ConditionallyWithVariances
    | None = None,
    elements: float | st.SearchStrategy | None = None,
    fill: float | st.SearchStrategy | None = None,
    unique: bool | st.SearchStrategy | None = None,
) -> dict[str, st.SearchStrategy | Any]:
    if ndim is not None:
        if sizes is not None:
            raise InvalidArgument(
                'Arguments `ndim` and `sizes` cannot both be used. '
                f'Got {ndim=}, {sizes=}.'
            )
    if sizes is None:
        sizes = sizes_dicts(ndim)
    if unit is None:
        unit = units()
    if dtype is None:
        # TODO other dtypes?
        dtype = scalar_numeric_dtypes()
    if with_variances is None:
        with_variances = _ConditionallyWithVariances()
    return {
        'sizes': sizes,
        'unit': unit,
        'dtype': dtype,
        'with_variances': with_variances,
        'elements': elements,
        'fill': fill,
        'unique': unique,
    }


# This implementation is designed such that the individual strategies
# for default arguments are constructed only once, namely when
# `variables` is called. Sampling via `_concrete_args` then reuses
# those strategies.
# A previous implementation constructed those component strategies inside
# an `st.composite` function for every example drawn. This led to high
# memory consumption by hypothesis and failed
# `hypothesis.HealthCheck.data_too_large`.
def variables(
    *,
    ndim: int | st.SearchStrategy | None = None,
    sizes: dict[str, int] | st.SearchStrategy | None = None,
    unit: str | Unit | st.SearchStrategy | None = None,
    dtype: str | DType | type | st.SearchStrategy | None = None,
    with_variances: bool | st.SearchStrategy | None = None,
    elements: float | st.SearchStrategy | None = None,
    fill: float | st.SearchStrategy | None = None,
    unique: bool | st.SearchStrategy | None = None,
) -> st.SearchStrategy[Variable]:
    args = _variable_arg_strategies(
        ndim=ndim,
        sizes=sizes,
        unit=unit,
        dtype=dtype,
        with_variances=with_variances,
        elements=elements,
        fill=fill,
        unique=unique,
    )
    return _concrete_args(args).flatmap(_variables_from_fixed_args)


def n_variables(
    n: int,
    *,
    ndim: int | st.SearchStrategy | None = None,
    sizes: dict[str, int] | st.SearchStrategy | None = None,
    unit: str | Unit | st.SearchStrategy | None = None,
    dtype: str | DType | type | st.SearchStrategy | None = None,
    with_variances: bool | st.SearchStrategy | None = None,
    elements: float | st.SearchStrategy | None = None,
    fill: float | st.SearchStrategy | None = None,
    unique: bool | st.SearchStrategy | None = None,
) -> st.SearchStrategy[tuple[Variable]]:
    args = _variable_arg_strategies(
        ndim=ndim,
        sizes=sizes,
        unit=unit,
        dtype=dtype,
        with_variances=with_variances,
        elements=elements,
        fill=fill,
        unique=unique,
    )
    return _concrete_args(args).flatmap(
        lambda a: st.tuples(*(_variables_from_fixed_args(a) for _ in range(n)))
    )


@st.composite
def coord_dicts(
    draw: Callable[[st.SearchStrategy[Ex]], Ex],
    *,
    sizes: dict[str, int],
    args: dict[str, Any] | None = None,
    bin_edges: bool = True,
) -> dict[str, Variable]:
    args = args or {}
    args['sizes'] = sizes
    try:
        del args['ndim']
    except KeyError:
        pass

    if bin_edges:

        def size_increment() -> int:
            return draw(st.integers(min_value=0, max_value=1))  # type:ignore[arg-type, return-value]

    else:

        def size_increment() -> int:
            return 0

    if not sizes:
        return {}

    names_and_sizes: list[tuple[Any, tuple[str, int]]] = draw(  # type: ignore[assignment]
        st.lists(  # type: ignore[arg-type]
            st.sampled_from(list(sizes))
            .map(lambda dim: (dim, sizes[dim] + size_increment()))
            .flatmap(
                lambda item: (st.just(item[0]) | dims()).map(lambda name: (name, item))
            ),
            min_size=0,
            max_size=6,
        )
    )
    return {
        name: draw(variables(**{**args, 'sizes': {dim: size}}))  # type: ignore[arg-type, misc]
        for name, (dim, size) in names_and_sizes
    }


@st.composite
def dataarrays(
    draw: Callable[[st.SearchStrategy[Ex]], Ex],
    *,
    data_args: dict[str, Any] | None = None,
    coords: bool = True,
    coord_args: dict[str, Any] | None = None,
    masks: bool = True,
    mask_args: dict[str, Any] | None = None,
    bin_edges: bool = True,
) -> DataArray:
    """Generate data arrays with coords and masks.

    The data variable can be any variable supported by
    ``scipp.testing.strategies.variables``.
    The coordinates and masks are constrained to be one-dimensional where the
    dimension is one of the dims of the data.
    The name of a coordinate or mask may be,
    but is not required to be, a dimension name.

    Parameters
    ----------
    draw:
        Provided by Hypothesis.
    data_args:
        Arguments for creating the data variable.
    coords:
        Selects whether coords are generated.
    coord_args:
        Arguments for creating the coordinate variable.
    masks:
        Selects whether masks are generated.
    mask_args:
        Arguments for creating the mask variable.
    bin_edges:
        If ``True``, coords may be bin edges.

    See Also
    --------
    scipp.testing.strategies.variables:
        For allowed items in ``*_args`` dicts.
    """
    data: Variable = draw(variables(**(data_args or {})))  # type: ignore[arg-type, assignment]

    coords_dict: dict[str, Variable] = (
        draw(  # type: ignore[assignment]
            coord_dicts(sizes=data.sizes, args=coord_args, bin_edges=bin_edges)  # type: ignore[arg-type]
        )
        if coords
        else {}
    )

    if masks:
        mask_args = mask_args or {}
        mask_args['dtype'] = bool
        masks_dict: dict[str, Variable] = draw(  # type: ignore[assignment]
            coord_dicts(sizes=data.sizes, args=mask_args, bin_edges=False)  # type: ignore[arg-type]
        )
    else:
        masks_dict = {}

    return DataArray(
        data,
        coords=coords_dict,
        masks=masks_dict,
    )


__all__ = [
    'coord_dicts',
    'dataarrays',
    'dims',
    'floating_dtypes',
    'integer_dtypes',
    'n_variables',
    'scalar_numeric_dtypes',
    'sizes_dicts',
    'units',
    'variables',
]
