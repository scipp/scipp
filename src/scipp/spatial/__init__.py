# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
"""Transformations of vectors.

Functions in this module can be used to construct, deconstruct, and modify
variables with transformations on vectors.

Despite the name, transformations in this module can be applied to 3-vectors
in any vector space and coordinate system, not just the physical space.
The user has to ensure that transformations are applied to the correct vectors.

See Also
--------
scipp.vector:
    Construct a scalar variable holding a 3-vector.
scipp.vectors:
    Construct an array variable holding 3-vectors.
"""

from collections.abc import Sequence
from typing import Any, TypeVar

import numpy as np
import numpy.typing as npt

from .. import units
from .._scipp import core as _core_cpp
from ..core import DType, Unit, UnitError, Variable, vectors
from ..core._cpp_wrapper_util import call_func as _call_cpp_func

# Element type for NumPy arrays.
# For sequences in multi-dim functions,
# we use `Sequence[Any]` as a simple but incomplete solution for nested lists.
_Float = TypeVar('_Float', bound=np.float64 | np.float32, covariant=True)

_Precision = TypeVar('_Precision', bound=npt.NBitBase)


def _to_eigen_layout(a: npt.NDArray[_Float] | Sequence[Any]) -> npt.NDArray[_Float]:
    # Numpy and scipp use row-major, but Eigen matrices use column-major,
    # transpose matrix axes for copying values.
    return np.moveaxis(a, -1, -2)  # type: ignore[arg-type]


def as_vectors(x: Variable, y: Variable, z: Variable) -> Variable:
    """Return inputs combined into vectors.

    Inputs may be broadcast to a common shape,.

    Parameters
    ----------
    x:
        Variable containing x components.
    y:
        Variable containing y components.
    z:
        Variable containing z components.

    Returns
    -------
    :
        Zip of input x, y and z with dtype ``vector3``.
        The output unit is the same as input unit.

    Raises
    ------
    scipp.DTypeError
        If the dtypes of inputs are not ``float64``.

    See also
    --------
    scipp.vector:
        Construct a vector from plain numbers.
    scipp.vectors:
        Construct vectors from plain numpy arrays or lists.

    .. versionadded:: 23.03.1
    """
    return _call_cpp_func(  # type: ignore[return-value]
        _core_cpp.geometry.as_vectors,
        *(c.to(dtype='float64', copy=False) for c in (x, y, z)),
    )


def translation(
    *,
    unit: Unit | str = units.dimensionless,
    value: npt.NDArray[_Float] | Sequence[_Float],
) -> Variable:
    """
    Creates a translation transformation from a single provided 3-vector.

    Parameters
    ----------
    unit:
        The unit of the translation
    value:
        A list or NumPy array of 3 items

    Returns
    -------
    :
        A scalar variable of dtype ``translation3``.

    See Also
    --------
    scipp.spatial.translations:
        Create multiple translation transformations.

    Examples
    --------
    Create a translation by 1 meter in x, 2 meters in y, and 3 meters in z:

      >>> import scipp as sc
      >>> trans = sc.spatial.translation(value=[1, 2, 3], unit='m')
      >>> trans
      <scipp.Variable> ()  translation3              [m]  (1, 2, 3)

    Apply the translation to a vector:

      >>> vec = sc.vector(value=[10, 20, 30], unit='m')
      >>> trans * vec
      <scipp.Variable> ()    vector3              [m]  (11, 22, 33)
    """
    return translations(dims=(), unit=unit, values=value)


def translations(
    *,
    dims: Sequence[str],
    unit: Unit | str = units.dimensionless,
    values: npt.NDArray[_Float] | Sequence[Any],
) -> Variable:
    """
    Creates translation transformations from multiple 3-vectors.

    Parameters
    ----------
    dims:
        The dimensions of the created variable
    unit:
        The unit of the translation
    values:
        A list or NumPy array of 3-vectors

    Returns
    -------
    :
        An array variable of dtype ``translation3``.

    See Also
    --------
    scipp.spatial.translation:
        Create a single translation transformation.

    Examples
    --------
    Create multiple translations along different axes:

      >>> import scipp as sc
      >>> trans = sc.spatial.translations(
      ...     dims=['x'],
      ...     values=[[1, 0, 0], [0, 1, 0], [0, 0, 1]],
      ...     unit='m'
      ... )
      >>> trans
      <scipp.Variable> (x: 3)  translation3              [m]  [(1, 0, 0), (0, 1, 0), (0, 0, 1)]
    """  # noqa: E501
    return Variable(dims=dims, unit=unit, values=values, dtype=DType.translation3)


