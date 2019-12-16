# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock

# flake8: noqa

from ._scipp.core import *
from ._scipp import __version__
from . import neutron
from .show import show, make_svg
from .table import table
from . import plot
from .extend_units import *
from . import config
from .table_html import to_html, make_html

setattr(Variable, '_repr_html_', make_html)
setattr(VariableConstProxy, '_repr_html_', make_html)
setattr(DataArray, '_repr_html_', make_html)
setattr(DataConstProxy, '_repr_html_', make_html)
setattr(Dataset, '_repr_html_', make_html)
setattr(DatasetConstProxy, '_repr_html_', make_html)
