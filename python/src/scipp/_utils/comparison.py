import scipp as sc


def is_near(a,
            b,
            rtol=None,
            atol=None,
            include_attrs=True,
            include_data=True,
            equal_nan=True):
    """
    Similar to scipp is_close, but intended to compare whole DataArrays.
    Coordinates compared element by element with abs(a - b) <= atol + rtol * b

    :param a: lhs input
    :param b: rhs input
    :param rtol: relative tolerance (to b)
    :param atol: absolute tolerance
    :param include_data: Compare that data is within tolerance
    :param include_attrs: Compare all meta (coords and attrs) between a and b,
           otherwise only compare coordinates from meta
    :param equal_nan: If True, consider Nans or Infs to be equal
                       providing that they match in location and, for infs,
                       have the same sign
    :type a: DataArray
    :type b: DataArray
    :type tol :
    :raises: If a, b are not going to be logically comparable
             for reasons relating to shape, item naming or non-finite elements.
    :return True if near:
    """
    same_data = sc.all(
        sc.is_close(a.data, b.data, rtol=rtol, atol=atol,
                    equal_nan=equal_nan)).value if include_data else True
    same_len = len(a.meta) == len(b.meta) if include_attrs else len(
        a.coords) == len(b.coords)
    if not same_len:
        raise RuntimeError('Different number of items'
                           f'in meta {len(a.meta)} {len(b.meta)}')
    for key, val in a.meta.items() if include_attrs else a.coords.items():
        x = a.meta[key] if include_attrs else a.coords[key]
        y = b.meta[key] if include_attrs else b.coords[key]
        if x.shape != y.shape:
            raise RuntimeError(f'For meta {key} have different'
                               f' shapes {x.shape}, {y.shape}')
        if val.dtype in [sc.dtype.float64, sc.dtype.float32]:
            if not sc.sum(~sc.isfinite(x)).value == sc.sum(
                    ~sc.isfinite(y)).value:
                raise RuntimeError(
                    f'For meta {key} different numbers of non-finite entries')
            if not sc.all(
                    sc.is_close(
                        x, y, rtol=rtol, atol=atol,
                        equal_nan=equal_nan)).value:
                return False
    return same_data
