# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Jan-Lukas Wynen

import importlib.resources as pkg_resources


def load_style() -> str:
    """
    Load the bundled CSS style and return it as a string.
    The string is cached upon first call.
    """
    if load_style.style is None:
        load_style.style = pkg_resources.read_text('scipp.html', 'style.css')
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
