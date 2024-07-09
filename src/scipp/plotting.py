# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

from typing import Any


def plot(*args: Any, **kwargs: Any) -> Any:
    """
    Wrapper function to plot data.
    See https://scipp.github.io/plopp/ for details.
    """
    from plopp import plot as _plot

    return _plot(*args, **kwargs)
