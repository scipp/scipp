# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Jan-Lukas Wynen

import importlib.resources
from functools import lru_cache, partial
from string import Template


def _read_text(filename):
    if hasattr(importlib.resources, 'files'):
        # Use new API added in Python 3.9
        return importlib.resources.files('scipp.html').joinpath(filename).read_text()
    # Old API, deprecated as of Python 3.11
    return importlib.resources.read_text('scipp.html', filename)


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


@lru_cache(maxsize=1)
def load_style_sheet() -> str:
    """
    Load the bundled CSS style and return it as a string.
    The string is cached upon first call.
    """
    return _preprocess_style(_read_text('style.css.template'))


def load_style() -> str:
    """
    Load the bundled CSS style and return it within <style> tags.
    """
    return '<style id="scipp-style-sheet">' + load_style_sheet() + '</style>'


@lru_cache(maxsize=1)
def load_icons() -> str:
    """
    Load the bundled icons and return them as an HTML string.
    The string is cached upon first call.
    """
    return _read_text('icons-svg-inline.html')


@lru_cache(maxsize=1)
def load_dg_style() -> str:
    """
    Load the bundled CSS style and return it within <style> tags.
    The string is cached upon first call.
    Datagroup stylesheet is dependent on ``style.css.template``.
    Therefore it loads both ``style.css.template`` and ``datagroup.css``.
    """
    from .formatting_html import load_style
    from .resources import _preprocess_style, _read_text
    style_sheet = '<style id="datagroup-style-sheet">' + \
                  _preprocess_style(_read_text('datagroup.css')) + \
                  '</style>'

    return load_style() + style_sheet


@lru_cache(maxsize=1)
def _load_all_templates() -> str:
    """All HTML Templates for DataGroup formatting."""
    html_tpl = _read_text('_repr_html_.template.html')
    import re

    # line breaks are not needed
    html_tpl = html_tpl.replace('\n', '')
    # remove comments
    html_tpl = re.sub(r'<!--(.|\s|\n)*?-->', '', html_tpl)
    # remove space around special characters
    html_tpl = re.sub(r'\s*([><])\s*', r'\1', html_tpl)
    return html_tpl


@lru_cache(maxsize=4)
def _load_template(tag: str) -> str:
    """Specific HTML Template with the tag name."""
    html_tpl = _load_all_templates()
    import re

    # retrieve the target tpl
    pattern = r'<' + tag + r'>(.|\s|\n)*?</' + tag + r'>'
    html_tpl = re.search(pattern, html_tpl).group()
    # remove tpl tag
    html_tpl = re.sub(r'<' + tag + r'>', '', html_tpl)
    html_tpl = re.sub(r'</' + tag + r'>', '', html_tpl)
    return html_tpl


load_collapsible_tpl = partial(_load_template, tag="collapsible_template")
load_atomic_tpl = partial(_load_template, tag="atomic_template")
load_dg_detail_tpl = partial(_load_template, tag="dg_detail_template")
load_dg_repr_tpl = partial(_load_template, tag="dg_repr_template")
