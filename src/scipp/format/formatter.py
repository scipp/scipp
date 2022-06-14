def format_variable(data, spec):
    """
    String formats the Variable according to the provided specification.

    Parameters
    ----------
    data
        A scalar or array-like scipp Variable object
    spec
        Format specification; only 'c' for Compact error-reporting supported at present

    Returns
    -------
    The formatted string
    """
    from ..core.cpp_classes import Unit
    from numpy import array
    dtype = str(data.dtype)
    if not any([x in dtype for x in ('float', 'int')]) or spec is None or len(spec) < 1:
        return data.__repr__()
    compact = spec[-1] == 'c'

    val = data.values if data.shape else array((data.value, ))
    var = data.variances if data.shape else array((data.variance, ))
    unt = "" if data.unit == Unit('dimensionless') else f" {data.unit}"

    if compact:
        # Iterate over array values to handle no- and infinite-precision cases
        if var is None:
            formatted = [_format(v) for v in val]
        else:
            formatted = [_format(*_round(v, e)) for v, e in zip(val, var)]
        return f"{', '.join(formatted)}{unt}"

    # punt (for now)
    return data.__repr__()


def _round(value, variance):
    from numpy import floor, log10, round, power, sqrt
    # Treat 'infinite' precision the same as no variance
    if variance is None or variance == 0:
        return value, None, None

    # The uncertainty is the square root of the variance
    error = sqrt(variance)

    # Determine how many digits before (+) or after (-) the decimal place
    # the error allows for one-digit uncertainty of precision
    precision = floor(log10(error))

    # By convention, if the first digit of the error rounds to 1
    # add an extra digit of precision, so there are two-digits of uncertainty
    if round(error * power(10., -precision)) == 1:
        precision -= 1

    # Build powers of ten to enable rounding to the specified precision
    negative_power = power(10., -precision)
    positive_power = power(10., precision)

    # Round the error, keeping the shifted value for the compact string
    error = int(round(error * negative_power))
    # Round the value, shifting back after rounding
    value = round(value * negative_power) * positive_power

    # If the precision is greater than that of 0.1
    if precision > -1:
        # pad the error to have the right number of trailing zeros
        error *= int(positive_power)

    return value, error, precision


def _format(value, error=None, precision=None):
    # Build the appropriate format string:
    # No variance (or infinite precision) values take no formatting string
    # Positive precision implies no decimals, with format '0.0f'
    format = '' if precision is None else f'0.{max(0, int(-precision)):d}f'

    # Format the value using the generated format string
    formatted = "{v:{s}}".format(v=value, s=format)

    # Append the error if there is non-infinite-precision variance
    if error is not None:
        formatted = f'{formatted}({error})'

    return formatted
