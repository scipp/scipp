# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
from .table_html import make_html


def _repr_html_():
    import inspect
    # Is there a better way to get the scope? The `7` is hard-coded for the
    # current IPython stack when calling _repr_html_ so this is bound to break.
    scope = inspect.stack()[7][0].f_globals
    from IPython import get_ipython
    ipython = get_ipython()
    out = ''
    for category in ['Variable', 'DataArray', 'Dataset']:
        names = ipython.magic(f"who_ls {category}")
        out += f"<details open=\"open\"><summary>{category}s:"\
               f"({len(names)})</summary>"
        for name in names:
            html = make_html(eval(name, scope))
            out += f"<details style=\"padding-left:2em\"><summary>"\
                   f"{name}</summary>{html}</details>"
        out += "</details>"
    from IPython.core.display import display, HTML
    display(HTML(out))
