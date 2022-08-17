# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock

# flake8: noqa
import os

if os.name == "nt" and "CONDA_PREFIX" in os.environ:
    # Due to https://github.com/conda-forge/python-feedstock/issues/444 , combinations of Python3, Anaconda and Windows
    # don't respect os.add_dll_path(...), which is python's mechanism for setting DLL search directories. Instead we
    # need to explicitly add it to the PATH environment variable. For non-conda versions of python we want to keep using
    # the usual python mechanism.
    #
    # This is probably due to the following patch in conda-built versions of python:
    # https://github.com/conda-forge/python-feedstock/blob/289b2a8017ddd000896e525f18867f4caacec6f2/recipe/patches/0020-Add-CondaEcosystemModifyDllSearchPath.patch
    #
    import importlib.resources
    with importlib.resources.path("scipp", "__init__.py") as path:
        dll_directory = (path.parent.parent / "bin").resolve()
        os.environ["PATH"] += os.pathsep + str(dll_directory)

from .configuration import config

from .core import __version__
# Import classes
from .core import Variable, DataArray, Dataset, DType, Unit
# Import errors
from .core import BinEdgeError, BinnedDataError, CoordError, \
                         DataArrayError, DatasetError, DimensionError, \
                         DTypeError, UnitError, VariableError, VariancesError
# Import submodules
from . import units
from . import geometry
# Import functions
from ._scipp.core import as_const, choose
# Import python functions
from .show import show, make_svg

from .html import to_html, make_html, table

setattr(Variable, '_repr_html_', make_html)
setattr(DataArray, '_repr_html_', make_html)
setattr(Dataset, '_repr_html_', make_html)

from .io.hdf5 import to_hdf5 as _to_hdf5

setattr(Variable, 'to_hdf5', _to_hdf5)
setattr(DataArray, 'to_hdf5', _to_hdf5)
setattr(Dataset, 'to_hdf5', _to_hdf5)
del _to_hdf5

from .format import format_variable as _format_variable

setattr(Variable, '__format__', _format_variable)
del _format_variable

from .extend_units import *
from .object_list import _repr_html_
from .utils import collapse, slices
from .compat.dict import to_dict, from_dict

from .coords import transform_coords, show_graph

from .core import add, divide, floor_divide, mod, multiply, negative, subtract
from .core import bin, group, hist, nanhist, rebin
from .core import lookup, bins, bins_like
from .core import less, greater, less_equal, greater_equal, equal, not_equal, identical, isclose, allclose
from .core import counts_to_density, density_to_counts
from .core import cumsum
from .core import merge
from .core import groupby
from .core import logical_not, logical_and, logical_or, logical_xor
from .core import abs, nan_to_num, norm, reciprocal, pow, sqrt, exp, log, log10, round, floor, ceil, erf, erfc, midpoints
from .core import dot, islinspace, issorted, allsorted, cross, sort, values, variances, stddevs, where
from .core import mean, nanmean, sum, nansum, min, max, nanmin, nanmax, all, any
from .core import broadcast, concat, fold, flatten, squeeze, transpose
from .core import sin, cos, tan, asin, acos, atan, atan2
from .core import isnan, isinf, isfinite, isposinf, isneginf, to_unit
from .core import scalar, index, zeros, zeros_like, ones, ones_like, empty, empty_like, full, full_like, matrix, matrices, vector, vectors, array, linspace, geomspace, logspace, arange, datetime, datetimes, epoch
from .core import to

from .logging import display_logs, get_logger

from .reduction import reduce

# Mainly imported for docs
from .core import Bins, Coords, GroupByDataset, GroupByDataArray, Masks

from . import _binding

_binding.bind_get()
_binding.bind_pop()
_binding.bind_conversion_to_builtin(Variable)
# Assign method binding for all containers
for _cls in (Variable, DataArray, Dataset):
    _binding.bind_functions_as_methods(_cls, globals(),
                                       ('sum', 'nansum', 'mean', 'nanmean', 'max',
                                        'min', 'nanmax', 'nanmin', 'all', 'any'))
del _cls
# Assign method binding for both Variable and DataArray
for _cls in (Variable, DataArray):
    _binding.bind_functions_as_methods(_cls, globals(),
                                       ('broadcast', 'flatten', 'fold', 'squeeze',
                                        'transpose', 'floor', 'ceil', 'round'))
    _binding.bind_function_as_method(cls=_cls, name='to', func=to, abbreviate_doc=False)
del _cls
del to
# Assign method binding for JUST Variable
_binding.bind_functions_as_methods(Variable, globals(), ('cumsum', ))
# Assign method binding for JUST Dataset
_binding.bind_functions_as_methods(Dataset, globals(), ('squeeze', ))
for _cls in (DataArray, Dataset):
    _binding.bind_functions_as_methods(_cls, globals(), ('groupby', 'transform_coords'))
del _cls
_binding.bind_functions_as_methods(Variable, globals(), ('bin', 'hist', 'nanhist'))
_binding.bind_functions_as_methods(DataArray, globals(),
                                   ('bin', 'group', 'hist', 'nanhist', 'rebin'))
_binding.bind_functions_as_methods(Dataset, globals(), ('hist', 'rebin'))
del _binding

from . import data
from . import spatial

from .plotting import plot

setattr(Variable, 'plot', plot)
setattr(DataArray, 'plot', plot)
setattr(Dataset, 'plot', plot)

from .core.binning import histogram
