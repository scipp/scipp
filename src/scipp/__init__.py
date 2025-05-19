# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
# ruff: noqa: E402, F401, F821
# Suppress errors from assigning methods, MyPy gets the correct types from the stub.
# mypy: disable-error-code="assignment, method-assign"
"""Multi-dimensional data arrays with labeled dimensions.

Scipp provides

* Physical units are stored with each data or coord array and are handled in arithmetic operations.
* Histograms, i.e., bin-edge axes, which are by 1 longer than the data extent.
* Support for non-regular or scattered data and non-destructive binning.
* Support for masks stored with data.
* Propagation of uncertainties.

See the online documentation for user guides and an API reference: https://scipp.github.io/
"""  # noqa: E501

from .core import __version__

# Import classes
from .core import Variable, DataArray, DataGroup, Dataset, DType, Unit

# Import errors
from .core import (
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

# Import submodules
from . import units

# Import functions

# Import python functions
from .visualization import show, make_svg, to_html, make_html, table

Variable._repr_html_ = make_html
DataArray._repr_html_ = make_html
Dataset._repr_html_ = make_html
del visualization  # type: ignore[name-defined]

from .io.hdf5 import save_hdf5 as _save_hdf5

Variable.save_hdf5 = _save_hdf5
DataArray.save_hdf5 = _save_hdf5
Dataset.save_hdf5 = _save_hdf5
DataGroup.save_hdf5 = _save_hdf5
del _save_hdf5

from .format import format_variable as _format_variable

Variable.__format__ = _format_variable
del _format_variable

from ._extend_units import extend_units

extend_units()
del extend_units

from .compat.dict import to_dict, from_dict

from .object_list import _repr_html_
from .utils import collapse, slices

del object_list, utils  # type: ignore[name-defined]

from .coords import transform_coords, show_graph

from .core import add, divide, floor_divide, mod, multiply, negative, subtract
from .core import bin, group, hist, nanhist, rebin
from .core import Lookup, lookup, bins, bins_like
from .core import (
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
from .core import counts_to_density, density_to_counts
from .core import cumsum
from .core import merge
from .core import groupby
from .core import logical_not, logical_and, logical_or, logical_xor
from .core import (
    abs,
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
from .core import (
    dot,
    islinspace,
    issorted,
    allsorted,
    cross,
    sort,
    values,
    variances,
    stddevs,
    where,
)
from .core import (
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
from .core import broadcast, concat, fold, flatten, squeeze, transpose
from .core import sinh, cosh, tanh, asinh, acosh, atanh
from .core import sin, cos, tan, asin, acos, atan, atan2
from .core import isnan, isinf, isfinite, isposinf, isneginf, to_unit
from .core import (
    scalar,
    index,
    zeros,
    zeros_like,
    ones,
    ones_like,
    empty,
    empty_like,
    full,
    full_like,
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
from .core import as_const
from .core import to

from .logging import display_logs, get_logger

from .reduction import reduce

del reduction  # type: ignore[name-defined]

# Mainly imported for docs
from .core import Bins, Coords, GroupByDataset, GroupByDataArray, Masks

from . import _binding

_binding.bind_get()
_binding.bind_pop()
_binding.bind_conversion_to_builtin(Variable)
# Assign method binding for all containers
for _cls in (Variable, DataArray, Dataset):
    _binding.bind_functions_as_methods(
        _cls,
        globals(),
        (
            'sum',
            'nansum',
            'mean',
            'nanmean',
            'median',
            'nanmedian',
            'std',
            'nanstd',
            'var',
            'nanvar',
            'max',
            'min',
            'nanmax',
            'nanmin',
            'all',
            'any',
        ),
    )
del _cls
# Assign method binding for both Variable and DataArray
for _cls in (Variable, DataArray):
    _binding.bind_functions_as_methods(
        _cls,
        globals(),
        (
            'broadcast',
            'flatten',
            'fold',
            'squeeze',
            'transpose',
            'floor',
            'ceil',
            'round',
        ),
    )
    _binding.bind_function_as_method(cls=_cls, name='to', func=to, abbreviate_doc=False)
del _cls
del to
# Assign method binding for JUST Variable
_binding.bind_functions_as_methods(Variable, globals(), ('cumsum',))
# Assign method binding for JUST Dataset
_binding.bind_functions_as_methods(Dataset, globals(), ('squeeze',))
for _cls in (DataArray, Dataset):
    _binding.bind_functions_as_methods(_cls, globals(), ('groupby', 'transform_coords'))
del _cls
_binding.bind_functions_as_methods(Variable, globals(), ('bin', 'hist', 'nanhist'))
_binding.bind_functions_as_methods(
    DataArray, globals(), ('bin', 'group', 'hist', 'nanhist', 'rebin')
)
_binding.bind_functions_as_methods(Dataset, globals(), ('hist', 'rebin'))
del _binding

from . import data
from . import spatial
from .operations import elemwise_func

del operations  # type: ignore[name-defined]

from .plotting import plot

Variable.plot = plot
DataArray.plot = plot
Dataset.plot = plot
del plotting  # type: ignore[name-defined]

from .core.util import VisibleDeprecationWarning

del core  # type: ignore[name-defined]

from .curve_fit import curve_fit

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
    'VisibleDeprecationWarning',
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
    'collapse',
    'compat',
    'concat',
    'coords',
    'cos',
    'cosh',
    'counts_to_density',
    'cross',
    'cumsum',
    'curve_fit',
    'data',
    'datetime',
    'datetimes',
    'density_to_counts',
    'display_logs',
    'divide',
    'dot',
    'elemwise_func',
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
    'format',
    'from_dict',
    'full',
    'full_like',
    'geomspace',
    'get_logger',
    'greater',
    'greater_equal',
    'group',
    'groupby',
    'hist',
    'histogram',
    'identical',
    'index',
    'io',
    'isclose',
    'isfinite',
    'isinf',
    'islinspace',
    'isnan',
    'isneginf',
    'isposinf',
    'issorted',
    'less',
    'less_equal',
    'linspace',
    'log',
    'log10',
    'logging',
    'logical_and',
    'logical_not',
    'logical_or',
    'logical_xor',
    'logspace',
    'lookup',
    'make_html',
    'make_svg',
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
    'plot',
    'pow',
    'rebin',
    'reciprocal',
    'reduce',
    'reduction',
    'round',
    'scalar',
    'show',
    'show_graph',
    'sin',
    'sinh',
    'slices',
    'sort',
    'spatial',
    'sqrt',
    'squeeze',
    'std',
    'stddevs',
    'subtract',
    'sum',
    'table',
    'tan',
    'tanh',
    'to_dict',
    'to_html',
    'to_unit',
    'transform_coords',
    'transpose',
    'typing',
    'units',
    'utils',
    'values',
    'var',
    'variances',
    'vector',
    'vectors',
    'where',
    'zeros',
    'zeros_like',
]
