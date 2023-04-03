# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen

import warnings
from typing import Union

import numpy as np

from . import spatial
from ._scipp import core as _cpp
from .core import Variable
from .core._cpp_wrapper_util import call_func as _call_cpp_func
from .core.util import VisibleDeprecationWarning


def position(x: Variable, y: Variable, z: Variable) -> Variable:
    """Element-wise zip functionality to produce a 3 element vector.

    :param x: Variable containing x components.
    :param y: Variable containing y components.
    :param z: Variable containing z components.
    :raises: If the dtypes of inputs are not double precision floats.
    :return: Zip of input x, y and z. Output unit is same as input unit.

    .. deprecated:: 23.03.1
       Has been moved to :func:`scipp.spatial.as_vectors`.
    """
    warnings.warn(
        "sc.geometry.position has been deprecated "
        "and will be removed in Scipp v23.09.0. "
        "Use scipp.spatial.as_vectors instead.",
        VisibleDeprecationWarning,
    )
    return spatial.as_vectors(x, y, z)


def rotation_matrix_from_quaternion_coeffs(
    value: Union[list, tuple, np.ndarray]
) -> Variable:
    """Construct a rotation matrix from quaternions.

    :param value: Coefficients for the 4 quaternions in a one
                  dimensional array of 4 elements.
    :raises: If the input size is not 4.
    :return: 3x3 rotation matrix

    .. deprecated:: 23.03.1
       Will be removed in favor of :func:`scipp.spatial.rotations`.
    """
    warnings.warn(
        "sc.geometry.rotation_matrix_from_quaternion_coeffs has been deprecated and "
        "will be removed in Scipp v23.09.0. "
        "Use scipp.spatial.rotations to construct a rotation transform.",
        VisibleDeprecationWarning,
    )
    return _call_cpp_func(_cpp.geometry.rotation_matrix_from_quaternion_coeffs, value)
