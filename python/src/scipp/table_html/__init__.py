# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Dimitar Tasev
from .._scipp import core as sc
from .formatting_html import dataset_repr, variable_repr


def to_html(container):
    from IPython.display import display, HTML

    if isinstance(container, sc.Variable) or isinstance(
            container, sc.VariableProxy):
        rep = variable_repr(container)
    else:
        rep = dataset_repr(container)
    display(HTML(rep))