def scaling_from_vector(*, value: npt.NDArray[_Float] | Sequence[_Float]) -> Variable:
    """
    Creates a scaling transformation from a provided 3-vector.

    Parameters
    ----------
    value:
        A list or NumPy array of 3 values, corresponding to scaling
        coefficients in the x, y and z directions respectively.

    Returns
    -------
    :
       A scalar variable of dtype ``linear_transform3``.

    See Also
    --------
    scipp.spatial.scalings_from_vectors:
        Create multiple scaling transformations.

    Examples
    --------
    Create a scaling transformation:

      >>> import scipp as sc
      >>> scale = sc.spatial.scaling_from_vector(value=[2, 3, 4])
      >>> scale
      <scipp.Variable> ()  linear_transform3  [dimensionless]  ((2, 0, 0), , (0, 3, 0), , (0, 0, 4), )

    Apply the scaling to a vector:

      >>> vec = sc.vector(value=[1, 1, 1], unit='m')
      >>> scale * vec
      <scipp.Variable> ()    vector3              [m]  (2, 3, 4)
    """  # noqa: E501
    return linear_transforms(dims=[], values=np.diag(value))


def scalings_from_vectors(
    *, dims: Sequence[str], values: npt.NDArray[_Float] | Sequence[Any]
) -> Variable:
    """
    Creates scaling transformations from corresponding to the provided 3-vectors.

    Parameters
    ----------
    dims:
        The dimensions of the variable.
    values:
        A list or NumPy array of 3-vectors, each corresponding to scaling
        coefficients in the x, y and z directions respectively.

    Returns
    -------
    :
        An array variable of dtype ``linear_transform3``.

    See Also
    --------
    scipp.spatial.scaling_from_vector:
        Create a single scaling transformation.

    Examples
    --------
    Create multiple scaling transformations:

      >>> import scipp as sc
      >>> scales = sc.spatial.scalings_from_vectors(
      ...     dims=['scale'],
      ...     values=[[1, 1, 1], [2, 2, 2], [0.5, 0.5, 0.5]]
      ... )
      >>> scales
      <scipp.Variable> (scale: 3)  linear_transform3  [dimensionless]  [(...), (...), (...)]
    """  # noqa: E501
    identity = linear_transform(value=np.identity(3))
    matrices = identity.broadcast(
        dims=dims,
        shape=(len(values),),
    ).copy()
    for field_name, index in (("xx", 0), ("yy", 1), ("zz", 2)):
        matrices.fields[field_name] = Variable(
            dims=dims, values=np.asarray(values)[:, index], dtype="float64"
        )
    return matrices


def rotation(*, value: npt.NDArray[_Float] | Sequence[_Float]) -> Variable:
    """
    Creates a rotation-type variable from the provided quaternion coefficients.

    The quaternion coefficients are provided in scalar-last order (x, y, z, w), where
    x, y, z and w form the quaternion

    .. math::

        q = w + xi + yj + zk.

    Attention
    ---------
    The quaternion must be normalized in order to represent a rotation.
    You can use, e.g.::

        q = np.array([1, 2, 3, 4])
        rot = sc.spatial.rotation(value=q / np.linalg.norm(q))

    Parameters
    ----------
    value:
        A NumPy array or list with length 4, corresponding to the quaternion
        coefficients (x*i, y*j, z*k, w)

    Returns
    -------
    :
        A scalar variable of dtype ``rotation3``.

    See Also
    --------
    scipp.spatial.rotations:
        Create multiple rotation transformations.
    scipp.spatial.rotations_from_rotvecs:
        Create rotations from rotation vectors.

    Examples
    --------
    Create an identity rotation (no rotation):

      >>> import scipp as sc
      >>> rot = sc.spatial.rotation(value=[0, 0, 0, 1])
      >>> rot
      <scipp.Variable> ()  rotation3  [dimensionless]  (1+0i+0j+0k)

    Create a 180 degree rotation around the z-axis and apply it to a vector:

      >>> rot_180z = sc.spatial.rotation(value=[0, 0, 1, 0])
      >>> vec_x = sc.vector(value=[1, 0, 0])
      >>> rot_180z * vec_x
      <scipp.Variable> ()    vector3  [dimensionless]  (-1, 0, 0)
    """
    return rotations(dims=(), values=value)


