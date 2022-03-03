# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet, Jan-Lukas Wynen
"""
Runtime configuration.

See https://scipp.github.io/reference/runtime-configuration.html
"""

# *** For developers ***
#
# When adding new options, update both the file config_default.yaml
# and Config._TEMPLATE.
# If the template is not updated, new options will simply be ignored.

from functools import lru_cache
from pathlib import Path
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

    def config_dir(self) -> Path:
        """
        Return the directory for configuration files.

        The directory is created if it does not already exist.
        """
        return Path(self._cfg.config_dir())

    def config_path(self) -> Path:
        """
        Return the path to the configuration file.

        The file may not exist but its folder is created if it does not already exist.
        """
        return Path(self._cfg.user_config_path())

    @lru_cache()
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
