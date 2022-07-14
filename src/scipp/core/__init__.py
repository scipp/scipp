# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock

# flake8: noqa
from .._scipp import _debug_
if _debug_:
    import warnings
    warnings.warn(
        'You are running a "Debug" build of scipp. For optimal performance use a "Release" build.'
    )

from .._scipp import __version__
from .cpp_classes import Coords, DataArray, Dataset, DType, GroupByDataArray,\
                         GroupByDataset, Masks, Unit, Variable
# Import errors
from .cpp_classes import BinEdgeError, BinnedDataError, CoordError, \
                         DataArrayError, DatasetError, DimensionError, \
                         DTypeError, UnitError, VariableError, VariancesError

from .._scipp.core import get_slice_params

from .dimensions import _rename_dims, _rename_variable, _rename_data_array, _rename_dataset

for cls in (Variable, DataArray, Dataset):
    setattr(cls, 'rename_dims', _rename_dims)
setattr(Variable, 'rename', _rename_variable)
setattr(DataArray, 'rename', _rename_data_array)
setattr(Dataset, 'rename', _rename_dataset)
del _rename_dims, _rename_variable, _rename_data_array, _rename_dataset, cls

from .bins import _bins, _set_bins

setattr(Variable, 'bins', property(_bins, _set_bins))
setattr(DataArray, 'bins', property(_bins, _set_bins))
setattr(Dataset, 'bins', property(_bins, _set_bins))

from .structured import _fields

setattr(
    Variable, 'fields',
    property(
        _fields,
        doc=
        """Provides access to fields of structured types such as vectors or matrices."""
    ))

from .bins import _groupby_bins, Bins

setattr(GroupByDataArray, 'bins', property(_groupby_bins))
setattr(GroupByDataset, 'bins', property(_groupby_bins))

# Prevent unwanted conversion to numpy arrays by operations. Properly defining
# __array_ufunc__ should be possible by converting non-scipp arguments to
# variables. The most difficult part is probably mapping the ufunc to scipp
# functions.
for _cls in (Variable, DataArray, Dataset):
    setattr(_cls, '__array_ufunc__', None)
del _cls

from .arithmetic import add, divide, floor_divide, mod, multiply, negative, subtract
from .binning import bin, group, hist, nanhist, rebin
from .bins import lookup, bins, bins_like
from .comparison import less, greater, less_equal, greater_equal, equal, not_equal, identical, isclose, allclose
from .counts import counts_to_density, density_to_counts
from .cumulative import cumsum
from .dataset import irreducible_mask, merge
from .groupby import groupby
from .logical import logical_not, logical_and, logical_or, logical_xor
from .math import abs, cross, dot, nan_to_num, norm, reciprocal, pow, sqrt, exp, log, log10, round, floor, ceil, erf, erfc, midpoints
from .operations import islinspace, issorted, allsorted, sort, values, variances, stddevs, where, to
from .reduction import mean, nanmean, sum, nansum, min, max, nanmin, nanmax, all, any
from .shape import broadcast, concat, fold, flatten, squeeze, transpose
from .trigonometry import sin, cos, tan, asin, acos, atan, atan2
from .unary import isnan, isinf, isfinite, isposinf, isneginf, to_unit
from .variable import scalar, index, zeros, ones, empty, full, matrix, matrices, vector, vectors, array, linspace, geomspace, logspace, arange, datetime, datetimes, epoch
from .like import zeros_like, ones_like, empty_like, full_like
