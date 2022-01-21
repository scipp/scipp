# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet, Jan-Lukas Wynen

from functools import lru_cache

import confuse


class Config:
    _TEMPLATE = {
        'colors': confuse.MappingValues(str),
        'plot': {
            'aspect': str,
            'color': confuse.Sequence(str),
            'dpi': int,
            'height': int,
            'linestyle': confuse.Sequence(confuse.Optional(str)),
            'linewidth': confuse.Sequence(int),
            'marker': confuse.Sequence(str),
            'padding': confuse.Sequence(confuse.Number()),
            'params': {
                'cbar': bool,
                'cmap': str,
                'color': confuse.Optional(str),
                'nan_color': str,
                'norm': str,
                'over_color': str,
                'show': bool,
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
        self._cfg.set_file('./scipp.config.yaml')

    @lru_cache
    def get(self) -> dict:
        return self._cfg.get(self._TEMPLATE)

    def __getitem__(self, item: str):
        return self.get()[item]


config = Config()
