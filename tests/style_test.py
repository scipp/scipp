# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 Scipp contributors (https://github.com/scipp)

from pathlib import Path

TEMPLATE_ROOT = (
    Path(__file__).resolve().parents[1] / 'src/scipp/visualization/templates'
)


def test_html_stylesheets_use_flat_rules() -> None:
    style = TEMPLATE_ROOT.joinpath('style.css').read_text(encoding='utf-8')
    datagroup_style = TEMPLATE_ROOT.joinpath('datagroup.css').read_text(
        encoding='utf-8'
    )

    assert '@scope' not in style
    assert '@scope' not in datagroup_style
    assert '&' not in style
    assert '&' not in datagroup_style

    for selector in (
        '.sc-root.sc-wrap,',
        '.sc-root .sc-wrap {',
        '.sc-root ul.sc-sections {',
        '.sc-root pre.sc-var-data {',
        '.sc-root dl.sc-attrs {',
    ):
        assert selector in style

    for selector in (
        '.dg-root * {',
        '.dg-root input.dg-header-in {',
        '.dg-root ul.dg-detail-list {',
    ):
        assert selector in datagroup_style
