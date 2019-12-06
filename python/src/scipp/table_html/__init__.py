# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Dimitar Tasev
from scipp.table_html.formatting_html import dataset_repr


def to_html(dataset):
    from IPython.display import display, HTML

    display(HTML(dataset_repr(dataset)))
