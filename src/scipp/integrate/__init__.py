# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
# flake8: noqa
"""Sub-package for integration."""

from ._integrate import simpson, trapezoid


__all__ = ['simpson', 'trapezoid']
