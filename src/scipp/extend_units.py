from ._scipp.core import Unit as _Unit
from .core import scalar


def __rmul(self, value):
    return scalar(value, unit=self)


def __rtruediv(self, value):
    return scalar(value, unit=self**(-1))


# add magic python methods to Unit class
# it is done here (on python side) because
# there is no proper way to do this in pybind11
setattr(_Unit, '__rtruediv__', __rtruediv)
setattr(_Unit, '__rmul__', __rmul)

# forbid numpy to apply ufuncs to unit
# wrong behavior in scalar * unit otherwise
setattr(_Unit, "__array_ufunc__", None)
