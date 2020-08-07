from ._scipp import core as sc


def _call_cpp_func(func, *args, out=None, **kwargs):
    if out is None:
        return func(*args, **kwargs)
    else:
        return func(*args, **kwargs, out=out)


def abs(x, out=None):
    """Element-wise absolute value.

    :param x: Input data.
    :param out: Optional output buffer.
    :raises: If the dtype has no absolute value, e.g., if it is a string.
    :return: The absolute values of the input.
    :seealso: `scipp.norm` for vector-like dtype.
    """
    return _call_cpp_func(sc.abs, x, out=out)


def nan_to_num(x, nan=None, posinf=None, neginf=None, out=None):
    """Element-wise special value replacement.

    All elements in the output are identical to input except in the presence
    of a NaN, Inf or -Inf.
    The function allows replacements to be separately specified for nan, inf
    or -inf values.
    You can choose to replace a subset of those special values by providing
    just the required key word arguments.
    If the replacement is value-only and the input has variances, the variance
    at the element(s) undergoing replacement are also replaced with the
    replacement value.
    If the replacement has a variance and the input has variances, the
    variance at the element(s) undergoing replacement are also replaced with
    the replacement variance.

    :param x: Input data.
    :param nan: Replacement values for NaN in the input.
    :param posinf: Replacement values for Inf in the input.
    :param neginf: Replacement values for -Inf in the input.
    :param out: Optional output buffer.
    :raises: If the types of input and replacement do not match.
    :return: Input elements are replaced in output with specified subsitutions.
    """
    return _call_cpp_func(sc.nan_to_num, x, nan, posinf, neginf, out=out)


def norm(x):
    """Element-wise norm.

    :param x: Input data.
    :raises: If the dtype has no norm, i.e., if it is not a vector.
    :return: Scalar elements computed as the norm values of the input elements.
    """
    return _call_cpp_func(sc.norm, x, out=None)


def reciprocal(x, out=None):
    """Element-wise reciprocal.

    :param x: Input data.
    :param out: Optional output buffer.
    :raises: If the dtype has no reciprocal, e.g., if it is a string.
    :return: The reciprocal values of the input.
    """
    return _call_cpp_func(sc.reciprocal, x, out=out)


def sqrt(x, out=None):
    """Element-wise square-root.

    :param x: Input data.
    :param out: Optional output buffer.
    :raises: If the dtype has no square-root, e.g., if it is a string.
    :return: The square-root values of the input.
    """
    return _call_cpp_func(sc.sqrt, x, out=out)
