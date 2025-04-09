# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
from .visualization import make_html


def _repr_html_() -> None:
    import inspect

    # Is there a better way to get the scope? The `7` is hard-coded for the
    # current IPython stack when calling _repr_html_ so this is bound to break.
    scope = inspect.stack()[7][0].f_globals
    from IPython import get_ipython  # type: ignore[attr-defined]

    ipython = get_ipython()  # type: ignore[no-untyped-call]
    out = ''
    for category in ['Variable', 'DataArray', 'Dataset', 'DataGroup']:
        names = ipython.magic(f"who_ls {category}")
        out += f"<details open=\"open\"><summary>{category}s:({len(names)})</summary>"
        for name in names:
            html = make_html(eval(name, scope))  # noqa: S307
            out += (
                f"<details style=\"padding-left:2em\"><summary>"
                f"{name}</summary>{html}</details>"
            )
        out += "</details>"
    from IPython.display import HTML, display

    display(HTML(out))  # type: ignore[no-untyped-call]
