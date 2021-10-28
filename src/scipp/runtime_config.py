# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

import appdirs
import config
import os
import yaml

defaults = {
    "plot": {
        # The list of default line colors
        "color": [
            "#1f77b4",
            "#ff7f0e",
            "#2ca02c",
            "#d62728",
            "#9467bd",
            "#8c564b",
            "#e377c2",
            "#7f7f7f",
            "#bcbd22",
            "#17becf",
        ],
        # The colorbar properties
        "params": {
            "cmap": "viridis",
            "norm": "linear",
            "vmin": None,
            "vmax": None,
            "color": None,
            "show": True,
            "cbar": True,
            "nan_color": "#d3d3d3",
            "under_color": "#9467bd",
            "over_color": "#8c564b",
        },
        # The default image height (in pixels)
        "height":
        400,
        # The default image width (in pixels)
        "width":
        600,
        # Resolution
        "dpi":
        96,
        # Aspect ratio for images: "equal" will conserve the aspect ratio,
        # while "auto" will stretch the image to the size of the figure
        "aspect":
        "auto",
        # Make list of markers for matplotlib
        "marker": [
            "o",
            "^",
            "s",
            "d",
            "*",
            "1",
            "P",
            "h",
            "X",
            "v",
            "<",
            ">",
            "2",
            "3",
            "4",
            "8",
            "p",
            "H",
            "+",
            "x",
            "D",
        ],
        # Default line width for 1D plots
        "linewidth": [1.5],
        # Default line style for 1D non-histogram plots
        "linestyle": ["none"],
        # Default padding around matplotlib axes
        "padding": [0.05, 0.02, 1, 1],
        # Default monitor pixel ratio for 3d plots.
        # Set to > 1 for high density (e.g. retina) displays.
        "pixel_ratio":
        1.0,
    },
    # The colors for each dataset member used in table and show functions
    "colors": {
        "attrs": "#ff5555",
        "coords": "#c6e590",
        "data": "#f6d028",
        "hover": "#d6eaf8",
        "masks": "#c8c8c8",
        "header_text": "#111111",
        "button": "#bdbdbd49",
        "button_selected": "#bdbdbdbb",
    },
    "table_max_size": 50,
}

config_directory = appdirs.user_config_dir('scipp')
config_filename = os.path.join(config_directory, 'config.yaml')


def load():
    # Create the user configuration directory if it does not exist
    if not os.path.exists(config_directory):
        os.makedirs(config_directory)

    # Save a default configuration if no user configuration file exists
    if not os.path.exists(config_filename):
        with open(config_filename, 'w+') as f:
            f.write(yaml.dump(defaults))

    # Load user configuration
    return config.config(
        ('yaml', config_filename, True),
        ('dict', defaults),
    )
