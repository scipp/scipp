from .. import matrices, units, UnitError, Variable


def from_rotvec(rotvec: Variable) -> Variable:
    """
    Create rotation matrix (or matrices) from rotation vector(s).

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