def rotations(
    *, dims: Sequence[str], values: npt.NDArray[_Float] | Sequence[Any]
) -> Variable:
    """
    Creates a rotation-type variable from the provided quaternion coefficients.

    The quaternion coefficients are provided in scalar-last order (x, y, z, w), where
    x, y, z and w form the quaternion

    .. math::

        q = w + xi + yj + zk.

    Attention
    ---------
    The quaternions must be normalized in order to represent a rotation.
    You can use, e.g.::

        q = np.array([[1, 2, 3, 4], [-1, -2, -3, -4]])
        rot = sc.spatial.rotations(
            dims=['x'],
            values=q / np.linalg.norm(q, axis=1)[:, np.newaxis])

    Parameters
    ----------
    dims:
        The dimensions of the variable.
    values:
        A NumPy array of NumPy arrays corresponding to the quaternion
        coefficients (w, x*i, y*j, z*k)

    Returns
    -------
    :
        An array variable of dtype ``rotation3``.

    See Also
    --------
    scipp.spatial.rotation:
        Create a single rotation transformation.
    scipp.spatial.rotations_from_rotvecs:
        Create rotations from rotation vectors.

    Examples
    --------
    Create multiple rotations from quaternions (identity and 180 deg around z):

      >>> import numpy as np
      >>> import scipp as sc
      >>> q_array = np.array([[0, 0, 0, 1], [0, 0, 1, 0]])
      >>> rots = sc.spatial.rotations(dims=['rot'], values=q_array)
      >>> rots  # doctest: +NORMALIZE_WHITESPACE
      <scipp.Variable> (rot: 2)  rotation3  [dimensionless]
      [(1+0i+0j+0k), (0+0i+0j+1k)]
    """
    values = np.asarray(values)
    if values.shape[-1] != 4:
        raise ValueError(
            "Inputs must be Quaternions to create a rotation, i.e., have "
            "4 components. If you want to pass a rotation matrix, use "
            "sc.linear_transforms instead."
        )
    return Variable(dims=dims, values=values, dtype=DType.rotation3)


def rotations_from_rotvecs(rotation_vectors: Variable) -> Variable:
    """
    Creates rotation transformations from rotation vectors.

    This requires ``scipy`` to be installed, as is wraps
    :meth:`scipy.spatial.transform.Rotation.from_rotvec`.

    A rotation vector is a 3 dimensional vector which is co-directional to the axis of
    rotation and whose norm gives the angle of rotation.

    Parameters
    ----------
    rotation_vectors:
        A Variable with vector dtype

    Returns
    -------
    :
        An array variable of dtype ``rotation3``.

    See Also
    --------
    scipp.spatial.rotation:
        Create a single rotation from quaternion coefficients.
    scipp.spatial.rotations:
        Create multiple rotations from quaternion coefficients.

    Examples
    --------
    Create rotations from rotation vectors (90 degrees around z and x axes):

      >>> import numpy as np
      >>> import scipp as sc
      >>> rotvecs = sc.vectors(
      ...     dims=['rot'],
      ...     values=[[0, 0, np.pi/2], [np.pi/2, 0, 0]],
      ...     unit='rad'
      ... )
      >>> rots = sc.spatial.rotations_from_rotvecs(rotvecs)
      >>> rots  # doctest: +NORMALIZE_WHITESPACE
      <scipp.Variable> (rot: 2)  rotation3  [dimensionless]
      [(0.707107+0i+0j+0.707107k), (0.707107+0.707107i+0j+0k)]
    """
    from scipy.spatial.transform import Rotation as R

    supported = [units.deg, units.rad]
    unit = rotation_vectors.unit
    if unit not in supported:
        raise UnitError(f"Rotation vector unit must be one of {supported}.")
    r = R.from_rotvec(rotation_vectors.values, degrees=unit == units.deg)
    return rotations(dims=rotation_vectors.dims, values=r.as_quat())


