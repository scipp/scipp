# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Dimitar Tasev

from __future__ import annotations

from ..typing import VariableLike
from .._scipp import core as sc
from .formatting_html import dataset_repr, inject_style, variable_repr


def make_html(container: VariableLike) -> str:
    inject_style()
    if isinstance(container, sc.Variable):
        return variable_repr(container)
    else:
        return dataset_repr(container)


def to_html(container: VariableLike):
    from IPython.display import display, HTML
    display(HTML(make_html(container)))
