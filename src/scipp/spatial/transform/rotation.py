from typing import Union, Sequence
import numpy as _np

from ... import units, Unit, UnitError, Variable, vectors
from ..._scipp import core as _core_cpp


def from_quat(*, value: Union[_np.ndarray, list]):
    """
    Creates a rotation-type variable from the provided quaternion coefficients.

    The quaternion coefficients are provided in scalar-last order (x, y, z, w), where
    x, y, z and w form the quaternion w + xi + yj + zk.

    :param value: a numpy array or list with length 4, corresponding to the quaternion
        coefficients (x*i, y*j, z*k, w)
    """
    return _core_cpp.rotations(dims=[], values=value)


def from_quats(*, dims: Sequence[str], values: Union[_np.ndarray, list]):
    """
    Creates a rotation-type variable from the provided quaternion coefficients.

    The quaternion coefficients are provided in scalar-last order (x, y, z, w), where
    x, y, z and w form the quaternion w + xi + yj + zk.

    :param values: a numpy array of numpy arrays corresponding to the quaternion
        coefficients (w, x*i, y*j, z*k)
    """
    return _core_cpp.rotations(dims=dims, values=values)


def from_matrix(*, value: Union[_np.ndarray, list]):
    """
    Create a single rotation transformation from a rotation matrix.

    This requires ``scipy`` to be installed, as is wraps ``Rotation.from_matrix()``
    from ``scipy.spatial.transform``.

    :param value: a numpy 3x3 matrix, which will be interpreted as a rotation matrix.
    """
    from scipy.spatial.transform import Rotation as R
    r = R.from_matrix(value)
    return _core_cpp.rotations(dims=[], values=r.as_quat())


def from_matrices(*, dims: Sequence[str], values: Union[_np.ndarray, list]):
    """
    Create rotation transformations from rotation matrices.

    This requires ``scipy`` to be installed, as is wraps ``Rotation.from_matrix()``
    from ``scipy.spatial.transform``.

    :param dims: The dimensions of the created variable
    :param values: a numpy array of numpy 3x3 matrices, which will be interpreted as a
        rotation matrix.
    """
    from scipy.spatial.transform import Rotation as R
    r = R.from_matrix(values)
    return _core_cpp.rotations(dims=dims, values=r.as_quat())


def from_rotvec(*, value: Union[_np.ndarray, list], unit: Union[_core_cpp.Unit,
                                                                str]) -> Variable:
    """
    Creates a rotation transformation from a rotation vector.

    This requires ``scipy`` to be installed, as is wraps ``Rotation.from_rotvec()``
    from ``scipy.spatial.transform``.

    A rotation vector is a 3 dimensional vector which is co-directional to the axis of
    rotation and whose norm gives the angle of rotation.

    :param value: A numpy vector of 3 values
    :param unit: The units of the 3 provided values. Should be either sc.units.rad or
        sc.units.deg.
    """
    return from_rotvecs(dims=[], values=value, unit=unit)


def from_rotvecs(*, dims: Sequence[str], values: Union[_np.ndarray, list],
                 unit: Union[_core_cpp.Unit, str]) -> Variable:
    """
    Creates rotation transformations from rotation vectors.

    This requires ``scipy`` to be installed, as is wraps ``Rotation.from_rotvec()``
    from ``scipy.spatial.transform``.

    A rotation vector is a 3 dimensional vector which is co-directional to the axis of
    rotation and whose norm gives the angle of rotation.

    :param dims: The dimensions of the created variable
    :param values: A numpy vector of 3 values
    :param unit: The units of the 3 provided values. Should be either sc.units.rad or
        sc.units.deg.
    """
    from scipy.spatial.transform import Rotation as R
    supported = [units.deg, units.rad]
    if unit not in supported:
        raise UnitError(f"Rotation vector unit must be one of {supported}.")
    r = R.from_rotvec(values, degrees=unit == units.deg)
    return from_quats(dims=dims, values=r.as_quat())


def as_rotvec(rotation: Variable, *, unit='rad') -> Variable:
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