def rotation_as_rotvec(rotation: Variable, *, unit: Unit | str = 'rad') -> Variable:
    """
    Represent a rotation matrix (or matrices) as rotation vector(s).

    This requires ``scipy`` to be installed, as is wraps
    :meth:`scipy.spatial.transform.Rotation.as_rotvec`.

    A rotation vector is a 3 dimensional vector which is co-directional to the axis of
    rotation and whose norm gives the angle of rotation.

    Parameters
    ----------
    rotation:
        A variable with rotation matrices.
    unit:
        Angle unit for the rotation vectors.

    Returns
    -------
    :
        An array variable with rotation vectors of dtype ``vector3``.
    """
    unit = Unit(unit) if not isinstance(unit, Unit) else unit
    supported = [units.deg, units.rad]
    if unit not in supported:
        raise UnitError(f"Rotation vector unit must be one of {supported}.")
    from scipy.spatial.transform import Rotation as R

    r = R.from_matrix(rotation.values)
    if rotation.unit not in [units.one, units.dimensionless]:
        raise UnitError(f"Rotation matrix must be dimensionless, got {rotation.unit}.")
    return vectors(
        dims=rotation.dims, values=r.as_rotvec(degrees=unit == units.deg), unit=unit
    )


def affine_transform(
    *,
    unit: Unit | str = units.dimensionless,
    value: npt.NDArray[_Float] | Sequence[_Float],
) -> Variable:
    """
    Initializes a single affine transformation from the provided affine matrix
    coefficients.

    Parameters
    ----------
    unit:
        The unit of the affine transformation's translation component.
    value:
        A 4x4 matrix of affine coefficients.

    Returns
    -------
    :
        A scalar variable of dtype ``affine_transform3``.

    See Also
    --------
    scipp.spatial.affine_transforms:
        Create multiple affine transformations.

    Examples
    --------
    Create an affine transformation combining identity rotation and translation:

      >>> import numpy as np
      >>> import scipp as sc
      >>> affine_matrix = np.array([
      ...     [1, 0, 0, 5],
      ...     [0, 1, 0, 10],
      ...     [0, 0, 1, 15],
      ...     [0, 0, 0, 1]
      ... ])
      >>> affine = sc.spatial.affine_transform(value=affine_matrix, unit='m')
      >>> affine
      <scipp.Variable> ()  affine_transform3              [m]   1  0  0  5
       0  1  0 10
       0  0  1 15
       0  0  0  1
    """
    return affine_transforms(dims=[], unit=unit, values=value)


def affine_transforms(
    *,
    dims: Sequence[str],
    unit: Unit | str = units.dimensionless,
    values: npt.NDArray[_Float] | Sequence[Any],
) -> Variable:
    """
    Initializes affine transformations from the provided affine matrix
    coefficients.

    Parameters
    ----------
    dims:
        The dimensions of the variable.
    unit:
        The unit of the affine transformation's translation component.
    values:
        An array of 4x4 matrices of affine coefficients.

    Returns
    -------
    :
        An array variable of dtype ``affine_transform3``.

    See Also
    --------
    scipp.spatial.affine_transform:
        Create a single affine transformation.

    Examples
    --------
    Create multiple affine transformations:

      >>> import numpy as np
      >>> import scipp as sc
      >>> affine_matrices = np.array([
      ...     [[1, 0, 0, 1], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]],
      ...     [[1, 0, 0, 0], [0, 1, 0, 2], [0, 0, 1, 0], [0, 0, 0, 1]]
      ... ])
      >>> affines = sc.spatial.affine_transforms(dims=['affine'], values=affine_matrices, unit='m')
      >>> affines
      <scipp.Variable> (affine: 2)  affine_transform3              [m]  [...]
    """  # noqa: E501
    return Variable(
        dims=dims,
        unit=unit,
        values=_to_eigen_layout(values),
        dtype=DType.affine_transform3,
    )


