from typing import Union, Sequence
import numpy as _np
from ... import matrices


def from_vector(*, value: Union[_np.ndarray, list]):
    """
    Creates a scaling transformation from a provided 3-vector.

    :param value: a list or numpy array of 3 values, corresponding to scaling
        coefficients in the x, y and z directions respectively.
    """
    return matrices(dims=[], values=_np.diag(value))


def from_vectors(*, dims: Sequence[str], values: Union[_np.ndarray, list]):
    """
    Creates scaling transformations from corresponding to the provided 3-vectors.

    :param dims: the dimensions of the variable
    :param values: a list or numpy array of 3-vectors, each corresponding to scaling
        coefficients in the x, y and z directions respectively.
    """
    return matrices(dims=dims, values=[_np.diag(v) for v in values])
