# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen

import html
import logging
import time

from .utils import running_in_jupyter
from ._styling import inject_style


def get_logger() -> logging.Logger:
    logger = logging.getLogger('scipp')
    if not get_logger.is_set_up:
        _setup_logger(logger)
    return logger


get_logger.is_set_up = False


def _setup_logger(logger: logging.Logger) -> None:
    logger.setLevel(logging.INFO)
    logger.addHandler(make_stream_handler())
    if running_in_jupyter():
        logger.addHandler(make_widget_handler())


def make_stream_handler() -> logging.StreamHandler:
    handler = logging.StreamHandler()
    handler.setLevel(logging.WARN)
    return handler


if running_in_jupyter():
    from ipywidgets import HTML

    class LogWidget(HTML):
        def __init__(self, **kwargs):
            super().__init__(**kwargs)
            self._rows_str = ''

        def add_row(self, time_stamp: str, level: str, message: str) -> None:
            self._rows_str += self._format_row(time_stamp, level, message)
            self._update()

        @staticmethod
        def _format_row(time_stamp: str, level: str, message: str) -> str:
            return (f'<tr><td class="sc-log-time-stamp">{html.escape(time_stamp)}</td> '
                    f'<td class="sc-log-level">{level}</td> '
                    f'<td class="sc-log-message">{html.escape(message)}</td></tr>')

        def _update(self) -> None:
            self.value = f'<div class="sc-log"><table>{self._rows_str}</table></div>'

        def clear(self) -> None:
            self._rows_str = ''
            self._update()


def make_log_widget():
    if not running_in_jupyter():
        raise RuntimeError('Cannot construct a logging widget because '
                           'Python is not running in a Jupyter notebook.')
    else:
        return LogWidget()


def get_log_widget():
    logger = get_logger()
    try:
        return next(
            filter(lambda handler: isinstance(handler, WidgetHandler),
                   logger.handlers)).widget
    except StopIteration:
        raise ValueError("The logger has no widget handler.") from None


def display_logs() -> None:
    from IPython.display import display
    inject_style()
    display(get_log_widget())


def clear_log_widget() -> None:
    get_log_widget().clear()


class WidgetHandler(logging.Handler):
    def __init__(self, level: int, widget):
        super().__init__(level)
        self.widget = widget
        self._rows = []

    def emit(self, record: logging.LogRecord) -> None:
        time_stamp = time.strftime('%Y-%m-%dT%H:%M:%S', time.localtime(record.created))
        self.widget.add_row(time_stamp, record.levelname, self.format(record))


def make_widget_handler() -> WidgetHandler:
    handler = WidgetHandler(level=logging.INFO, widget=make_log_widget())
    return handler
