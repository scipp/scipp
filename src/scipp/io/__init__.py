# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock

# ruff: noqa: F403

"""File input and output.

All functions listed here are also available in :mod:`scipp.io` module directly,
e.g., as ``scipp.io.load_hdf5``.

See also `Reading and Writing Files <../../user-guide/reading-and-writing-files.ipynb>`_.

Functions
---------
.. currentmodule:: scipp.io

.. autosummary::

   csv.load_csv
   hdf5.load_hdf5
   hdf5.save_hdf5
"""  # noqa: E501

from .csv import load_csv
from .hdf5 import load_hdf5, save_hdf5

__all__ = ["load_csv", "load_hdf5", "save_hdf5"]
