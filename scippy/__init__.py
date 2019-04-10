# @file
# SPDX-License-Identifier: GPL-3.0-or-later
# @author Simon Heybrock
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
# National Laboratory, and European Spallation Source ERIC.
from .scippy import *
from .table import table
from .plot import plot
# try:
#     from .plot import plot
# except ImportError:
#     print("Warning: the plotting module for Scippy was not imported. Check "
#           "that plotly is installed on your system. You can still use Scippy "
#           "without its plotting functionality enabled.")
