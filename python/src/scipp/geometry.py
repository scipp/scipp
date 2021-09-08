# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen

from typing import Union

import numpy as np

from ._scipp import core as _cpp
from .core._cpp_wrapper_util import call_func as _call_cpp_func


def position(x: _cpp.Variable, y: _cpp.Variable, z: _cpp.Variable) -> _cpp.Variable:
    """Element-wise zip functionality to produce a 3 element vector.

    :param x: Variable containing x components.
    :param y: Variable containing y components.
    :param z: Variable containing z components.
    :raises: If the dtypes of inputs are not double precision floats.
    :return: Zip of input x, y and z. Output unit is same as input unit.
    """
    return _call_cpp_func(_cpp.geometry.position, x, y, z)


def rotation_matrix_from_quaternion_coeffs(
        value: Union[list, tuple, np.ndarray]) -> _cpp.Variable:
    """Construct a rotation matrix from quaternions.

    :param value: Coefficients for the 4 quaternions in a one
                  dimensional array of 4 elements.
    :raises: If the input size is not 4.
    :return: 3x3 rotation matrix
    """
    return _call_cpp_func(_cpp.geometry.rotation_matrix_from_quaternion_coeffs, value)
