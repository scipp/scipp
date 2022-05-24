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
        out.append(obj[list(obj.keys())[0]].attrs)
    return out


def _make_row(obj, attrs, ind=0):

    out = ''
    for group in _make_groups(obj, attrs=attrs):
        for var in group.values():
            out += _string_in_cell(var[0])
    return out


def _empty_strings_or_values(group):
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
