# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock

# flake8: noqa

from . import runtime_config
user_configuration_filename = runtime_config.config_filename
config = runtime_config.load()
del runtime_config

from ._scipp import _debug_
if _debug_:
    import warnings

    def custom_formatwarning(msg, *args, **kwargs):
        return str(msg) + '\n'

    warnings.formatwarning = custom_formatwarning
    warnings.warn(
        'You are running a "Debug" build of scipp. For optimal performance use a "Release" build.'
    )

from ._scipp import __version__
# Import classes
from ._scipp.core import Variable, DataArray, Dataset, GroupByDataArray, \
                         GroupByDataset, Unit, Coords, Masks
# Import errors
from ._scipp.core import BinEdgeError, BinnedDataError, CoordError, \
                         DataArrayError, DatasetError, DimensionError, \
                         DTypeError, NotFoundError, SizeError, SliceError, \
                         UnitError, VariableError, VariancesError
# Import submodules
from ._scipp.core import units, dtype, buckets, geometry
# Import functions
from ._scipp.core import choose, divide, floor_divide, logical_and, \
                         logical_or, logical_xor, minus, mod, plus, times
# Import python functions
from .show import show, make_svg
from .table import table
from .plotting import plot
from .extend_units import *
from .html import to_html, make_html
from .object_list import _repr_html_
from ._utils import collapse, slices
from ._utils.typing import is_variable, is_dataset, is_data_array, \
                           is_dataset_or_array
from .compat.dict import to_dict, from_dict, map_to_dict
from .sizes import _make_sizes

# Wrappers for free functions from _scipp.core
from ._bins import *
from ._counts import *
from ._comparison import *
from ._cumulative import *
from ._dataset import *
from ._groupby import *
from ._math import *
from ._operations import *
from ._unary import *
from ._reduction import *
from ._shape import *
from ._trigonometry import *
from ._variable import *

setattr(Variable, '_repr_html_', make_html)
setattr(DataArray, '_repr_html_', make_html)
setattr(Dataset, '_repr_html_', make_html)

from .io.hdf5 import to_hdf5 as _to_hdf5
setattr(Variable, 'to_hdf5', _to_hdf5)
setattr(DataArray, 'to_hdf5', _to_hdf5)
setattr(Dataset, 'to_hdf5', _to_hdf5)

setattr(Variable, 'sizes', property(_make_sizes))
setattr(DataArray, 'sizes', property(_make_sizes))
setattr(Dataset, 'sizes', property(_make_sizes))

from ._bins import _bins, _set_bins, _events
setattr(Variable, 'bins', property(_bins, _set_bins))
setattr(DataArray, 'bins', property(_bins, _set_bins))
setattr(Dataset, 'bins', property(_bins, _set_bins))
setattr(Variable, 'events', property(_events))
setattr(DataArray, 'events', property(_events))

from ._bins import _groupby_bins
setattr(GroupByDataArray, 'bins', property(_groupby_bins))
setattr(GroupByDataset, 'bins', property(_groupby_bins))

setattr(Coords, 'to_dict', map_to_dict)
setattr(Masks, 'to_dict', map_to_dict)

setattr(Variable, 'plot', plot)
setattr(DataArray, 'plot', plot)
setattr(Dataset, 'plot', plot)

# Prevent unwanted conversion to numpy arrays by operations. Properly defining
# __array_ufunc__ should be possible by converting non-scipp arguments to
# variables. The most difficult part is probably mapping the ufunc to scipp
# functions.
for _obj in [Variable, DataArray, Dataset]:
    setattr(_obj, '__array_ufunc__', None)
