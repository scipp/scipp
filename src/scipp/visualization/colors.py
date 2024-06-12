# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

import numpy as np
import numpy.typing as npt

STYLE = {
    'attrs': '#ff5555',
    'coords': '#c6e590',
    'data': '#f6d028',
    'masks': '#c8c8c8',
}


def hex_to_rgb(hex_color: str) -> npt.NDArray[np.int64]:
    rgb_hex = [hex_color[x : x + 2] for x in [1, 3, 5]]
    return np.array([int(hex_value, 16) for hex_value in rgb_hex])


def rgb_to_hex(rgb: npt.NDArray[np.int64]) -> str:
    hex_value = []
    for i in rgb:
        h = hex(int(i))[2:]
        if len(h) < 2:
            h = f"{h}0"
        hex_value.append(h)
    return "#" + "".join(hex_value)
