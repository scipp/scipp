from .. import vectors, matrices, units, Unit, UnitError, Variable


def from_rotvec(rotvec: Variable) -> Variable:
    """
    Create rotation matrix (or matrices) from rotation vector(s).

    This requires ``scipy`` to be installed, as is wraps ``Rotation.from_rotvec()``
    from ``scipy.spatial.transform``.

    A rotation vector is a 3 dimensional vector which is co-directional to the axis of
    rotation and whose norm gives the angle of rotation.

    :param rotvec: A variable with vector(s) giving rotation axis and angle. Unit must
                   be rad or deg.
    """
    from scipy.spatial.transform import Rotation as R
    supported = [units.deg, units.rad]
    if rotvec.unit not in supported:
        raise UnitError(f"Rotation vector unit must be one of {supported}.")
    r = R.from_rotvec(rotvec.values, degrees=rotvec.unit == units.deg)
    return matrices(dims=rotvec.dims, values=r.as_matrix())


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
