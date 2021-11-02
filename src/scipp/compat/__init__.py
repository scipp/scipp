# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Jan-Lukas Wynen

from .pandas_compat import from_pandas
from .xarray_compat import from_xarray

__all__ = ['from_pandas', 'from_xarray']
