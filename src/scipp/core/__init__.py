# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock

# ruff: noqa: E402, F401, F403, F821
from .._scipp import _debug_  # type: ignore[attr-defined]

if _debug_:
    import warnings

    warnings.warn(
        'You are running a "Debug" build of Scipp. '
        'For optimal performance use a "Release" build.',
        stacklevel=1,
    )
    del warnings
del _debug_

from .._scipp import __version__ as _core_version  # type: ignore[attr-defined]

__version__: str = _core_version
del _core_version

from .cpp_classes import (
    Coords,
    DataArray,
    Dataset,
    DefaultUnit,
    DType,
    GroupByDataArray,
    GroupByDataset,
    Masks,
    Unit,
    Variable,
)

# Import errors
from .cpp_classes import (
    BinEdgeError,
    BinnedDataError,
    CoordError,
    DataArrayError,
    DatasetError,
    DimensionError,
    DTypeError,
    UnitError,
    VariableError,
    VariancesError,
)

from .operations import as_const

from .data_group import DataGroup
from .dimensions import (
    _rename_dims,
    _rename_variable,
    _rename_data_array,
    _rename_dataset,
)

for cls in (Variable, DataArray, Dataset):
    cls.rename_dims = _rename_dims  # type: ignore[method-assign]
Variable.rename = _rename_variable  # type: ignore[method-assign]
DataArray.rename = _rename_data_array  # type: ignore[method-assign]
Dataset.rename = _rename_dataset  # type: ignore[method-assign]
del _rename_dims, _rename_variable, _rename_data_array, _rename_dataset, cls

from .bins import _bins, _set_bins, _is_binned

Variable.is_binned = property(_is_binned)  # type: ignore[assignment, method-assign]
DataArray.is_binned = property(_is_binned)  # type: ignore[assignment, method-assign]
Dataset.is_binned = property(_is_binned)  # type: ignore[assignment, method-assign]

Variable.bins = property(_bins, _set_bins)  # type: ignore[assignment, method-assign]
DataArray.bins = property(_bins, _set_bins)  # type: ignore[assignment, method-assign]
Dataset.bins = property(_bins, _set_bins)  # type: ignore[assignment, method-assign]

del _bins, _is_binned, _set_bins

from .structured import _fields

Variable.fields = property(  # type: ignore[assignment, method-assign]
    _fields,
    doc='Provides access to fields of structured types such as vectors or matrices.',
)
del _fields

from .bins import Bins

# Prevent unwanted conversion to numpy arrays by operations. Properly defining
# __array_ufunc__ should be possible by converting non-scipp arguments to
# variables. The most difficult part is probably mapping the ufunc to scipp
# functions.
for _cls in (Variable, DataArray, Dataset):
    _cls.__array_ufunc__ = None  # type: ignore[union-attr]
del _cls


from .arithmetic import add, divide, floor_divide, mod, multiply, negative, subtract
from .binning import bin, group, hist, nanhist, rebin
from .bins import Lookup, lookup, bins, bins_like
from .comparison import (
    less,
    greater,
    less_equal,
    greater_equal,
    equal,
    not_equal,
    identical,
    isclose,
    allclose,
)
from .counts import counts_to_density, density_to_counts
from .cumulative import cumsum
from .dataset import irreducible_mask
from .groupby import groupby
from .hyperbolic import sinh, cosh, tanh, asinh, acosh, atanh
from .logical import logical_not, logical_and, logical_or, logical_xor
from .math import (
    abs,
    cross,
    dot,
    nan_to_num,
    norm,
    reciprocal,
    pow,
    sqrt,
    exp,
    log,
    log10,
    round,
    floor,
    ceil,
    erf,
    erfc,
    midpoints,
)
from .operations import (
    islinspace,
    issorted,
    allsorted,
    sort,
    values,
    variances,
    stddevs,
    where,
    to,
    merge,
    label_based_index_to_positional_index,
)
from .reduction import (
    mean,
    nanmean,
    median,
    nanmedian,
    std,
    nanstd,
    var,
    nanvar,
    sum,
    nansum,
    min,
    max,
    nanmin,
    nanmax,
    all,
    any,
)
from .shape import broadcast, concat, fold, flatten, squeeze, transpose
from .trigonometry import sin, cos, tan, asin, acos, atan, atan2
from .unary import isnan, isinf, isfinite, isposinf, isneginf, to_unit
from .variable import (
    scalar,
    index,
    zeros,
    ones,
    empty,
    full,
    vector,
    vectors,
    array,
    linspace,
    geomspace,
    logspace,
    arange,
    datetime,
    datetimes,
    epoch,
)
from .like import zeros_like, ones_like, empty_like, full_like

