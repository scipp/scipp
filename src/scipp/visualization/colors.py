# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

import numpy as np

STYLE = {
    'attrs': '#ff5555',
    'coords': '#c6e590',
    'data': '#f6d028',
    'masks': '#c8c8c8',
}


def hex_to_rgb(hex_color):
    rgb_hex = [hex_color[x : x + 2] for x in [1, 3, 5]]
    return np.array([int(hex_value, 16) for hex_value in rgb_hex])


def rgb_to_hex(rgb):
    hex_value = []
    for i in rgb:
        h = hex(int(i))[2:]
        if len(h) < 2:
            h = f"{h}0"
        hex_value.append(h)
    return "#" + "".join(hex_value)


def make_random_color(fmt='rgb'):
    """
    Generate a random color.
    Possible output formats are:

      - 'rgb' (default): (255, 255, 100)
      - 'dec': (1.0, 1.0, 0.392)
      - 'rgba': (1.0, 1.0, 0.392, 1.0)
      - 'hex': #ffff64
    """
    rgb = np.random.randint(0, 255, 3)
    if fmt == 'rgb':
        return rgb
    elif fmt == 'dec':
        return rgb.astype(np.float) / 255.0
    elif fmt == 'rgba':
        return np.concatenate((rgb.astype(np.float) / 255.0, [1.0]))
    elif fmt == 'hex':
        return rgb_to_hex(rgb)
    else:
        raise RuntimeError(
            f"Unknown color format {fmt}. Possible choices are: "
            "rgb, dec, rgba, and hex."
        )
