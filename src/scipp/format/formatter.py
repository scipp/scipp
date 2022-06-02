def format_variable(data, spec):
    """
    String formats the Variable according to the provided specification.
    Parameters
    ----------
    obj
        A scalar or array-like scipp Variable object
    spec
        Format specification; only 'c' for Compact error-reporting supported at present

    Returns
    -------
    The formatted string
    """
    from scipp import Unit
    dtype = str(data.dtype)
    if not any([x in dtype for x in ('float', 'int')]) or spec is None or len(spec) < 1:
        return data.__repr__()
    compact = spec[-1] == 'c'
    is_scalar = False if data.shape else True
    val = data.value if is_scalar else data.values
    var = data.variance if is_scalar else data.variances
    unt = "" if data.unit == Unit('dimensionless') else f" {data.unit}"
    if compact and var is not None:
        from numpy import floor, log10, round, array, sqrt, power
        if is_scalar:
            val = array((val, ))
            var = array((var, ))
        err = sqrt(var)
        p = floor(log10(err))
        p[round(err * power(10., -p)).astype('int') == 1] -= 1
        np, pp = power(10., -p), power(10., p)
        es = round(err * np).astype('int')
        vs = round(val * np) * pp
        if p > -1:
            es *= pp.astype('int')
            vs = vs.astype('int')
        specs = ['d' if x > -1 else f'0.{int(-x):d}f' for x in p]
        fvs = ["{v:{x}}".format(v=v, x=spec) for v, spec in zip(vs, specs)]
        return f"{', '.join([f'{v}({e})' for v,e in zip(fvs, es)])}{unt}"
    elif compact:
        if is_scalar:
            from numpy import array
            val = array((val, ))
        return f"{', '.join([f'{v}' for v in val])}{unt}"

    # punt (for now)
    return data.__repr__()
