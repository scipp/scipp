# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Jan-Lukas Wynen

"""Functions to convert to/from data structures of other libraries."""

from .pandas_compat import from_pandas
from .xarray_compat import from_xarray, to_xarray

__all__ = ['from_pandas', 'from_xarray', 'to_xarray']
