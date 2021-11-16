# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)


def inject_style():
    """
    Add our CSS style to the HTML head so that it can be used by all
    HTML and SVG output without duplicating it in every cell.
    This also preserves the style when the output in Jupyter is cleared.

    The style is only injected once per session but if the kernel is restarted,
    the old style (if present in the HTML) is replaced.
    """
    if not inject_style._has_been_injected:
        from IPython.display import display, Javascript
        from .html.resources import load_style
        display(
            Javascript(f"""
            const style = document.createElement('style');
            style.setAttribute('id', 'scipp-style-sheet');
            style.textContent = String.raw`{load_style()}`;
            const oldStyle = document.getElementById('scipp-style-sheet');
            if (oldStyle === null) {{
                document.head.append(style);
            }} else {{
                oldStyle.replaceWith(style);
            }}
            """))
    inject_style._has_been_injected = True


inject_style._has_been_injected = False
