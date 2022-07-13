# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
"""Sub-package for lower-level control over binning and histogramming.
"""

# flake8: noqa
from ..core.binning import make_binned, make_histogrammed

# This makes Sphinx display these functions in the docs.
make_binned.__module__ = 'scipp.binning'
make_histogrammed.__module__ = 'scipp.binning'

__all__ = ['make_binned', 'make_histogrammed']
