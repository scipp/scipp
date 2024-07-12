from typing import Any

from ._scipp.core import Unit as _Unit
from .core import Variable, scalar


def __rmul(self: _Unit, value: Any) -> Variable:
    return scalar(value, unit=self)


def __rtruediv(self: _Unit, value: Any) -> Variable:
    return scalar(value, unit=self ** (-1))


def extend_units() -> None:
    # add magic python methods to Unit class
    # it is done here (on python side) because
    # there is no proper way to do this in pybind11
    _Unit.__rtruediv__ = __rtruediv
    _Unit.__rmul__ = __rmul

    # forbid numpy to apply ufuncs to unit
    # wrong behavior in scalar * unit otherwise
    _Unit.__array_ufunc__ = None
