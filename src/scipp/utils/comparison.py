import scipp as sc


def isnear(x,
           y,
           rtol=None,
           atol=None,
           include_attrs=True,
           include_data=True,
           equal_nan=True):
    """
    Similar to scipp isclose, but intended to compare whole DataArrays.
    Coordinates compared element by element
    with abs(x - y) <= atol + rtol * abs(y)

    Compared coord and attr pairs are only considered equal if all
    element-wise comparisons are True.

    See scipp isclose for more details on how the comparions on each
    item will be conducted.

    :param x: lhs input
    :param y: rhs input
    :param rtol: relative tolerance (to y)
    :param atol: absolute tolerance
    :param include_data: Compare data element-wise between x, and y
    :param include_attrs: Compare all meta (coords and attrs) between x and y,
           otherwise only compare coordinates from meta
    :param equal_nan: If True, consider Nans or Infs to be equal
                       providing that they match in location and, for infs,
                       have the same sign
    :type x: DataArray
    :type y: DataArray
    :type tol :
    :raises: If x, y are not going to be logically comparable
             for reasons relating to shape, item naming or non-finite elements.
    :return True if near:
    """
    same_data = sc.all(
        sc.isclose(x.data, y.data, rtol=rtol, atol=atol,
                   equal_nan=equal_nan)).value if include_data else True
    same_len = len(x.meta) == len(y.meta) if include_attrs else len(x.coords) == len(
        y.coords)
    if not same_len:
        return False
    for key, val in x.meta.items() if include_attrs else x.coords.items():
        a = x.meta[key] if include_attrs else x.coords[key]
        b = y.meta[key] if include_attrs else y.coords[key]
        if a.shape != b.shape:
            raise sc.CoordError(
                f'Coord (or attr) with key {key} have different'
                f' shapes. For x, shape is {a.shape}. For y, shape = {b.shape}')
        if val.dtype in [sc.dtype.float64, sc.dtype.float32]:
            if not sc.all(sc.isclose(a, b, rtol=rtol, atol=atol,
                                     equal_nan=equal_nan)).value:
                return False
    return same_data
