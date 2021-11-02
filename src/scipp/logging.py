# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen
"""
Utilities for managing scipp's logger and log widget.
"""

import html
import logging
import time
from typing import Optional

from .utils import running_in_jupyter
from ._styling import inject_style


def get_logger() -> logging.Logger:
    """
    Return the global logger used by scipp.
    """
    logger = logging.getLogger('scipp')
    if not get_logger.is_set_up:
        _setup_logger(logger)
        get_logger.is_set_up = True
    return logger


get_logger.is_set_up = False


def _setup_logger(logger: logging.Logger) -> None:
    logger.setLevel(logging.INFO)
    logger.addHandler(make_stream_handler())
    if running_in_jupyter():
        logger.addHandler(make_widget_handler())


def make_stream_handler() -> logging.StreamHandler:
    """
    Make a new ``StreamHandler`` with default setup for scipp.
    """
    handler = logging.StreamHandler()
    handler.setLevel(logging.WARN)
    handler.setFormatter(
        logging.Formatter('[%(asctime)s] %(levelname)-8s : %(message)s',
                          datefmt='%Y-%m-%dT%H:%M:%S'))
    return handler


if running_in_jupyter():
    from ipywidgets import HTML

    class LogWidget(HTML):
        """
        Widget that displays log messages in a table.
        """
        def __init__(self, **kwargs):
            super().__init__(**kwargs)
            self._rows_str = ''

        def add_message(self, name: str, time_stamp: str, level: str,
                        message: str) -> None:
            """
            Add a message to the output.
            :param name: Name of the logger.
            :param time_stamp: Formatted time when the message was recorded.
            :param level: Name of the log level.
            :param message: The actual message.
            """
            self._rows_str += self._format_row(name, time_stamp, level, message)
            self._update()

        @staticmethod
        def _format_row(name: str, time_stamp: str, level: str, message: str) -> str:
            return (f'<tr class="sc-log-{level.lower()}">'
                    f'<td class="sc-log-time-stamp">[{html.escape(time_stamp)}]</td>'
                    f'<td class="sc-log-level">{level}</td>'
                    f'<td class="sc-log-message">{html.escape(message)}</td>'
                    f'<td class="sc-log-name">&lt;{html.escape(name)}&gt;</td>'
                    f'</tr>')

        def _update(self) -> None:
            self.value = f'<div class="sc-log"><table>{self._rows_str}</table></div>'

        def clear(self) -> None:
            """
            Remove all output from the widget.
            """
            self._rows_str = ''
            self._update()


def make_log_widget() -> 'LogWidget':
    """
    Create and return a new ``LogWidget`` for use with ``WidgetHandler``
    in a Jupyter notebook.

    :raises RuntimeError: If Python is not running in Jupyter.
    """
    if not running_in_jupyter():
        raise RuntimeError('Cannot construct a logging widget because '
                           'Python is not running in a Jupyter notebook.')
    else:
        return LogWidget()


def get_log_widget() -> Optional['LogWidget']:
    """
    Return the log widget used by the global scipp logger.
    If multiple widget handlers are installed, only the first one is returned.
    If no widget handler is installed, returns ``None``.
    """
    handler = get_widget_handler()
    if handler is None:
        return None
    return handler.widget


def display_logs() -> None:
    """
    Display the log widget associated with the global scipp logger.
    """
    widget = get_log_widget()
    if widget is None:
        raise RuntimeError(
            'Cannot display log widgets because no widget handler is installed.')

    from IPython.display import display
    inject_style()
    display(widget)


def clear_log_widget() -> None:
    """
    Remove the current output of the log widget of the global scipp logger
    if there is one.
    """
    widget = get_log_widget()
    if widget is None:
        return
    widget.clear()


class WidgetHandler(logging.Handler):
    """
    Logging handler that sends messages to a ``LogWidget``
    for display in Jupyter notebooks.
    """
    def __init__(self, level: int, widget: 'LogWidget'):
        super().__init__(level)
        self.widget = widget
        self._rows = []

    def emit(self, record: logging.LogRecord) -> None:
        """
        Send the formatted record to the widget.
        """
        time_stamp = time.strftime('%Y-%m-%dT%H:%M:%S', time.localtime(record.created))
        self.widget.add_message(record.name, time_stamp, record.levelname,
                                self.format(record))


def make_widget_handler() -> WidgetHandler:
    """
    Create a new widget log handler.
    :raises RuntimeError: If Python is not running in Jupyter.
    """
    handler = WidgetHandler(level=logging.INFO, widget=make_log_widget())
    return handler


def get_widget_handler() -> Optional[WidgetHandler]:
    """
    Return the widget handler installed in the global scipp logger.
    If multiple widget handlers are installed, only the first one is returned.
    If no widget handler is installed, returns ``None``.
    """
    try:
        return next(  # type: ignore
            filter(lambda handler: isinstance(handler, WidgetHandler),
                   get_logger().handlers))
    except StopIteration:
        return None
