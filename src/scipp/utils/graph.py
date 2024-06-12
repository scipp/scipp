# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

from typing import TYPE_CHECKING, Any

if TYPE_CHECKING:
    try:
        from graphviz import Digraph
    except ImportError:
        Digraph = Any
else:
    Digraph = Any


def make_graphviz_digraph(*args: Any, **kwargs: Any) -> Digraph:
    try:
        from graphviz import Digraph
    except ImportError as err:
        raise RuntimeError(
            "Failed to import `graphviz`. "
            "Use `pip install graphviz` (requires installed `graphviz` executable) or "
            "`conda install -c conda-forge python-graphviz`."
        ) from err
    return Digraph(*args, **kwargs)
