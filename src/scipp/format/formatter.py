from numpy import ndarray


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

    if compact and not (var is None or any([x is None for x in var])):
        from numpy import sqrt
        return f"{_format_to(*_round_to(val, sqrt(var)))}{unt}"
    elif compact:
        return f"{', '.join([f'{v}' for v in val])}{unt}"

    # punt (for now)
    return data.__repr__()


def _round_to(values: ndarray, errors: ndarray):
    from numpy import floor, log10, round, power, any
    # Determine how many digits before (+) or after (-) the decimal place
    # the error(s) allow for one-digit uncertainty of precision
    precision = floor(log10(errors))

    # By convention, if the first digit of the error rounds to 1
    # add an extra digit of precision, so there are two-digits of uncertainty
    precision[round(errors * power(10., -precision)) == 1] -= 1

    # Build powers of ten to enable rounding to the specified precision
    negative_power = power(10., -precision)
    positive_power = power(10., precision)

    # Round the error(s), keeping the shifted value(s) for the compact string
    errors = round(errors * negative_power).astype('int')
    # Round the value(s), shifting back after rounding
    values = round(values * negative_power) * positive_power

    # If the precision is greater than that of 0.1
    greater = precision > -1
    if any(greater):
        # pad the error(s) to have the right number of trailing zeros
        errors[greater] *= positive_power[greater].astype('int')

    return values, errors, precision


def _format_to(values: ndarray, errors: ndarray, precision: ndarray):
    # positive precisions should have no decimals, accomplished by '0.0f'
    formats = [f'0.{max(0, int(-p)):d}f' for p in precision]

    # format the values to the required precision
    formatted = ["{v:{s}}".format(v=v, s=s) for v, s in zip(values, formats)]

    # and add on the formatted uncertainties
    formatted = [f'{v}({e})' for v, e in zip(formatted, errors)]

    # concatenate multiple values
    return ', '.join(formatted)
