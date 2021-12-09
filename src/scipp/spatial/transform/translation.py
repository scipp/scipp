from typing import Union, Sequence
import numpy as _np

from ..._scipp import core as _core_cpp


def from_vector(*,
                unit: Union[_core_cpp.Unit, str] = _core_cpp.units.dimensionless,
                value: Union[_np.ndarray, list]):
    """
    Creates a translation transformation from a single provided 3-vector.

    :param unit: The unit of the translation
    :param value: A list or numpy array of 3 items
    """
    return _core_cpp.translations(dims=[], unit=unit, values=value)


def from_vectors(*,
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
