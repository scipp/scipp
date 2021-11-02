# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Jan-Lukas Wynen

import importlib.resources as pkg_resources
from string import Template


def _format_style(template: str) -> str:
    from .. import config
    # Color patterns in the CSS template use the name in
    # the config file plus a _color suffix.
    return Template(template).substitute(
        **{f'{key}_color': val
           for key, val in config['colors'].items()})


def _preprocess_style(template: str) -> str:
    css = _format_style(template)
    import re
    # line breaks are not needed
    css = css.replace('\n', '')
    # remove comments
    css = re.sub(r'/\*(\*(?!/)|[^*])*\*/', '', css)
    # remove space around special characters
    css = re.sub(r'\s*([;{}:,])\s*', r'\1', css)
    return css


def load_style() -> str:
    """
    Load the bundled CSS style and return it as a string.
    The string is cached upon first call.
    """
    if load_style.style is None:
        load_style.style = _preprocess_style(
            pkg_resources.read_text('scipp.html', 'style.css.template'))
    return load_style.style


load_style.style = None


def load_icons() -> str:
    """
    Load the bundled icons and return them as an HTML string.
    The string is cached upon first call.
    """
    if load_icons.icons is None:
        load_icons.icons = pkg_resources.read_text('scipp.html',
                                                   'icons-svg-inline.html')
    return load_icons.icons


load_icons.icons = None