def linear_transform(
    *,
    unit: Unit | str = units.dimensionless,
    value: npt.NDArray[np.floating[_Precision] | np.integer[_Precision]]
    | Sequence[np.floating[_Precision] | np.integer[_Precision]],
) -> Variable:
    """Constructs a zero dimensional :class:`Variable` holding a single 3x3
    matrix.

    Parameters
    ----------
    value:
        Initial value, a list of lists or 2-D NumPy array.
    unit:
        Optional, unit. Default=dimensionless

    Returns
    -------
    :
        A scalar variable of dtype ``linear_transform3``.

    See Also
    --------
    scipp.spatial.linear_transforms:
        Create multiple linear transformations.

    Examples
    --------
    Create an identity transformation:

      >>> import numpy as np
      >>> import scipp as sc
      >>> identity = sc.spatial.linear_transform(value=np.identity(3))
      >>> identity
      <scipp.Variable> ()  linear_transform3  [dimensionless]  ((1, 0, 0), , (0, 1, 0), , (0, 0, 1), )

    Create a rotation matrix (90 degrees around z-axis):

      >>> rot_matrix = np.array([[0, -1, 0], [1, 0, 0], [0, 0, 1]])
      >>> linear_rot = sc.spatial.linear_transform(value=rot_matrix)
      >>> linear_rot
      <scipp.Variable> ()  linear_transform3  [dimensionless]  ((0, -1, 0), , (1, 0, 0), , (0, 0, 1), )
    """  # noqa: E501
    return linear_transforms(
        dims=(),
        unit=unit,
        values=value,
    )


def linear_transforms(
    *,
    dims: Sequence[str],
    unit: Unit | str = units.dimensionless,
    values: npt.NDArray[np.floating[_Precision] | np.integer[_Precision]]
    | Sequence[Any],
) -> Variable:
    """Constructs a :class:`Variable` with given dimensions holding an array
    of 3x3 matrices.

    Parameters
    ----------
    dims:
        Dimension labels.
    values:
        Initial values.
    unit:
        Optional, data unit. Default=dimensionless

    Returns
    -------
    :
        An array variable of dtype ``linear_transform3``.

    See Also
    --------
    scipp.spatial.linear_transform:
        Create a single linear transformation.

    Examples
    --------
    Create multiple linear transformations:

      >>> import numpy as np
      >>> import scipp as sc
      >>> matrices = np.array([
      ...     [[1, 0, 0], [0, 1, 0], [0, 0, 1]],
      ...     [[2, 0, 0], [0, 2, 0], [0, 0, 2]]
      ... ])
      >>> linear_array = sc.spatial.linear_transforms(dims=['transform'], values=matrices)
      >>> linear_array
      <scipp.Variable> (transform: 2)  linear_transform3  [dimensionless]  [(...), (...)]
    """  # noqa: E501
    return Variable(
        dims=dims,
        unit=unit,
        values=_to_eigen_layout(values),
        dtype=DType.linear_transform3,
    )


def inv(var: Variable) -> Variable:
    """Return the inverse of a spatial transformation.

    Parameters
    ----------
    var:
        Input variable.
        Its ``dtype`` must be one of

        - :attr:`scipp.DType.linear_transform3`
        - :attr:`scipp.DType.affine_transform3`
        - :attr:`scipp.DType.rotation3`
        - :attr:`scipp.DType.translation3`

    Returns
    -------
    :
        A variable holding the inverse transformation to ``var``.

    Examples
    --------
    Inverse of a translation:

      >>> import scipp as sc
      >>> trans = sc.spatial.translation(value=[1, 2, 3], unit='m')
      >>> inv_trans = sc.spatial.inv(trans)
      >>> inv_trans
      <scipp.Variable> ()  translation3              [m]  (-1, -2, -3)

    Inverse of a rotation:

      >>> import numpy as np
      >>> rot = sc.spatial.rotation(value=[0, 0, 1/np.sqrt(2), 1/np.sqrt(2)])
      >>> inv_rot = sc.spatial.inv(rot)
      >>> inv_rot
      <scipp.Variable> ()  rotation3  [dimensionless]  (...-0.707107k)

    Inverse of a scaling transformation:

      >>> scale = sc.spatial.scaling_from_vector(value=[2, 3, 4])
      >>> inv_scale = sc.spatial.inv(scale)
      >>> inv_scale
      <scipp.Variable> ()  linear_transform3  [dimensionless]  ((0.5, 0, 0), , (0, 0.333333, 0), , (0, 0, 0.25), )
    """  # noqa: E501
    return _call_cpp_func(_core_cpp.inv, var)  # type: ignore[return-value]


__all__ = [
    'affine_transform',
    'affine_transforms',
    'inv',
    'linear_transform',
    'linear_transforms',
    'rotation',
    'rotation_as_rotvec',
    'rotations',
    'rotations_from_rotvecs',
    'scaling_from_vector',
    'scalings_from_vectors',
    'translation',
    'translations',
]
