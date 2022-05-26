# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

import numpy as np
from typing import Any, List, Optional
from .. import DataArray, Dataset, Variable
from ..typing import MetaDataMap, VariableLike
from .. import DType

CENTER = 'style="text-align: center;"'
WITH_BORDER = 'style="border-left:1px solid #a9a9a9;"'
CENTER_BORDER = 'style="text-align: center; border-left:1px solid #a9a9a9;"'


def _string_in_cell(v: Variable) -> str:

    if v.bins is not None:
        return f'len={v.value.shape}'
    if v.dtype in (DType.vector3, DType.string):
        return str(v.value)
    if (v.variance is None) or (v.variance == 0):
        return str(round(v.value, 3))
    err = np.sqrt(v.variance)
    prec = -int(np.floor(np.log10(err)))
    v_str = round(v.value, prec)
    e_str = round(err, prec)
    return f'{v_str}&plusmn;{e_str}'


# def _make_groups(obj: Dataset, attrs: MetaDataMap) -> List[Any]:

#     out = [obj.coords, obj]
#     if attrs:
#         out.append(obj[list(obj.keys())[0]].attrs)
#     return out

# def _make_row(obj: Dataset, attrs: MetaDataMap) -> str:

#     out = ''
#     for group in _make_groups(obj, attrs=attrs):
#         for var in group.values():
#             out += _string_in_cell(var[0])
#     return out

# def _empty_strings_or_values(group: MetaDataMap) -> List[str]:

#     return [
#         _string_in_cell(var[-1]) if group.is_edges(key) else '<td></td>'
#         for key, var in group.items()
#     ]


def _var_name_with_unit(name, var):
    out = f'<span style="font-weight: bold;">{name}</span>'
    unit = var.bins.unit if var.bins is not None else var.unit
    if unit is not None:
        out += ' [𝟙]' if unit == 'dimensionless' else f' [{unit}]'
    return out


def _add_td_tags(cell_list, border=False):
    td = WITH_BORDER if border else ""
    td = f'<td {td}>'
    # print(td)
    return [f'{td}{cell}</td>' for cell in cell_list]


def _make_variable_column(name, var, indices, need_bin_edge, is_bin_edge, border=False):
    out = [_var_name_with_unit(name, var)]
    for i in indices:
        if i is None:
            out.append('...')
        else:
            out.append(_string_in_cell(var[i]))
    if need_bin_edge:
        if is_bin_edge:
            out.append(_string_in_cell(var[-1]))
        else:
            out.append('')
    # print(out)
    return _add_td_tags(out, border=border)


# def _make_coords_table(coords, indices):

#     return [_make_variable_column(name, var, indices) for name, var in coords]


def _make_data_array_table(da, indices, bin_edges):
    # out = []
    # for i in indices:
    #     if i is None:
    #         out.append('...')
    #     else:
    #         out.append(_string_in_cell(var[i]))
    # return out
    # out = []
    # for maybe_bin_edge, group in zip((False, {
    #         '': da.data
    # }), (False, da.masks), (True, da.attrs)):

    out = [
        _make_variable_column(name='',
                              var=da.data,
                              indices=indices,
                              need_bin_edge=bin_edges,
                              is_bin_edge=False,
                              border=True)
    ]

    for name, var in sorted(da.masks.items()):
        out.append(
            _make_variable_column(name=name,
                                  var=var,
                                  indices=indices,
                                  need_bin_edge=bin_edges,
                                  is_bin_edge=False,
                                  border=False))

    for name, var in sorted(da.attrs.items()):
        out.append(
            _make_variable_column(name=name,
                                  var=var,
                                  indices=indices,
                                  need_bin_edge=bin_edges,
                                  is_bin_edge=da.attrs.is_edges(name),
                                  border=False))

    # print(out)

    # for maybe_bin_edge, group in zip((False, False, True), ({
    #         '': da.data
    # }, da.masks, da.attrs)):
    #     out += [
    #         _make_variable_column(
    #             name=name,
    #             var=var,
    #             indices=indices,
    #             need_bin_edge=bin_edges,
    #             is_bin_edge=group.is_edges(name) if maybe_bin_edge else False)
    #         for name, var in sorted(group.items())
    #     ]
    # # if da.coords:
    # #     out += [_make_variable_column(name, var, indices) for name, var in da.coords]
    # # if da.masks:
    # #     out += [_make_variable_column(name, var, indices) for name, var in da.masks]
    # # if da.coords:
    # #     out += [_make_variable_column(name, var, indices) for name, var in da.coords]
    return out


