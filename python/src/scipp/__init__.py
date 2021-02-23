# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock

# flake8: noqa

from . import runtime_config
user_configuration_filename = runtime_config.config_filename
config = runtime_config.load()
del runtime_config

from ._scipp.core import *
from ._scipp import __version__
from . import detail
from . import neutron
from .show import show, make_svg
from .table import table
from .plotting import plot
from .extend_units import *
from .table_html import to_html, make_html
from .object_list import _repr_html_
from ._utils import collapse, slices
from ._utils.is_type import is_variable, is_dataset, is_data_array, \
                            is_dataset_or_array
from .compat.dict import to_dict, from_dict
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
setattr(VariableConstView, '_repr_html_', make_html)
setattr(DataArray, '_repr_html_', make_html)
setattr(DataArrayConstView, '_repr_html_', make_html)
setattr(Dataset, '_repr_html_', make_html)
setattr(DatasetConstView, '_repr_html_', make_html)

from .io.hdf5 import to_hdf5 as _to_hdf5
setattr(Variable, 'to_hdf5', _to_hdf5)
setattr(VariableView, 'to_hdf5', _to_hdf5)
setattr(DataArray, 'to_hdf5', _to_hdf5)
setattr(DataArrayView, 'to_hdf5', _to_hdf5)
setattr(Dataset, 'to_hdf5', _to_hdf5)
setattr(DatasetView, 'to_hdf5', _to_hdf5)

setattr(Variable, 'sizes', property(_make_sizes))
setattr(VariableView, 'sizes', property(_make_sizes))
setattr(DataArray, 'sizes', property(_make_sizes))
setattr(DataArrayView, 'sizes', property(_make_sizes))
setattr(Dataset, 'sizes', property(_make_sizes))
setattr(DatasetView, 'sizes', property(_make_sizes))

from ._bins import _bins, _set_bins
setattr(Variable, 'bins', property(_bins, _set_bins))
setattr(VariableView, 'bins', property(_bins, _set_bins))
setattr(DataArray, 'bins', property(_bins, _set_bins))
setattr(DataArrayView, 'bins', property(_bins, _set_bins))
setattr(Dataset, 'bins', property(_bins, _set_bins))
setattr(DatasetView, 'bins', property(_bins, _set_bins))

from ._bins import _groupby_bins
setattr(GroupByDataArray, 'bins', property(_groupby_bins))
setattr(GroupByDataset, 'bins', property(_groupby_bins))

setattr(Variable, 'plot', plot)
setattr(VariableConstView, 'plot', plot)
setattr(DataArray, 'plot', plot)
setattr(DataArrayConstView, 'plot', plot)
setattr(Dataset, 'plot', plot)
setattr(DatasetConstView, 'plot', plot)
