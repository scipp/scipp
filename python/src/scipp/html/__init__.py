# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Dimitar Tasev
from .._utils.typing import is_variable
from .formatting_html import dataset_repr, inject_style, variable_repr


def make_html(container):
    inject_style()
    if is_variable(container):
        return variable_repr(container)
    else:
        return dataset_repr(container)


def to_html(container):
    from IPython.display import display, HTML
    display(HTML(make_html(container)))
