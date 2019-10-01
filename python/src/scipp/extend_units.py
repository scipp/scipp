from ._scipp.core import Unit


def __rmul(self, scalar):
    tp = type(scalar)
    if tp in [float, int]:
        return self.__rmul(scalar)
    else:
        return self.__rmul(scalar, scalar.dtype)


setattr(Unit, '__rmul__', __rmul)


def __rtruediv(self, scalar):
    tp = type(scalar)
    if tp in [float, int]:
        return self.__rtruediv(scalar)
    else:
        return self.__rtruediv(scalar, scalar.dtype)


setattr(Unit, '__rtruediv__', __rtruediv)

setattr(Unit, "__array_ufunc__", None)
