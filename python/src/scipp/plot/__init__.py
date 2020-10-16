# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

from .plot import plot as _plot

# If we are running inside a notebook, then make plot interactive by default.
# From: https://stackoverflow.com/a/22424821
is_doc_build = False
try:
    from IPython import get_ipython
    import matplotlib as mpl
    ipy = get_ipython()
    if ipy is not None:

        # Check if a docs build is requested in the metadata. If so,
        # use the default Qt/inline backend.
        cfg = ipy.config
        try:
            meta = cfg["Session"]["metadata"]
            if hasattr(meta, "to_dict"):
                meta = meta.to_dict()
            is_doc_build = meta["scipp_docs_build"]
        except KeyError:
            is_doc_build = False

        # If we are in an IPython kernel, select widget backend
        if "IPKernelApp" in ipy.config and not is_doc_build:
            mpl.use('module://ipympl.backend_nbagg')
            # Hide the figure header:
            # see https://github.com/matplotlib/ipympl/issues/229
            from ipympl.backend_nbagg import Canvas
            Canvas.header_visible.default_value = False

except ImportError:
    pass

if is_doc_build:
    mpl.pyplot.rcParams.update({'figure.max_open_warning': 0})


def plot(*args, **kwargs):
    # Determine if we are using a static or interactive backend
    interactive = mpl.get_backend().lower().endswith('nbagg')
    # Switch auto figure display off for better control over when figures are
    # displayed.
    mpl.pyplot.ioff()
    output = _plot(*args, **kwargs)
    if not interactive:
        for key in output:
            output[key].as_static(keep_widgets=is_doc_build)
    # Turn auto figure display back on.
    # TODO: we need to consider whether users manually turned auto figure
    # display off, in which case we would not want to turn it back on here.
    mpl.pyplot.ion()
    return output


def superplot(*args, **kwargs):
    return plot(*args, projection="1d", **kwargs)


def image(*args, **kwargs):
    return plot(*args, projection="2d", **kwargs)


def scatter3d(*args, **kwargs):
    return plot(*args, projection="3d", **kwargs)