def _make_entries_header(ds):
    out = '<tr>'
    if ds.coords:
        out += f'<th colspan="{len(ds.coords)}"></th>'
    for name, da in sorted(ds.items()):
        ncols = 1 + len(da.masks) + len(da.attrs)
        out += f'<th {CENTER} colspan="{ncols}">{name}</th>'
    out += '</tr>'
    return out


def _make_sections_header(ds):
    out = '<tr>'
    if ds.coords:
        out += f'<th {CENTER} colspan="{len(ds.coords)}">Coordinates</th>'
    for _, da in sorted(ds.items()):
        out += f'<th {CENTER_BORDER}>Data</th>'
        if da.masks:
            out += f'<th {CENTER} colspan="{len(da.masks)}">Masks</th>'
        if da.attrs:
            out += f'<th {CENTER} colspan="{len(da.attrs)}">Attributes</th>'
    out += '</tr>'
    return out


# Coordinates | Data | Attrs | Masks
# x  y  z     |      | a b   | m1 m2
# 0  2  1     | 4+-5 | 1 3   | T  F


def _to_html_table(header, body):
    out = '<table>' + header
    ncols = len(body)
    nrows = len(body[0])
    # print("HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH")
    for i in range(nrows):
        # print('<tr>' + ''.join([body[j][i] for j in range(ncols)]) + '</tr>')
        # print(body[j][i])
        out += '<tr>' + ''.join([body[j][i] for j in range(ncols)]) + '</tr>'
    out += '</table>'
    return out


def _find_bin_edges(ds):
    for key in ds.coords:
        if ds.coords.is_edges(key):
            return True
    for da in ds.values():
        for key in da.attrs:
            if da.attrs.is_edges(key):
                return True
    return False


def _to_dataset(obj):
    if isinstance(obj, DataArray):
        # attrs = obj.attrs
        return Dataset({obj.name: obj})
    if isinstance(obj, Variable):
        return Dataset(data={"": obj})
    if isinstance(obj, dict):
        return Dataset(obj)
    return obj


def table(obj: VariableLike, max_rows: Optional[int] = 20):

    obj = _to_dataset(obj)

    # Limit the number of rows to be printed
    size = obj.shape[0]
    if size > max_rows:
        half = int(max_rows / 2)
        inds = list(range(half)) + [None] + list(range(size - half, size))
    else:
        inds = range(size)

    # data = [
    #     _make_variable_column(name=name, var=var.data, indices=inds)
    #     for name, var in obj.items()
    # ]

    bin_edges = _find_bin_edges(obj)
    # print("bin_edges", bin_edges)

    header = _make_sections_header(obj)
    if len(obj) > 1:
        header = _make_entries_header(obj) + header
    # print("=======================")
    # print(header)
    # body = _make_data_array_table(obj, indices=inds)

    # First attach coords
    body = [
        _make_variable_column(name=name,
                              var=var,
                              indices=inds,
                              need_bin_edge=bin_edges,
                              is_bin_edge=obj.coords.is_edges(name))
        for name, var in sorted(obj.coords.items())
    ]

    for _, da in sorted(obj.items()):
        body += _make_data_array_table(da=da, indices=inds, bin_edges=bin_edges)
    html = _to_html_table(header=header, body=body)
    from IPython.display import display, HTML
    display(HTML(html))


def oldtable(obj: VariableLike, max_rows: Optional[int] = 20):
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
            out += '</th>'
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
