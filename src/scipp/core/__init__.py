# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock

# flake8: noqa
from .._scipp import _debug_
if _debug_:
    import warnings

    def custom_formatwarning(msg, *args, **kwargs):
        return str(msg) + '\n'

    warnings.formatwarning = custom_formatwarning
    warnings.warn(
        'You are running a "Debug" build of scipp. For optimal performance use a "Release" build.'
    )

from .._scipp import __version__
from .._scipp.core import Variable, DataArray, Dataset, GroupByDataArray, \
                         GroupByDataset, Unit
# Import errors
from .._scipp.core import BinEdgeError, BinnedDataError, CoordError, \
                         DataArrayError, DatasetError, DimensionError, \
                         DTypeError, NotFoundError, SizeError, SliceError, \
                         UnitError, VariableError, VariancesError
# Import submodules
from .._scipp.core import units, dtype

from .._scipp.core import get_slice_params

from .sizes import _make_sizes

setattr(Variable, 'sizes', property(_make_sizes))
setattr(DataArray, 'sizes', property(_make_sizes))
setattr(Dataset, 'sizes', property(_make_sizes))

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

from .arithmetic import add, divide, floor_divide, mod, multiply, subtract
from .bins import lookup, histogram, bin, bins, bins_like
from .comparison import less, greater, less_equal, greater_equal, equal, not_equal, identical, isclose, allclose
from .counts import counts_to_density, density_to_counts
from .cumulative import cumsum
from .dataset import combine_masks, merge
from .groupby import groupby
from .math import abs, nan_to_num, norm, reciprocal, pow, sqrt, exp, log, log10, round, floor, ceil, erf, erfc
from .operations import dot, islinspace, issorted, allsorted, cross, sort, values, variances, stddevs, rebin, where
from .reduction import mean, nanmean, sum, nansum, min, max, nanmin, nanmax, all, any
from .shape import broadcast, concat, concatenate, fold, flatten, transpose
from .trigonometry import sin, cos, tan, asin, acos, atan, atan2
from .unary import isnan, isinf, isfinite, isposinf, isneginf, to_unit
from .variable import scalar, zeros, zeros_like, ones, ones_like, empty, empty_like, full, full_like, matrix, matrices, vector, vectors, array, linspace, geomspace, logspace, arange
