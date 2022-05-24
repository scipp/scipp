# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from ..utils import value_to_string
from .. import DataArray, Dataset


def _string_in_cell(v):
    out = f'<td>{value_to_string(v.value)}'
    if v.variance is not None:
        out += f'&plusmn;{value_to_string(v.variance)}'
    out += '</td>'
    return out


def _make_groups(obj, attrs):
    out = [obj.coords, obj]
    if attrs:
        out.append(attrs)
    return out


def _make_row(obj, attrs, ind=0):

    out = ''
    for group in _make_groups(obj, attrs=attrs):
        for var in group.values():
            out += _string_in_cell(var[0])
            # v = var[0]
            # out += f'<td>{value_to_string(v.value)}'
            # if v.variance is not None:
            #     out += f'&plusmn;{value_to_string(v.variance)}'
            # out += '</td>'
    return out


def _empty_strings_or_values(group):
    # out = {}
    # for key, var in group.items():
    #     if group.is_edges(key):
    #         need_extra_row = True
    #         out[key] = var[-1:]
    #     else:
    #         out[key] = sc.array(dims=var.dims, values=[""])
    # return out
    return [
        _string_in_cell(var[-1]) if group.is_edges(key) else '<td></td>'
        for key, var in group.items()
    ]


def table(obj, max_rows=20):

    out = '<table><tr>'
    attrs = {}

    if isinstance(obj, DataArray):
        attrs = obj.attrs
        obj = Dataset({obj.name: obj})

    if obj.coords:
        out += f'<th colspan="{len(obj.coords)}">Coordinates</th>'

    out += f'<th colspan="{len(obj.keys())}">Data</th>'

    if attrs:
        out += f'<th colspan="{len(attrs)}">Attributes</th>'

    out += '</tr>'

    ncols = 0
    for group in _make_groups(obj, attrs):
        for name in group:
            out += f'<th>{name}</th>'
            ncols += 1

    size = obj.shape[0]
    if size > max_rows:
        half = int(max_rows / 2)
        inds = list(range(half)) + [None] + list(range(size - half, size))
    else:
        inds = range(size)

    for i in inds:
        if i is None:
            out += '<tr>' + ('<td>...</td>' * ncols) + '</tr>'
        else:
            out += f'<tr>{_make_row(obj=obj[i:i+1], attrs=attrs)}</tr>'

    # need_extra_row = False
    # bin_edge_row = ['<td></td>'] * ncols
    empty_cell = '<td></td>'

    bin_edge_coords = _empty_strings_or_values(obj.coords)
    bin_edge_attrs = _empty_strings_or_values(attrs)

    # [
    #     _string_in_cell(var[-1]) if obj.coords.is_edges(key) else '<td></td>'
    #     for key, var in obj.coords.items()
    # ]

    # bin_edge_attrs = [
    #     _string_in_cell(var[-1]) if attrs.is_edges(key) else '<td></td>'
    #     for key, var in attrs.items()
    # ]

    extra_row = "".join(bin_edge_coords + ([empty_cell] * len(obj.keys())) +
                        bin_edge_attrs)

    if len(extra_row) > len(empty_cell) * ncols:
        out += f'<tr>{extra_row}</tr>'

    print(extra_row)

    #     if obj.coords.is_edges(key):
    #         bin_edge_row[ind] = _string_in_cell(var[-1])
    #     ind += 1

    # ind = 0
    # for key, var in obj.coords.items():
    #     if obj.coords.is_edges(key):
    #         bin_edge_row[ind] = _string_in_cell(var[-1])
    #     ind += 1
    # for key, var in obj.coords.items():
    #     if obj.coords.is_edges(key):
    #         bin_edge_row[ind] = _string_in_cell(var[-1])
    #     ind += 1

    # need_extra_row = False
    # for group in (obj.coords, attrs):
    #     for key in group:
    #         if group.is_edges(key):
    #             need_extra_row = True
    # if need_extra_row:
    #     bin_edge = Dataset(data={key: var.data for key, var in last_row.items()})
    #     out += f'<tr>{_make_row(obj=obj[-1:], attrs=attrs, ind=1)}</tr>'

    # #     bin_edge.coords[key] =
    # # , **attrs}.items():

    # # bin_edge_row = False

    # last_row = obj[-1:]
    # bin_edge = Dataset(data={key: var.data for key, var in last_row.items()},
    #     coords=_empty_strings_or_values(last_row.coords))

    # for key, var in last_row.coords.items():
    #     if last_row.coords.is_edges(key):
    #         need_extra_row = True
    #         bin_edge.coords[key] = var[1:]
    #     else:
    #         bin_edge.coords[key] = sc.array(dims=var.dims, values=[""])

    #     bin_edge.coords[key] =
    # , **attrs}.items():

    out += '</table>'

    # return out
    from IPython.display import display, HTML
    display(HTML(out))
