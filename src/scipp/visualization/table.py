# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

from typing import Any

import numpy as np

from .. import DataArray, Dataset, DType, Variable
from ..typing import VariableLike

CENTER = 'text-align: center;'
LEFT_BORDER = 'border-left:1px solid #a9a9a9;'
BOTTOM_BORDER = 'border-bottom:2px solid #a9a9a9;'


def _string_in_cell(v: Variable) -> str:
    if v.is_binned:
        return f'len={v.value.shape}'
    if v.dtype not in (DType.float32, DType.float64):
        return str(v.value)
    if v.variance is None:
        return f'{v.value:.3f}'
    err = np.sqrt(v.variance)
    if err == 0.0:
        prec = 3
    else:
        try:
            prec = max(3, -int(np.floor(np.log10(err))))
        except OverflowError:
            prec = 3  # happens when err is non-finite
    return f'{v.value:.{prec}f}&plusmn;{err:.{prec}f}'


def _var_name_with_unit(name: str, var: Variable) -> str:
    out = f'<span style="font-weight: bold;">{name}</span>'
    try:
        unit = var.unit
    except RuntimeError:  # binned variable with Dataset content
        unit = None
    if unit is not None:
        out += ' [𝟙]' if unit == 'dimensionless' else f' [{unit}]'  # noqa: RUF001
    return out


def _add_td_tags(cell_list: list[str], border: str = '') -> list[str]:
    td = f' style="{border}"' if border else ''
    td = f'<td{td}>'
    return [f'{td}{cell}</td>' for cell in cell_list]


def _make_variable_column(
    name: str,
    var: Variable,
    indices: list[int | None] | range,
    need_bin_edge: bool,
    is_bin_edge: bool,
    border: str = '',
) -> list[str]:
    head = [_var_name_with_unit(name, var)]
    rows = []
    for i in indices:
        if i is None:
            rows.append('...')
        else:
            rows.append(_string_in_cell(var[i]))
    if need_bin_edge:
        if is_bin_edge:
            rows.append(_string_in_cell(var[-1]))
        else:
            rows.append('')
    return _add_td_tags(head, border=border + BOTTOM_BORDER) + _add_td_tags(
        rows, border=border
    )


def _make_data_array_table(
    da: DataArray,
    indices: list[int | None] | range,
    bin_edges: bool,
    no_left_border: bool = False,
) -> list[list[str]]:
    out = [
        _make_variable_column(
            name='',
            var=da.data,
            indices=indices,
            need_bin_edge=bin_edges,
            is_bin_edge=False,
            border='' if no_left_border else LEFT_BORDER,
        )
    ]

    for name, var in sorted(da.masks.items()):
        out.append(
            _make_variable_column(
                name=name,
                var=var,
                indices=indices,
                need_bin_edge=bin_edges,
                is_bin_edge=False,
            )
        )

    return out


def _make_entries_header(ds: Dataset) -> str:
    out = '<tr>'
    if ds.coords:
        out += f'<th colspan="{len(ds.coords)}"></th>'
    for name, da in sorted(ds.items()):
        ncols = 1 + len(da.masks)
        out += f'<th style="{CENTER}" colspan="{ncols}">{name}</th>'
    out += '</tr>'
    return out


def _make_sections_header(ds: Dataset) -> str:
    out = '<tr>'
    if ds.coords:
        out += f'<th style="{CENTER}" colspan="{len(ds.coords)}">Coordinates</th>'
    for i, (_, da) in enumerate(sorted(ds.items())):
        border = '' if (i == 0) and (not ds.coords) else LEFT_BORDER
        out += f'<th style="{CENTER + border}">Data</th>'
        if da.masks:
            out += f'<th style="{CENTER}" colspan="{len(da.masks)}">Masks</th>'
    out += '</tr>'
    return out


def _to_html_table(header: str, body: list[list[str]]) -> str:
    out = '<table>' + header
    ncols = len(body)
    nrows = len(body[0])
    for i in range(nrows):
        out += '<tr>' + ''.join(body[j][i] for j in range(ncols)) + '</tr>'
    out += '</table>'
    return out


def _find_bin_edges(ds: Dataset) -> bool:
    for key in ds.coords:
        if ds.coords.is_edges(key):
            return True
    return False


def _strip_scalars_and_broadcast_masks(ds: Dataset) -> Dataset:
    out = {}
    for key, da in ds.items():
        out[key] = DataArray(
            data=da.data,
            coords={
                key: var for key, var in da.coords.items() if var.dims == da.data.dims
            },
            masks={key: var.broadcast(sizes=da.sizes) for key, var in da.masks.items()},
        )
    return Dataset(out)


def _to_dataset(obj: VariableLike | dict[str, Variable | DataArray]) -> Dataset:
    if isinstance(obj, DataArray):
        return Dataset({obj.name: obj})
    if isinstance(obj, Variable):
        return Dataset(data={"": obj})
    if isinstance(obj, dict):
        return Dataset(obj)
    if isinstance(obj, Dataset):
        return obj
    raise TypeError(f'Unsupported argument type: {type(obj)}')


def table(
    obj: Variable | DataArray | Dataset | dict[str, Variable | DataArray],
    max_rows: int = 20,
) -> Any:
    """Create an HTML table from the contents of the supplied object.

    Possible inputs are:
     - Variable
     - DataArray
     - Dataset
     - dict of Variable
     - dict of DataArray

    Inputs must be one-dimensional. Zero-dimensional data members, attributes and
    coordinates are stripped. Zero-dimensional masks are broadcast.

    Parameters
    ----------
    obj:
        Input to be turned into a html table.
    max_rows:
        Maximum number of rows to display.
    """
    ds = _to_dataset(obj)

    if ds.ndim != 1:
        raise ValueError("Table can only be generated for one-dimensional objects.")

    ds = _strip_scalars_and_broadcast_masks(ds)

    # Limit the number of rows to be printed
    size = ds.shape[0]
    if size > max_rows:
        half = int(max_rows / 2)
        indices: list[int | None] | range = [
            *range(half),
            None,
            *range(size - half, size),
        ]
    else:
        indices = range(size)

    bin_edges = _find_bin_edges(ds)

    header = _make_sections_header(ds)
    if len(ds) > 1:
        header = _make_entries_header(ds) + header

    # First attach coords
    body = [
        _make_variable_column(
            name=name,
            var=var,
            indices=indices,
            need_bin_edge=bin_edges,
            is_bin_edge=ds.coords.is_edges(name),
        )
        for name, var in sorted(ds.coords.items())
    ]

    # Rest of the table from DataArrays
    for i, (_, da) in enumerate(sorted(ds.items())):
        body += _make_data_array_table(
            da=da,
            indices=indices,
            bin_edges=bin_edges,
            no_left_border=(i == 0) and (not ds.coords),
        )

    html = _to_html_table(header=header, body=body)
    from IPython.display import HTML

    return HTML(html)  # type: ignore[no-untyped-call]
