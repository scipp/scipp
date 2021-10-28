from ._scipp.core import Unit as _Unit


def __rmul(self, scalar):
    tp = type(scalar)
    if tp in [float, int]:
        return self.__rmul(scalar)
    else:
        return self.__rmul(scalar, scalar.dtype)


def __rtruediv(self, scalar):
    tp = type(scalar)
    if tp in [float, int]:
        return self.__rtruediv(scalar)
    else:
        return self.__rtruediv(scalar, scalar.dtype)


# add magic python methods to Unit class
# it is done here (on python side) because
# there is no proper way to do this in pybind11
setattr(_Unit, '__rtruediv__', __rtruediv)
setattr(_Unit, '__rmul__', __rmul)

# forbid numpy to apply ufuncs to unit
# wrong behavior in scalar * unit otherwise
setattr(_Unit, "__array_ufunc__", None)
