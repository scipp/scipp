# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Gregory Tucker, Jan-Lukas Wynen

from collections.abc import Sequence
from typing import Any

import numpy as np
import numpy.typing as npt

import scipp

from ..core.cpp_classes import DType, Unit, Variable
from ..core.data_group import DataGroup
from ._parse import FormatSpec, FormatType, Selection, parse


def format_variable(self: Variable, format_spec: str) -> str:
    """String formats the Variable according to the provided specification.

    Parameters
    ----------
    format_spec:
        Format specification;
        only 'c' for Compact error-reporting supported at present.

    Returns
    -------
    :
        The formatted string.
    """

    spec = parse(format_spec, Variable)
    return _VARIABLE_FORMATTERS[spec.format_type](self, spec)


def _format_sizes(data: Variable) -> str:
    return '(' + ', '.join(f'{dim}: {size}' for dim, size in data.sizes.items()) + ')'


def _format_unit(data: Variable) -> str:
    if data.unit is None:
        return '<no unit>'
    return f'[{data.unit}]'


def _format_element(elem: Any, *, dtype: DType, spec: str) -> str:
    if spec:
        return f'{elem:{spec}}'
    if dtype in (DType.float64, DType.float32):
        # Replicate behavior of C++ formatter.
        return f'{elem:g}'
    if dtype == DType.string:
        return f'"{elem}"'
    return f'{elem}'


def _format_scalar(data: Any, *, dtype: DType, spec: FormatSpec) -> str:
    if spec.length == 0:
        return '...'
    return _format_element(data, dtype=dtype, spec=spec.nested)


def _as_flat_array(data: npt.ArrayLike) -> npt.ArrayLike:
    if isinstance(data, np.ndarray):
        return data.flat
    if 'ElementArray' in repr(type(data)):
        return data
    return np.array([data])


def _format_array_flat(data: Any, *, dtype: DType, spec: FormatSpec) -> str:
    if dtype in (
        DType.Variable,
        DType.DataArray,
        DType.Dataset,
        DType.VariableView,
        DType.DataArrayView,
        DType.DatasetView,
    ):
        return _format_array_flat_scipp_objects(data)
    if dtype == DType.PyObject:
        if 'ElementArray' in repr(type(data)):
            # We can handle scalars of PyObject but not arrays.
            return _format_array_flat_scipp_objects(data)
        elif isinstance(data, DataGroup):
            return _format_data_group_element(data)
    data = _as_flat_array(data)
    return _format_array_flat_regular(data, dtype=dtype, spec=spec)


def _format_array_flat_scipp_objects(data: npt.ArrayLike) -> str:
    # Fallback because ElementArrayView does not allow us to
    # slice and access elements nicely.
    return str(data)


def _format_data_group_element(data: scipp.DataGroup[object]) -> str:
    return f'[{data}]'


def _element_ranges(spec: FormatSpec) -> tuple[slice, slice]:
    match spec.selection:
        case Selection.edges:
            return slice(None, spec.length // 2), slice(-spec.length // 2, None)
        case Selection.begin:
            return slice(None, spec.length), slice(0, 0)
        case Selection.end:
            return slice(0, 0), slice(-spec.length, None)


def _format_array_flat_regular(
    data: Sequence[Any], *, dtype: DType, spec: FormatSpec
) -> str:
    def _format_all_in(d: Sequence[Any]) -> list[str]:
        return [_format_element(e, dtype=dtype, spec=spec.nested) for e in d]

    if len(data) <= spec.length:
        elements = _format_all_in(data)
    elif spec.length == 0:
        elements = ['...']
    else:
        left, right = _element_ranges(spec)
        elements = []
        if left != slice(0, 0):
            elements.extend(_format_all_in(data[left]))
        elements.append('...')
        if right != slice(0, 0):
            elements.extend(_format_all_in(data[right]))
    return f'[{", ".join(elements)}]'


def _format_variable_default(var: Variable, spec: FormatSpec) -> str:
    dims = _format_sizes(var)
    dtype = str(var.dtype)
    unit = _format_unit(var)
    if var.ndim == 0:
        values = _format_scalar(var.value, dtype=var.dtype, spec=spec)
        variances = (
            _format_scalar(var.variance, dtype=var.dtype, spec=spec)
            if var.variance is not None
            else ''
        )
    else:
        values = _format_array_flat(var.values, dtype=var.dtype, spec=spec)
        variances = (
            _format_array_flat(var.variances, dtype=var.dtype, spec=spec)
            if var.variances is not None
            else ''
        )

    return f'<scipp.Variable> {dims}  {dtype:>9}  {unit:>15}  {values}' + (
        '  ' + variances if variances else ''
    )


def _format_variable_compact(var: Variable, spec: FormatSpec) -> str:
    if spec.has_nested or spec.has_length or spec.has_selection:
        raise ValueError(f"Invalid format spec for compact formatter: '{spec}'")
    if not _is_numeric(var.dtype):
        raise ValueError(f"Compact formatting is not supported for dtype {var.dtype}")

    values = var.values if var.shape else np.array((var.value,))
    variances = var.variances if var.shape else np.array((var.variance,))
    unt = "" if var.unit == Unit('dimensionless') else f" {var.unit}"

    # Iterate over array values to handle no- and infinite-precision cases
    if variances is None:
        formatted = [_format_element_compact(v) for v in values]
    else:
        formatted = [
            _format_element_compact(*_round(v, e))
            for v, e in zip(values, variances, strict=True)
        ]
    return f"{', '.join(formatted)}{unt}"


def _is_numeric(dtype: DType) -> bool:
    return any(x in str(dtype) for x in ('float', 'int'))


def _round(
    value: float, variance: float | None
) -> tuple[float, float | None, float | None]:
    from numpy import floor, log10, power, round, sqrt

    # Treat 'infinite' precision the same as no variance
    if variance is None or variance == 0:
        return value, None, None

    # The uncertainty is the square root of the variance
    error = sqrt(variance)

    # Determine how many digits before (+) or after (-) the decimal place
    # the error allows for one-digit uncertainty of precision
    precision = floor(log10(error))

    # By convention, if the first digit of the error rounds to 1,
    # add an extra digit of precision, so there are two-digits of uncertainty
    if round(error * power(10.0, -precision)) == 1:
        precision -= 1

    # Build powers of ten to enable rounding to the specified precision
    negative_power = power(10.0, -precision)
    positive_power = power(10.0, precision)

    # Round the error, keeping the shifted value for the compact string
    error = int(round(error * negative_power))
    # Round the value, shifting back after rounding
    value = round(value * negative_power) * positive_power

    # If the precision is greater than that of 0.1
    if precision > -1:
        # pad the error to have the right number of trailing zeros
        error *= int(positive_power)

    return value, error, precision


def _format_element_compact(
    value: float, error: float | None = None, precision: float | None = None
) -> str:
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


_VARIABLE_FORMATTERS = {
    FormatType.default: _format_variable_default,
    FormatType.compact: _format_variable_compact,
}
