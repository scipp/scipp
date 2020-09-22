# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

# If we are running inside a notebook, then make plot interactive by default.
# From: https://stackoverflow.com/a/22424821
try:
    is_doc_build = False
    try:
        from IPython import get_ipython
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
                import matplotlib as mpl
                mpl.use('module://ipympl.backend_nbagg')
                # Hide the figure header:
                # see https://github.com/matplotlib/ipympl/issues/229
                from ipympl.backend_nbagg import Canvas
                Canvas.header_visible.default_value = False

    except ImportError:
        pass

    if is_doc_build:
        import matplotlib.pyplot as plt
        plt.rcParams.update({'figure.max_open_warning': 0})

except ImportError:
    pass

from .plot import plot


def superplot(scipp_obj, **kwargs):
    return plot(scipp_obj, projection="1d", **kwargs)


def image(scipp_obj, **kwargs):
    return plot(scipp_obj, projection="2d", **kwargs)


def scatter3d(scipp_obj, **kwargs):
    return plot(scipp_obj, projection="3d", **kwargs)
