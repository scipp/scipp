# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)


def plot(*args, **kwargs):
    """
    Wrapper function to plot data.
    See https://scipp.github.io/plopp/reference/generated/plopp.plot.html for details.
    """
    from plopp import plot as _plot

    return _plot(*args, **kwargs)
