# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

import numpy as np
from typing import Any, List, Optional
from .. import DataArray, Dataset, Variable
from ..typing import MetaDataMap, VariableLike
from .. import DType


def _string_in_cell(v: Variable) -> str:

    if v.bins is not None:
        return f'<td>len={v.value.shape}</td>'
    if v.dtype in (DType.vector3, DType.string):
        return f'<td>{v.value}</td>'
    if v.variances is None:
        return f'<td>{round(v.value, 3)}</td>'
    err = np.sqrt(v.variance)
    prec = -int(np.floor(np.log10(err)))
    v_str = round(v.value, prec)
    e_str = round(err, prec)
    return f'<td>{v_str}&plusmn;{e_str}</td>'


def _make_groups(obj: Dataset, attrs: MetaDataMap) -> List[Any]:

    out = [obj.coords, obj]
    if attrs:
        out.append(obj[list(obj.keys())[0]].attrs)
    return out


def _make_row(obj: Dataset, attrs: MetaDataMap) -> str:

    out = ''
    for group in _make_groups(obj, attrs=attrs):
        for var in group.values():
            out += _string_in_cell(var[0])
    return out


def _empty_strings_or_values(group: MetaDataMap) -> List[str]:

    return [
        _string_in_cell(var[-1]) if group.is_edges(key) else '<td></td>'
        for key, var in group.items()
    ]


def table(obj: VariableLike, max_rows: Optional[int] = 20):
    """Creates a html table from the contents of a :class:`Dataset`, :class:`DataArray`,
    or :class:`Variable`.

    Parameters
    ----------
    obj:
        Input object.
    max_rows:
        Optional, maximum number of rows to display.
    """
    if obj.ndim != 1:
        raise ValueError("Table can only be generated for one-dimensional objects.")

    out = '<table><tr>'
    attrs = {}

    if isinstance(obj, DataArray):
        attrs = obj.attrs
        obj = Dataset({obj.name: obj})
    if isinstance(obj, Variable):
        obj = Dataset(data={"": obj})

    # Create first table row with group headers
    if obj.coords:
        out += f'<th colspan="{len(obj.coords)}">Coordinates</th>'
    out += f'<th colspan="{len(obj.keys())}">Data</th>'
    if attrs:
        out += f'<th colspan="{len(attrs)}">Attributes</th>'
    out += '</tr>'

    # Create second table row with column names
    ncols = 0
    for group in _make_groups(obj, attrs):
        for name, var in group.items():
            out += f'<th>{name}'
            unit = var.bins.unit if var.bins is not None else var.unit
            if unit is not None:
                out += ' [𝟙]' if unit == 'dimensionless' else f' [{unit}]'
            out == '</th>'
            ncols += 1

    # Limit the number of rows to be printed
    size = obj.shape[0]
    if size > max_rows:
        half = int(max_rows / 2)
        inds = list(range(half)) + [None] + list(range(size - half, size))
    else:
        inds = range(size)

    # Generate html rows
    for i in inds:
        if i is None:
            out += '<tr>' + ('<td>...</td>' * ncols) + '</tr>'
        else:
            out += f'<tr>{_make_row(obj=obj[i:i+1], attrs=attrs)}</tr>'

    # Maybe add an extra row if there are bin edges in the coords or attrs
    empty_cell = '<td></td>'
    bin_edge_coords = _empty_strings_or_values(obj.coords)
    bin_edge_attrs = _empty_strings_or_values(attrs)
    extra_row = "".join(bin_edge_coords + ([empty_cell] * len(obj.keys())) +
                        bin_edge_attrs)
    if len(extra_row) > len(empty_cell) * ncols:
        out += f'<tr>{extra_row}</tr>'

    out += '</table>'

    from IPython.display import display, HTML
    display(HTML(out))
