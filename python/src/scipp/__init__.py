# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
from ._scipp import *
from .table import table
from .plot_matplotlib import plot
# try:
    
# except ImportError:
#     print("Warning: the plotting module for scipp was not imported. Check "
#           "that plotly is installed on your system. You can still use scipp "
#           "without its plotting functionality enabled.")
