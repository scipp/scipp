# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
from typing import Union, Sequence
import numpy as _np
import numpy as np

from .._scipp import core as _core_cpp
from .. import units, Unit, UnitError, Variable, vectors


def _to_eigen_layout(a):
    # Numpy and scipp use row-major, but Eigen matrices use column-major,
    # transpose matrix axes for copying values.
    return _np.moveaxis(a, -1, -2)


def translation(*,
                unit: Union[_core_cpp.Unit, str] = _core_cpp.units.dimensionless,
                value: Union[_np.ndarray, list]):
    """
    Creates a translation transformation from a single provided 3-vector.

    :param unit: The unit of the translation
    :param value: A list or numpy array of 3 items
    """
    return _core_cpp.translations(dims=[], unit=unit, values=value)


def translations(*,
                 dims: Sequence[str],
                 unit: Union[_core_cpp.Unit, str] = _core_cpp.units.dimensionless,
                 values: Union[_np.ndarray, list]):
    """
    Creates translation transformations from multiple 3-vectors.

    :param dims: The dimensions of the created variable
    :param unit: The unit of the translation
    :param value: A list or numpy array of 3-vectors
    """
    return _core_cpp.translations(dims=dims, unit=unit, values=values)


def scaling_from_vector(*, value: Union[_np.ndarray, list]):
    """
    Creates a scaling transformation from a provided 3-vector.

    :param value: a list or numpy array of 3 values, corresponding to scaling
        coefficients in the x, y and z directions respectively.
    """
    return linear_transforms(dims=[], values=_np.diag(value))


def scalings_from_vectors(*, dims: Sequence[str], values: Union[_np.ndarray, list]):
    """
    Creates scaling transformations from corresponding to the provided 3-vectors.

    :param dims: the dimensions of the variable
    :param values: a list or numpy array of 3-vectors, each corresponding to scaling
        coefficients in the x, y and z directions respectively.
    """
    identity = linear_transform(value=np.identity(3))
    matrices = identity.broadcast(dims=dims, shape=(len(values), )).copy()
    for field_name, index in (("xx", 0), ("yy", 1), ("zz", 2)):
        matrices.fields[field_name] = Variable(dims=dims,
                                               values=np.asarray(values)[:, index],
                                               dtype="float64")
    return matrices


def rotation(*, value: Union[_np.ndarray, list]):
    """
    Creates a rotation-type variable from the provided quaternion coefficients.

    The quaternion coefficients are provided in scalar-last order (x, y, z, w), where
    x, y, z and w form the quaternion w + xi + yj + zk.

    :param value: a numpy array or list with length 4, corresponding to the quaternion
        coefficients (x*i, y*j, z*k, w)
    """
    return _core_cpp.rotations(dims=[], values=value)


def rotations(*, dims: Sequence[str], values: Union[_np.ndarray, list]):
    """
    Creates a rotation-type variable from the provided quaternion coefficients.

    The quaternion coefficients are provided in scalar-last order (x, y, z, w), where
    x, y, z and w form the quaternion w + xi + yj + zk.

    :param values: a numpy array of numpy arrays corresponding to the quaternion
        coefficients (w, x*i, y*j, z*k)
    """
    for val in values:
        if hasattr(val, "__len__") and len(val) != 4:
            raise ValueError("Inputs must be Quaternions to create a rotation. If you"
                             "want to pass a rotation matrix, use "
                             "sc.linear_transforms instead.")
    return _core_cpp.rotations(dims=dims, values=values)


def rotations_from_rotvecs(rotation_vectors: Variable) -> Variable:
    """
    Creates rotation transformations from rotation vectors.

    This requires ``scipy`` to be installed, as is wraps ``Rotation.from_rotvec()``
    from ``scipy.spatial.transform``.

    A rotation vector is a 3 dimensional vector which is co-directional to the axis of
    rotation and whose norm gives the angle of rotation.

    :param rotation_vectors: A Variable with vector dtype
    """
    from scipy.spatial.transform import Rotation as R
    supported = [units.deg, units.rad]
    unit = rotation_vectors.unit
    if unit not in supported:
        raise UnitError(f"Rotation vector unit must be one of {supported}.")
    r = R.from_rotvec(rotation_vectors.values, degrees=unit == units.deg)
    return rotations(dims=rotation_vectors.dims, values=r.as_quat())


