# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet, Jan-Lukas Wynen
"""
Runtime configuration utility.

See https://scipp.github.io/reference/runtime-configuration.html
"""

from functools import lru_cache
from typing import Any, Iterable, Tuple

import confuse


class Config:
    """
    Runtime configuration parameters.

    Provides dict-like access to configuration parameters.
    Modifications apply to the current process only and do not
    modify any configuration files.

    See https://scipp.github.io/reference/runtime-configuration.html
    """

    _TEMPLATE = {
        'colors': confuse.MappingValues(str),
        'plot': {
            'aspect': confuse.OneOf((confuse.Number(), str)),
            'color': confuse.Sequence(str),
            'dpi': int,
            'height': int,
            'linestyle': confuse.Sequence(confuse.Optional(str)),
            'linewidth': confuse.Sequence(int),
            'marker': confuse.Sequence(str),
            'bounding_box': confuse.Sequence(confuse.Number()),
            'params': {
                'cbar': bool,
                'cmap': str,
                'color': confuse.Optional(str),
                'nan_color': str,
                'norm': confuse.Choice(('linear', 'log')),
                'over_color': str,
                'under_color': str,
                'vmax': confuse.Optional(confuse.Number()),
                'vmin': confuse.Optional(confuse.Number()),
            },
            'pixel_ratio': confuse.Number(),
            'width': confuse.Number(),
        },
        'table_max_size': int,
    }

    def __init__(self):
        self._cfg = confuse.LazyConfig('scipp', __name__)
        self._cfg.set(
            confuse.YamlSource('./scipp.config.yaml',
                               optional=True,
                               loader=self._cfg.loader))

    @lru_cache
    def get(self) -> dict:
        """Return parameters as a dict."""
        return self._cfg.get(self._TEMPLATE)

    def __getitem__(self, name: str):
        """Return parameter of given name."""
        return self.get()[name]

    def __setitem__(self, name: str, value):
        """Change the value of a parameter."""
        if name not in self.get():
            raise TypeError(
                f"New items cannot be inserted into the configuration, got '{name}'.")
        self.get()[name] = value

    def keys(self) -> Iterable[str]:
        """Returns iterable over parameter names."""
        yield from self.get().keys()

    def values(self) -> Iterable[Any]:
        """Returns iterable over parameter values."""
        yield from self.get().values()

    def items(self) -> Iterable[Tuple[str, Any]]:
        """Returns iterable over parameter names and values."""
        yield from self.get().items()


config = Config()
