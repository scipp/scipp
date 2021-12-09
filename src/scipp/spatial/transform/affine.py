from typing import Union, Sequence
import numpy as _np

from ...core.variable import _to_eigen_layout
from ..._scipp import core as _core_cpp


def affine_transform(*,
                     unit: Union[_core_cpp.Unit, str] = _core_cpp.units.dimensionless,
                     value: Union[_np.ndarray, list]):
    """
    Initializes a single affine transformation from the provided affine matrix
    coefficients.

    :param unit: The unit of the affine transformation's translation component.
    :param value: A 4x4 matrix of affine coefficients.
    """
    return _core_cpp.affine_transforms(dims=[],
                                       unit=unit,
                                       values=_to_eigen_layout(value))


def affine_transforms(*,
                      dims: Sequence[str],
                      unit: Union[_core_cpp.Unit, str] = _core_cpp.units.dimensionless,
                      values: Union[_np.ndarray, list]):
    """
    Initializes affine transformations from the provided affine matrix
    coefficients.

    :param unit: The unit of the affine transformation's translation component.
    :param value: An array of 4x4 matrix of affine coefficients.
    """
    return _core_cpp.affine_transforms(dims=dims,
                                       unit=unit,
                                       values=_to_eigen_layout(values))
