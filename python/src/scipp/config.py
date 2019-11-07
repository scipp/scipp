# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet


class _Plot:

    def __init__(self):

        # The plotting backend: possible choices are "interactive", "static",
        # "matplotlib", and "matplotlib:quiet"
        self.backend = "interactive"

        # The list of default line colors
        self.color_list = ["#1f77b4", "#ff7f0e", "#2ca02c", "#d62728",
                           "#9467bd", "#8c564b", "#e377c2", "#7f7f7f",
                           "#bcbd22", "#17becf"]

        # The colorbar properties
        self.cb = {"name": "viridis", "log": False, "min": None, "max": None,
                   "min_var": None, "max_var": None}

        # The default image height (in pixels)
        self.height = 600

        # The default image width (in pixels)
        self.width = 950

        # The size threshold above which an image is automatically rasterized
        self.rasterize_threshold = 100000


class _Colors:

    def __init__(self):

        # The colors for each dataset member used in table and show functions
        self.scheme = {
            "coord": "#dde9af",
            "data": "#ffe680",
            "labels": "#afdde9",
            "attr": "#ff8080",
            "inactive": "#cccccc",
            "masks": "#ffb3ff"
        }


plot = _Plot()
colors = _Colors()
