# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
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
            "log": False,
            "vmin": None,
            "vmax": None,
            "color": None,
            "show": True,
            "cbar": True,
            "norm": None,
        },
        # The default image height (in pixels)
        "height":
        533,
        # The default image width (in pixels)
        "width":
        800,
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
    },
    # The colors for each dataset member used in table and show functions
    "colors": {
        "coords": "#dde9af",
        "data": "#ffe680",
        "labels": "#afdde9",
        "attrs": "#ff8080",
        "masks": "#cccccc",
        "hover": "#d6eaf8",
    },
    "table_max_size": 50,
}

config_directory = appdirs.user_config_dir('scipp')
config_filename = os.path.join(config_directory, 'config.yaml')


def load():
    # Create the user configuration directory if it does not exist
    if not os.path.exists(config_directory):
        os.makedirs(config_directory)

    # Save a default configuration if no user coniguration file exists
    if not os.path.exists(config_filename):
        with open(config_filename, 'w+') as f:
            f.write(yaml.dump(defaults))

    # Load user configuration
    return config.config(
        ('yaml', config_filename, True),
        ('dict', defaults),
    )