from .assignments import assign_coords, assign_data, assign_masks

Dataset.assign_coords = assign_coords  # type: ignore[method-assign]
DataArray.assign = assign_data  # type: ignore[method-assign]
DataArray.assign_coords = assign_coords  # type: ignore[method-assign]
DataArray.assign_masks = assign_masks  # type: ignore[method-assign]
del assign_coords, assign_data, assign_masks

# Remove submodules to reduce clutter
del (
    arithmetic,  # type: ignore[name-defined]
    assignments,  # type: ignore[name-defined]
    binning,  # type: ignore[name-defined]
    comparison,  # type: ignore[name-defined]
    counts,  # type: ignore[name-defined]
    cpp_classes,  # type: ignore[name-defined]
    cumulative,  # type: ignore[name-defined]
    dataset,  # type: ignore[name-defined]
    data_group,  # type: ignore[name-defined]
    dimensions,  # type: ignore[name-defined]
    like,  # type: ignore[name-defined]
    logical,  # type: ignore[name-defined]
    math,  # type: ignore[name-defined]
    operations,  # type: ignore[name-defined]
    reduction,  # type: ignore[name-defined]
    shape,  # type: ignore[name-defined]
    structured,  # type: ignore[name-defined]
    trigonometry,  # type: ignore[name-defined]
    unary,  # type: ignore[name-defined]
    variable,  # type: ignore[name-defined]
)

__all__ = [
    'BinEdgeError',
    'BinnedDataError',
    'Bins',
    'CoordError',
    'Coords',
    'DType',
    'DTypeError',
    'DataArray',
    'DataArrayError',
    'DataGroup',
    'Dataset',
    'DatasetError',
    'DefaultUnit',
    'DimensionError',
    'GroupByDataArray',
    'GroupByDataset',
    'Lookup',
    'Masks',
    'Unit',
    'UnitError',
    'Variable',
    'VariableError',
    'VariancesError',
    '__version__',
    'abs',
    'acos',
    'acosh',
    'add',
    'all',
    'allclose',
    'allsorted',
    'any',
    'arange',
    'array',
    'as_const',
    'asin',
    'asinh',
    'atan',
    'atan2',
    'atanh',
    'bin',
    'bins',
    'bins_like',
    'broadcast',
    'ceil',
    'concat',
    'cos',
    'cosh',
    'counts_to_density',
    'cross',
    'cumsum',
    'datetime',
    'datetimes',
    'density_to_counts',
    'divide',
    'dot',
    'empty',
    'empty_like',
    'epoch',
    'equal',
    'erf',
    'erfc',
    'exp',
    'flatten',
    'floor',
    'floor_divide',
    'fold',
    'full',
    'full_like',
    'geomspace',
    'greater',
    'greater_equal',
    'group',
    'groupby',
    'hist',
    'identical',
    'index',
    'irreducible_mask',
    'isclose',
    'isfinite',
    'isinf',
    'islinspace',
    'isnan',
    'isneginf',
    'isposinf',
    'issorted',
    'label_based_index_to_positional_index',
    'less',
    'less_equal',
    'linspace',
    'log',
    'log10',
    'logical_and',
    'logical_not',
    'logical_or',
    'logical_xor',
    'logspace',
    'lookup',
    'max',
    'mean',
    'median',
    'merge',
    'midpoints',
    'min',
    'mod',
    'multiply',
    'nan_to_num',
    'nanhist',
    'nanmax',
    'nanmean',
    'nanmedian',
    'nanmin',
    'nanstd',
    'nansum',
    'nanvar',
    'negative',
    'norm',
    'not_equal',
    'ones',
    'ones_like',
    'pow',
    'rebin',
    'reciprocal',
    'round',
    'scalar',
    'sin',
    'sinh',
    'sort',
    'sqrt',
    'squeeze',
    'std',
    'stddevs',
    'subtract',
    'sum',
    'tan',
    'tanh',
    'to',
    'to_unit',
    'transpose',
    'values',
    'var',
    'variances',
    'vector',
    'vectors',
    'where',
    'zeros',
    'zeros_like',
]
