# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen
"""
Utilities for managing Scipp's logger and log widget.

See the https://scipp.github.io/reference/logging.html for an overview.
"""

import html
import logging
import time
from collections.abc import Iterable
from copy import copy
from dataclasses import dataclass
from typing import Any

from .core import DataArray, Dataset, Variable
from .typing import VariableLike
from .utils import running_in_jupyter
from .visualization import make_html
from .visualization.resources import load_style


def get_logger() -> logging.Logger:
    """
    Return the global logger used by Scipp.
    """
    return logging.getLogger('scipp')


@dataclass
class WidgetLogRecord:
    """
    Preprocessed data for display in ``LogWidget``.
    """

    name: str
    levelname: str
    time_stamp: str
    message: str


def make_log_widget(**widget_kwargs: object) -> object:
    if not running_in_jupyter():
        raise RuntimeError(
            'Cannot create a logging widget because '
            'Python is not running in a Jupyter notebook.'
        )

    from ipywidgets import HTML

    class LogWidget(HTML):  # type: ignore[misc]
        """
        Widget that displays log messages in a table.

        :seealso: :py:class:`scipp.logging.WidgetHandler`
        """

        def __init__(self, **kwargs: object) -> None:
            super().__init__(**kwargs)
            self._rows_str = ''
            self._update()

        def add_message(self, record: WidgetLogRecord) -> None:
            """
            Add a message to the output.
            :param record: Log record formatted for the widget.
            """
            self._rows_str += self._format_row(record)
            self._update()

        @staticmethod
        def _format_row(record: WidgetLogRecord) -> str:
            # The message is assumed to be safe HTML.
            # It is WidgetHandler's responsibility to ensure that.
            return (
                f'<tr class="sc-log sc-log-{html.escape(record.levelname.lower())}">'
                f'<td class="sc-log-time-stamp">[{html.escape(record.time_stamp)}]</td>'
                f'<td class="sc-log-level">{html.escape(record.levelname)}</td>'
                f'<td class="sc-log-message">{record.message}</td>'
                f'<td class="sc-log-name">&lt;{html.escape(record.name)}&gt;</td>'
                '</tr>'
            )

        def _update(self) -> None:
            self.value = (
                '<div class="sc-log">'
                f'<table class="sc-log">{self._rows_str}</table>'
                '</div>'
            )

        def clear(self) -> None:
            """
            Remove all output from the widget.
            """
            self._rows_str = ''
            self._update()

    return LogWidget(**widget_kwargs)


def get_log_widget() -> object:
    """
    Return the log widget used by the Scipp logger.
    If multiple widget handlers are installed, only the first one is returned.
    If no widget handler is installed, returns ``None``.
    """
    handler = get_widget_handler()
    if handler is None:
        return None
    return handler.widget


def display_logs() -> None:
    """
    Display the log widget associated with the Scipp logger.
    Only works in Jupyter notebooks.
    """
    widget = get_log_widget()
    if widget is None:
        raise RuntimeError(
            'Cannot display log widgets because no widget handler is installed.'
        )

    from IPython.display import display
    from ipywidgets import HTML, VBox

    display(VBox([HTML(value=load_style()), widget]).add_class('sc-log-wrap'))  # type: ignore[no-untyped-call]


def clear_log_widget() -> None:
    """
    Remove the current output of the log widget of the Scipp logger
    if there is one.
    """
    widget = get_log_widget()
    if widget is None:
        return
    widget.clear()  # type: ignore[attr-defined]


def _has_html_repr(x: object) -> bool:
    return isinstance(x, DataArray | Dataset | Variable)


def _make_html(x: VariableLike) -> str:
    return f'<div class="sc-log-html-payload">{make_html(x)}</div>'


# This class is used with the log formatter to distinguish between str and repr.
# str produces a pattern that can be replaced with HTML and
# repr produces a plain string for the argument.
class _ReplacementPattern:
    _PATTERN = '$__SCIPP_HTML_REPLACEMENT_{:02d}__'

    def __init__(self, i: int, arg: Any) -> None:
        self._i = i
        self._arg = arg

    def __str__(self) -> str:
        return self._PATTERN.format(self._i)

    def __repr__(self) -> str:
        return repr(self._arg)


def _preprocess_format_args(
    args: Iterable[object],
) -> tuple[tuple[object, ...], dict[str, object]]:
    format_args: list[object] = []
    replacements = {}
    for i, arg in enumerate(args):
        if _has_html_repr(arg):
            tag = _ReplacementPattern(i, arg)
            format_args.append(tag)
            replacements[str(tag)] = arg
        else:
            format_args.append(arg)
    return tuple(format_args), replacements


def _replace_html_repr(message: str, replacements: dict[str, object]) -> str:
    # Do separate check `key in message` in order to avoid calling
    # _make_html unnecessarily. Linear string searches are likely less
    # expensive than HTML formatting.
    for key, repl in replacements.items():
        if key in message:
            if isinstance(repl, Variable | DataArray | Dataset):
                message = message.replace(key, _make_html(repl))
            else:
                message = message.replace(key, str(repl))
    return message


class WidgetHandler(logging.Handler):
    """
    Logging handler that sends messages to a ``LogWidget``
    for display in Jupyter notebooks.

    Messages are formatted into a ``WidgetLogRecord`` and not into a string.

    This handler introduces special formatting for objects with an HTML representation.
    If the log message is a single such object, its HTML repr is embedded in the widget.
    Strings are formatted to replace %s with the HTML repr and %r with a plain string
    repr using ``str(x)`` and ``repr(x)`` is inaccessible.
    """

    def __init__(self, level: int, widget: object) -> None:
        super().__init__(level)
        self.widget: Any = widget  # The type is not nameable here.

    def _format_record(self, record: logging.LogRecord) -> WidgetLogRecord:
        """Format the specified record for consumption by a ``LogWidget``."""
        message = (
            self._format_html(record)
            if _has_html_repr(record.msg)
            else self._format_text(record)
        )
        return WidgetLogRecord(
            name=record.name,
            levelname=record.levelname,
            time_stamp=time.strftime(
                '%Y-%m-%dT%H:%M:%S', time.localtime(record.created)
            ),
            message=message,
        )

    def _format_text(self, record: logging.LogRecord) -> str:
        if record.args is None:
            record_args: Iterable[object] = ()
        elif isinstance(record.args, tuple):
            record_args = record.args
        else:
            record_args = record.args.values()
        args, replacements = _preprocess_format_args(record_args)
        record = copy(record)
        record.args = tuple(args)
        message = html.escape(super().format(record))
        return _replace_html_repr(message, replacements)

    @staticmethod
    def _format_html(record: logging.LogRecord) -> str:
        if record.args:
            raise TypeError('not all arguments converted during string formatting')
        return _make_html(record.msg)  # type: ignore[arg-type]

    def emit(self, record: logging.LogRecord) -> None:
        """
        Send the formatted record to the widget.
        """
        self.widget.add_message(self._format_record(record))


def make_widget_handler() -> WidgetHandler:
    """
    Create a new widget log handler.
    :raises RuntimeError: If Python is not running in Jupyter.
    """
    handler = WidgetHandler(level=logging.INFO, widget=make_log_widget())
    return handler


def get_widget_handler() -> WidgetHandler | None:
    """
    Return the widget handler installed in the Scipp logger.
    If multiple widget handlers are installed, only the first one is returned.
    If no widget handler is installed, returns ``None``.
    """
    try:
        return next(  # type: ignore[return-value]
            filter(
                lambda handler: isinstance(handler, WidgetHandler),
                get_logger().handlers,
            )
        )
    except StopIteration:
        return None
