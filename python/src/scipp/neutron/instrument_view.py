# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

import numpy as np


def instrument_view(scipp_obj=None,
                    positions="position",
                    pixel_size=None,
                    **kwargs):
    """
    Plot a 3D view of the instrument, using the `position` coordinate as the
    detector vector positions.
    Use the `positions` argument to change the vectors used as pixel positions.
    Sliders are added to navigate extra dimensions.
    Spatial slicing and pixel opacity control is available using the controls
    below the scene.
    Use the `pixel_size` argument to specify the size of the detectors.
    If no `pixel_size` is given, a guess is performed based on the distance
    between the positions of the first two pixel positions.
    The aspect ratio of the positions is preserved by default, but this can
    be changed to automatic scaling using `aspect="equal"`.
    """

    from .. import plot

    if pixel_size is None:
        pos_array = scipp_obj.meta[positions].values
        if len(pos_array) > 1:
            pixel_size = np.linalg.norm(pos_array[1] - pos_array[0])

    return plot(scipp_obj,
                projection="3d",
                positions=positions,
                pixel_size=pixel_size,
                **kwargs)
