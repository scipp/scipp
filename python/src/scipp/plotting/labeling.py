# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)

import enum

from matplotlib.dates import AutoDateLocator, AutoDateFormatter
import matplotlib.ticker as ticker

from .._scipp import core as sc
from .._utils import value_to_string


class Locator:
    """
    Variant type to identify and build a tick locator.
    """
    class Kind(enum.Enum):
        MAX_N = enum.auto()
        DATETIME = enum.auto()
        DEFAULT = enum.auto()

    def __init__(self, kind: Kind, **kwargs):
        self.kind = kind
        self._kwargs = kwargs

    def make(self, *args, **kwargs):
        if self.kind == self.Kind.MAX_N:
            return ticker.MaxNLocator(*args, **{**self._kwargs, **kwargs})
        if self.kind == self.Kind.DATETIME:
            return AutoDateLocator(*args, **{**self._kwargs, **kwargs})
        if self.kind == self.Kind.DEFAULT:
            return ticker.AutoLocator()


class Formatter:
    """
    Variant type to identify and build a tick formatter.
    """
    class Kind(enum.Enum):
        FUNC = enum.auto()
        DATETIME = enum.auto()
        DEFAULT = enum.auto()

    def __init__(self, kind: Kind, **kwargs):
        self.kind = kind
        self._kwargs = kwargs

    def make(self, locator=None, *args, **kwargs):
        if self.kind == self.Kind.FUNC:
            return ticker.FuncFormatter(*args, **{**self._kwargs, **kwargs})
        if self.kind == self.Kind.DATETIME:
            return AutoDateFormatter(locator, *args, **{
                **self._kwargs,
                **kwargs
            })
        if self.kind == self.Kind.DEFAULT:
            return ticker.ScalarFormatter(*args, **{**self._kwargs, **kwargs})


def vector_tick_formatter(array_values, size):
    """
    Format vector output for ticks: return 3 components as a string.
    """
    return lambda val, pos: "(" + ",".join([
        value_to_string(item, precision=2) for item in array_values[int(val)]
    ]) + ")" if (0 <= int(val) < size) else ""


def string_tick_formatter(array_values, size):
    """
    Format string ticks: find closest string in coordinate array.
    """
    return lambda val, pos: array_values[int(val)] if (0 <= int(val) < size
                                                       ) else ""


def labeling_factory(dim, data_array, shape):
    def make_default():
        # If coordinate contains float / int
        return Formatter(Formatter.Kind.DEFAULT), Locator(Locator.Kind.DEFAULT)

    if dim not in data_array.meta:
        formatter, locator = make_default()
    else:
        meta = data_array.meta[dim]
        if meta.dtype == sc.dtype.vector_3_float64:
            # If coordinate contains vectors
            formatter = Formatter(Formatter.Kind.FUNC,
                                  func=vector_tick_formatter(
                                      meta.values, shape))
            locator = Locator(Locator.Kind.MAX_N, integer=True)
        elif meta.dtype == sc.dtype.string:
            # If coordinate contains strings
            formatter = Formatter(Formatter.Kind.FUNC,
                                  func=string_tick_formatter(
                                      meta.values, shape))
            locator = Locator(Locator.Kind.MAX_N, integer=True)
        elif meta.dtype == sc.dtype.datetime64:
            # If coordinate contains datetimes
            formatter = Formatter(Formatter.Kind.DATETIME)
            locator = Locator(Locator.Kind.DATETIME)
        else:
            formatter, locator = make_default()

    return {'linear': formatter, 'log': formatter, 'locator': locator}
