# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
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

from . import runtime_config

user_configuration_filename = runtime_config.config_filename
config = runtime_config.load()
del runtime_config

from .core import __version__
# Import classes
from .core import Variable, DataArray, Dataset, Unit
# Import errors
from .core import BinEdgeError, BinnedDataError, CoordError, \
                         DataArrayError, DatasetError, DimensionError, \
                         DTypeError, NotFoundError, SizeError, SliceError, \
                         UnitError, VariableError, VariancesError
# Import submodules
from .core import units, dtype
from . import geometry
# Import functions
from ._scipp.core import as_const, choose, logical_and, logical_or, logical_xor
# Import python functions
from .show import show, make_svg
from .table import table

from .plotting import plot

setattr(Variable, 'plot', plot)
setattr(DataArray, 'plot', plot)
setattr(Dataset, 'plot', plot)

from .html import to_html, make_html

setattr(Variable, '_repr_html_', make_html)
setattr(DataArray, '_repr_html_', make_html)
setattr(Dataset, '_repr_html_', make_html)

from .io.hdf5 import to_hdf5 as _to_hdf5

setattr(Variable, 'to_hdf5', _to_hdf5)
setattr(DataArray, 'to_hdf5', _to_hdf5)
setattr(Dataset, 'to_hdf5', _to_hdf5)
del _to_hdf5

from .extend_units import *
from .object_list import _repr_html_
from .utils import collapse, slices
from .compat.dict import to_dict, from_dict

from .coords import transform_coords, show_graph

from .core import add, divide, floor_divide, mod, multiply, subtract
from .core import lookup, histogram, bin, bins, bins_like
from .core import less, greater, less_equal, greater_equal, equal, not_equal, identical, isclose, allclose
from .core import counts_to_density, density_to_counts
from .core import cumsum
from .core import combine_masks, merge
from .core import groupby
from .core import abs, nan_to_num, norm, reciprocal, pow, sqrt, exp, log, log10, round, floor, ceil, erf, erfc
from .core import dot, islinspace, issorted, allsorted, cross, sort, values, variances, stddevs, rebin, where
from .core import mean, nanmean, sum, nansum, min, max, nanmin, nanmax, all, any
from .core import broadcast, concat, concatenate, fold, flatten, transpose
from .core import sin, cos, tan, asin, acos, atan, atan2
from .core import isnan, isinf, isfinite, isposinf, isneginf, to_unit
from .core import scalar, zeros, zeros_like, ones, ones_like, empty, empty_like, full, full_like, matrix, matrices, vector, vectors, array, linspace, geomspace, logspace, arange

from .logging import display_logs, get_logger

from .reduction import reduce

# Mainly imported for docs
from .core import Bins, GroupByDataset, GroupByDataArray

from . import _binding

_binding.bind_get()
_binding.bind_pop()
# Assign method binding for both Variable and DataArray
for _cls in (Variable, DataArray):
    _binding.bind_functions_as_methods(
        _cls, globals(), ('broadcast', 'flatten', 'fold', 'transpose', 'all', 'any',
                          'mean', 'sum', 'nanmean', 'nansum', 'floor', 'ceil', 'round'))
del _cls
# Assign method binding for JUST Variable
_binding.bind_functions_as_methods(Variable, globals(),
                                   ('cumsum', 'max', 'min', 'nanmax', 'nanmin'))
for _cls in (DataArray, Dataset):
    _binding.bind_functions_as_methods(_cls, globals(), ('groupby', 'transform_coords'))
del _cls
del _binding

from . import data

from . import serialization
