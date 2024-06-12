# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen
"""Sphinx extension to automatically place plot directives.

Places `matplotlib plot directives <https://matplotlib.org/stable/api/sphinxext_plot_directive_api.html>`_
for doctest code blocks (starting with ``>>>``) in docstrings when those
code blocks create plots.

Plots are detected by matching a pattern against the source code in code blocks.
This means that either ``sc.plot`` or any of the corresponding bound methods must be
called directly in the code block.
It does not work if ``plot`` is called by another function.
It does however work with any function called 'plot'.

It is possible to use the plot directive manually.
``autoplot`` will detect that and not place an additional directive.
This is useful, e.g., for specifying options to the directive.

Simple Example
--------------
``autoplot`` turns

.. autoplot-disable::

.. code-block:: text

    >>> sc.arange('x',5).plot()

into

.. code-block:: text

    .. plot::

        >>> sc.arange('x',5).plot()

Disabling autoplot
------------------
``autoplot`` can be disabled by using the ``.. autoplot-disable::`` directive
anywhere (on its own line) in a docstring.
This will disable autoplot for the entire docstring.
"""

import re

from sphinx.application import Sphinx
from sphinx.util.docutils import SphinxDirective

# If this pattern matches any line of a code block,
# that block is assumed to produce plots.
PLOT_PATTERN = re.compile(r'\.plot\(')
# Pattern to detect existing plot directives.
PLOT_DIRECTIVE_PATTERN = re.compile(r'^\s*\.\. plot::.*')
# Pattern to detect directive to disable autoplot.
DISABLE_DIRECTIVE_PATTERN = re.compile(r'^\s*\.\. autoplot-disable::.*')


def _previous_nonempty_line(lines: list[str], index: int) -> str | None:
    index -= 1
    while index >= 0 and not lines[index].strip():
        index -= 1
    return lines[index] if index >= 0 else None


def _indentation_of(s: str) -> int:
    return len(s) - len(s.lstrip())


def _process_block(lines: list[str], begin: int, end: int) -> list[str]:
    block = lines[begin:end]
    if PLOT_PATTERN.search(block[-1]) is not None:
        prev = _previous_nonempty_line(lines, begin)
        if prev and PLOT_DIRECTIVE_PATTERN.match(prev) is None:
            # If the block is not preceded by any text or directives (e.g. comes right
            # after the 'Examples' header), its indentation is stripped in `lines`.
            # Adding some indentation here fixes that but also modifies indentation
            # if the block is already indented. That should not cause problems.
            return [
                ' ' * _indentation_of(prev) + '.. plot::',
                '',
                *('    ' + x for x in block),
            ]
    return block


def _is_start_of_block(line: str) -> bool:
    return line.lstrip().startswith('>>>')


def _is_part_of_block(line: str) -> bool:
    stripped = line.lstrip()
    return stripped.startswith('>>>') or stripped.startswith('...')


def add_plot_directives(
    app: object,
    what: object,
    name: object,
    obj: object,
    options: object,
    lines: list[str],
) -> None:
    new_lines = []
    block_begin: int | None = None
    for i, line in enumerate(lines):
        if DISABLE_DIRECTIVE_PATTERN.match(line):
            return
        if block_begin is None:
            if _is_start_of_block(line):
                # Begin a new block.
                block_begin = i
            else:
                # Not in a block -> copy line into output.
                new_lines.append(line)
        else:
            if not _is_part_of_block(line):
                # Block has ended.
                new_lines.extend(_process_block(lines, block_begin, i))
                new_lines.append(line)
                block_begin = None
            # else: Continue block.

    lines[:] = new_lines


class AutoplotDisable(SphinxDirective):
    has_content = False

    def run(self) -> list[object]:
        return []


def setup(app: Sphinx) -> dict[str, int | bool]:
    app.add_directive('autoplot-disable', AutoplotDisable)
    app.connect('autodoc-process-docstring', add_plot_directives)
    return {'version': 1, 'parallel_read_safe': True}
