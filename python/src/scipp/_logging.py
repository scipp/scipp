# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen

import logging

from .utils import running_in_jupyter


def _setup_logger(logger: logging.Logger) -> None:
    logger.setLevel(logging.INFO)
    if running_in_jupyter():
        install_widget_handler(logger)


def get_logger() -> logging.Logger:
    logger = logging.getLogger('scipp')
    if not get_logger.is_set_up:
        _setup_logger(logger)
    return logger


get_logger.is_set_up = False


class WidgetHandler(logging.Handler):
    def __init__(self, level, widget):
        super().__init__(level)
        self.widget = widget

    def emit(self, record: logging.LogRecord) -> None:
        formatted_record = self.format(record)
        new_output = {
            'name': 'stdout',
            'output_type': 'stream',
            'text': formatted_record + '\n'
        }
        self.widget.outputs = (new_output, ) + self.widget.outputs

    def clear_logs(self):
        """ Clear the current logs """
        self.widget.clear_output()


def _make_output_widget():
    if not running_in_jupyter():
        raise RuntimeError('Cannot construct a logging widget because '
                           'Python is not running in a Jupyter notebook.')
    else:
        from ipywidgets import Output
        layout = {'width': '100%', 'height': '160px', 'border': '1px solid black'}
        return Output(layout=layout)


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
    display(get_log_widget())


def clear_logs() -> None:
    get_log_widget().clear_output()


def install_widget_handler(logger: logging.Logger) -> None:
    handler = WidgetHandler(level=logging.INFO, widget=_make_output_widget())
    handler.setFormatter(
        logging.Formatter('%(asctime)s  - [%(levelname)s] %(message)s'))
    logger.addHandler(handler)
