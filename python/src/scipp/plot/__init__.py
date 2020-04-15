# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

import importlib

# If we are running inside a notebook, then make plot interactive by default.
# From: https://stackoverflow.com/a/22424821
try:
    import matplotlib
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

            # If we are in an IPython kernel, select either widget backend
            # if installed (for JupyterLan), or notebook backend.
            if "IPKernelApp" in ipy.config and not is_doc_build:
                try:
                    _ = importlib.import_module("ipympl")
                    matplotlib.use('module://ipympl.backend_nbagg')
                except ImportError:
                    matplotlib.use('nbAgg')
                    # Remove the title banner and button from figure
                    ipy.run_cell_magic(
                        "html", "", "<style>.output_wrapper "
                        ".ui-dialog-titlebar {display: none;}</style>")
    except ImportError:
        pass
    # Turn interactive plotting on
    import matplotlib.pyplot as plt
    plt.ion()
    if is_doc_build:
        plt.rcParams.update({'figure.max_open_warning': 0})

except ImportError:
    pass

from .plot import plot


def superplot(dataset, **kwargs):
    return plot(dataset, projection="1d", **kwargs)


def image(dataset, **kwargs):
    return plot(dataset, projection="2d", **kwargs)


def threeslice(dataset, **kwargs):
    return plot(dataset, projection="3d", **kwargs)
