# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Jan-Lukas Wynen

import importlib.resources
from functools import lru_cache, partial


def _read_text(filename: str) -> str:
    return (
        importlib.resources.files('scipp.visualization.templates')
        .joinpath(filename)
        .read_text(encoding='utf-8')
    )


def _preprocess_style(raw_css: str) -> str:
    import re

    # line breaks are not needed
    css = raw_css.replace('\n', '')
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
    return _preprocess_style(_read_text('style.css'))


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
    Datagroup stylesheet is dependent on ``style.css``.
    Therefore it loads both ``style.css`` and ``datagroup.css``.
    """
    style_sheet = (
        '<style id="datagroup-style-sheet">'
        + _preprocess_style(_read_text('datagroup.css'))
        + '</style>'
    )

    return load_style() + style_sheet


@lru_cache(maxsize=4)
def _load_template(name: str) -> str:
    """HTML template in scipp.visualization.templates"""
    html_tpl = _read_text(name + '.html')
    import re

    # line breaks are not needed
    html_tpl = html_tpl.replace('\n', '')
    # remove comments
    html_tpl = re.sub(r'<!--(.|\s|\n)*?-->', '', html_tpl)
    # remove space around special characters
    html_tpl = re.sub(r'\s*([><])\s*', r'\1', html_tpl)
    return html_tpl


load_atomic_row_tpl = partial(_load_template, name="dg_atomic_row")
load_collapsible_row_tpl = partial(_load_template, name="dg_collapsible_row")
load_dg_detail_list_tpl = partial(_load_template, name="dg_detail_list")
load_dg_repr_tpl = partial(_load_template, name="dg_repr")