def rotation_as_rotvec(rotation: Variable, *, unit='rad') -> Variable:
    """
    Represent a rotation matrix (or matrices) as rotation vector(s).

    This requires ``scipy`` to be installed, as is wraps ``Rotation.as_rotvec()``
    from ``scipy.spatial.transform``.

    A rotation vector is a 3 dimensional vector which is co-directional to the axis of
    rotation and whose norm gives the angle of rotation.

    :param rotation: A variable with rotation matrices.
    :returns: Variable with rotation vectors.
    :rtype: Variable
    """
    unit = Unit(unit)
    supported = [units.deg, units.rad]
    if unit not in supported:
        raise UnitError(f"Rotation vector unit must be one of {supported}.")
    from scipy.spatial.transform import Rotation as R
    r = R.from_matrix(rotation.values)
    if rotation.unit not in [units.one, units.dimensionless]:
        raise UnitError(f"Rotation matrix must be dimensionless, got {rotation.unit}.")
    return vectors(dims=rotation.dims,
                   values=r.as_rotvec(degrees=unit == units.deg),
                   unit=unit)


def affine_transform(*,
                     unit: Union[_core_cpp.Unit, str] = _core_cpp.units.dimensionless,
                     value: Union[_np.ndarray, list]):
    """
    Initializes a single affine transformation from the provided affine matrix
    coefficients.

    :param unit: The unit of the affine transformation's translation component.
    :param value: A 4x4 matrix of affine coefficients.
    """
    return affine_transforms(dims=[], unit=unit, values=value)


def affine_transforms(*,
                      dims: Sequence[str],
                      unit: Union[_core_cpp.Unit, str] = _core_cpp.units.dimensionless,
                      values: Union[_np.ndarray, list]):
    """
    Initializes affine transformations from the provided affine matrix
    coefficients.

    :param unit: The unit of the affine transformation's translation component.
    :param value: An array of 4x4 matrices of affine coefficients.
    """
    return _core_cpp.affine_transforms(dims=dims,
                                       unit=unit,
                                       values=_to_eigen_layout(values))


def linear_transform(*,
                     unit: Union[_core_cpp.Unit, str] = _core_cpp.units.dimensionless,
                     value: Union[_np.ndarray, list]):
    """Constructs a zero dimensional :class:`Variable` holding a single 3x3
    matrix.

    :seealso: :py:func:`scipp.matrices`

    :param value: Initial value, a list or 1-D numpy array.
    :param unit: Optional, unit. Default=dimensionless
    :returns: A scalar (zero-dimensional) Variable.
    :rtype: Variable
    """
    return _core_cpp.matrices(dims=[], unit=unit, values=_to_eigen_layout(value))


def linear_transforms(*,
                      dims: Sequence[str],
                      unit: Union[_core_cpp.Unit, str] = _core_cpp.units.dimensionless,
                      values: Union[_np.ndarray, list]):
    """Constructs a :class:`Variable` with given dimensions holding an array
    of 3x3 matrices.

    :seealso: :py:func:`scipp.matrix`

    :param dims: Dimension labels.
    :param values: Initial values.
    :param unit: Optional, data unit. Default=dimensionless
    """
    return _core_cpp.matrices(dims=dims, unit=unit, values=_to_eigen_layout(values))


__all__ = [
    'rotation', 'rotations', 'rotations_from_rotvecs', 'rotation_as_rotvec',
    'scaling_from_vector', 'scalings_from_vectors', 'translation', 'translations',
    'affine_transform', 'affine_transforms', 'linear_transform', 'linear_transforms'
]
