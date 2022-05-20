# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)


def make_graphviz_digraph(*args, **kwargs):
    try:
        from graphviz import Digraph
    except ImportError:
        raise RuntimeError(
            "Failed to import `graphviz`. "
            "Use `pip install graphviz` (requires installed `graphviz` executable) or "
            "`conda install -c conda-forge python-graphviz`.")
    return Digraph(*args, **kwargs)
