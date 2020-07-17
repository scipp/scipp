# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
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
from . import plot
from .extend_units import *
from .table_html import to_html, make_html
from .object_list import _repr_html_
from ._utils import collapse, slices
from .compat.dict import to_dict, from_dict

setattr(Variable, '_repr_html_', make_html)
setattr(VariableConstView, '_repr_html_', make_html)
setattr(DataArray, '_repr_html_', make_html)
setattr(DataArrayConstView, '_repr_html_', make_html)
setattr(Dataset, '_repr_html_', make_html)
setattr(DatasetConstView, '_repr_html_', make_html)
