from typing import Union, Sequence
import numpy as _np
import numpy as np

from .._scipp import core as _core_cpp
from .. import matrices, units, Unit, UnitError, Variable, vectors
from ..core.variable import _to_eigen_layout, matrix


def translation_from_vector(*,
                            unit: Union[_core_cpp.Unit,
                                        str] = _core_cpp.units.dimensionless,
                            value: Union[_np.ndarray, list]):
    """
    Creates a translation transformation from a single provided 3-vector.

    :param unit: The unit of the translation
    :param value: A list or numpy array of 3 items
    """
    return _core_cpp.translations(dims=[], unit=unit, values=value)


def translations_from_vectors(*,
                              dims: Sequence[str],
                              unit: Union[_core_cpp.Unit,
                                          str] = _core_cpp.units.dimensionless,
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
    return matrices(dims=[], values=_np.diag(value))


def scalings_from_vectors(*, dims: Sequence[str], values: Union[_np.ndarray, list]):
    """
    Creates scaling transformations from corresponding to the provided 3-vectors.

    :param dims: the dimensions of the variable
    :param values: a list or numpy array of 3-vectors, each corresponding to scaling
        coefficients in the x, y and z directions respectively.
    """
    identity = matrix(value=np.identity(3))
    matrices = identity.broadcast(dims=dims, shape=(len(values),)).copy()
    for field_name, index in (("xx", 0), ("yy", 1), ("zz", 2)):
        matrices.fields[field_name] = Variable(
            dims=dims, values=np.asarray(values)[:, index], dtype="float64")
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
    return _core_cpp.rotations(dims=dims, values=values)


def rotation_from_rotvec(*, value: Union[_np.ndarray, list],
                         unit: Union[_core_cpp.Unit, str]) -> Variable:
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
    return rotations_from_rotvecs(dims=[], values=value, unit=unit)


def rotations_from_rotvecs(*, dims: Sequence[str], values: Union[_np.ndarray, list],
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
    return rotations(dims=dims, values=r.as_quat())


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
