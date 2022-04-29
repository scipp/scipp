from __future__ import annotations
from typing import Union

from ..logging import get_logger
from ..typing import VariableLike

def _dtype_lut():
    from .._scipp.core import DType as d
    # For types understood by numpy we do not actually need this special
    # handling, but will do as we add support for other types such as
    # variable-length strings.
    dtypes = [
        d.float64, d.float32, d.int64, d.int32, d.bool, d.datetime64, d.string,
        d.Variable, d.DataArray, d.Dataset, d.VariableView, d.DataArrayView,
        d.DatasetView, d.vector3, d.linear_transform3, d.affine_transform3,
        d.translation3, d.rotation3
    ]
    names = [str(dtype) for dtype in dtypes]
    return dict(zip(names, dtypes))


class DatasetFormatter:
    @staticmethod
    def format(data, spec):
        get_logger().warning("Dataset formatting not supported yet.")
        return f"{data}"


class DataArrayFormatter:
    @staticmethod
    def format(data, spec):
        get_logger().warning("DataArray formatting not supported yet.")
        return f"{data}"


class VariableFormatter:
    @staticmethod
    def format(data, spec):
        dtype = str(data.dtype)
        if not any([x in dtype for x in ('float', 'int')]) or spec is None or len(spec) < 1:
            return data.__repr__()
        compact = spec[-1] == 'c'
        is_scalar = False if data.shape else True
        val = data.value if is_scalar else data.values
        var = data.variance if is_scalar else data.variances
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
                es *= pp
                vs = vs.astype('int')
            specs = ['d' if x > -1 else f'0.{int(-x):d}f' for x in p]
            fvs = ["{v:{x}}".format(v=v, x=spec) for v, spec in zip(vs, specs)]
            return f"{', '.join([f'{v}({e})' for v,e in zip(fvs, es)])} {data.unit}"
        # elif is_scalar:
        #     if compact:
        #         spec[-1] = 'f' if '.' in spec else 'd'
        #     # how can we pass the format specification into the C++ object?

        # punt (for now)
        return data.__repr__()


class Formatter:
    _handlers = dict(
        zip(['Variable', 'DataArray', 'Dataset'], [VariableFormatter, DataArrayFormatter, DatasetFormatter]))

    @classmethod
    def format(cls, data, spec):
        name = data.__class__.__name__.replace('View', '')
        return cls._handlers[name].format(data, spec)


def formatter(obj: VariableLike, spec: str):
    """
    String formats the object according to the provided specification.
    Parameters
    ----------
    obj
        A scalar or array-like scipp Variable object
    spec
        The format specification; only 'c' for Compact error-reporting is supported at present

    Returns
    -------
    The formatted string
    """
    return Formatter.format(obj, spec)
