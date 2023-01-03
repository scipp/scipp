# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)


def make_graphviz_digraph(*args, **kwargs):
    try:
        from graphviz import Digraph
    except ImportError as err:
        raise RuntimeError(
            "Failed to import `graphviz`. "
            "Use `pip install graphviz` (requires installed `graphviz` executable) or "
            "`conda install -c conda-forge python-graphviz`.") from err
    return Digraph(*args, **kwargs)
