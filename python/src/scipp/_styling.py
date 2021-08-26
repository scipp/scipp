# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)


def inject_style():
    """
    Add our CSS style to the HTML head so that it can be used by all
    HTML and SVG output without duplicating it in every cell.
    This also preserves the style when the output in Jupyter is cleared.

    The style is only injected once per session.
    """
    if not inject_style._has_been_injected:
        from IPython.display import display, Javascript
        from .html.resources import load_style
        # `display` claims that its parameter should be a tuple, but
        # that does not seem to work in the case of Javascript.
        display(
            Javascript(f"""
            const style = document.createElement('style');
            style.textContent = String.raw`{load_style()}`;
            document.head.append(style);
            """))
    inject_style._has_been_injected = True


inject_style._has_been_injected = False
