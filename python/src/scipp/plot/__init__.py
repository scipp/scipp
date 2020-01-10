# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

# flake8: noqa

# If we are running inside a notebook, then make plot interactive by default.
# From: https://stackoverflow.com/a/22424821
try:
    from IPython import get_ipython
    ipy = get_ipython()
    if ipy is not None:
        cfg = ipy.config
        try:
            meta = cfg["Session"]["metadata"]
            if hasattr(meta, "to_dict"):
                meta = meta.to_dict()
            is_doc_build = meta["scipp_docs_build"]
        except KeyError:
            is_doc_build = False
        if is_doc_build:
            ipy.run_line_magic("matplotlib", "inline")
        elif "IPKernelApp" in ipy.config:
            ipy.run_line_magic("matplotlib", "notebook")
            ipy.run_cell_magic("html", "", "<style>.output_wrapper "
                               ".ui-dialog-titlebar {display: none;}</style>")
except ImportError:
    pass


from .plot import plot


def superplot(dataset, **kwargs):
    return plot(dataset, projection="1d", **kwargs)

def image(dataset, **kwargs):
    return plot(dataset, projection="2d", **kwargs)

def threeslice(dataset, **kwargs):
    return plot(dataset, projection="3d", **kwargs)
