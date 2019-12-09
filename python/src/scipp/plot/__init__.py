# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

# flake8: noqa

from .plot import plot


# If we are running inside a notebook, then make plot interactive by default.
# From: https://stackoverflow.com/a/22424821
try:
    from IPython import get_ipython
    ipy = get_ipython()
    if ipy is not None:
        if "IPKernelApp" in ipy.config:
            ipy.run_line_magic("matplotlib", "notebook")
except ImportError:
    pass


def superplot(dataset, **kwargs):
    return plot(dataset, projection="1d", **kwargs)

def image(dataset, **kwargs):
    return plot(dataset, projection="2d", **kwargs)

def threeslice(dataset, **kwargs):
    return plot(dataset, projection="3d", **kwargs)
