# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .. import scalar, Variable
from .. import abs as abs_

import numpy as np
from typing import Tuple


def find_limits(x: Variable, scale: str = "linear") -> Tuple[Variable, ...]:
    """
    Find sensible limits, depending on linear or log scale.
    """
    v = x.values
    finite_vals = v[np.isfinite(v)]
    if scale == "log":
        finite_min = np.amin(finite_vals, initial=np.inf, where=finite_vals > 0)
    else:
        finite_min = np.amin(finite_vals)
    finite_max = np.amax(finite_vals)

    return (scalar(finite_min, unit=x.unit,
                   dtype='float64'), scalar(finite_max, unit=x.unit, dtype='float64'))


def fix_empty_range(lims: Tuple[Variable, ...],
                    replacement: Variable = None) -> Tuple[Variable, ...]:
    """
    Range correction in case xmin == xmax
    """
    dx = scalar(0.0, unit=lims[0].unit)
    if lims[0].value == lims[1].value:
        if replacement is not None:
            dx = 0.5 * replacement
        elif lims[0].value == 0.0:
            dx = scalar(0.5, unit=lims[0].unit)
        else:
            dx = 0.5 * abs_(lims[0])
    return [lims[0] - dx, lims[1] + dx]
