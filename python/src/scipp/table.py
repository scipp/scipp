# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Igor Gudich

import scipp as sp
# from xml.etree import ElementTree as et
# from collections import defaultdict
from .tools import axis_label
from IPython.display import display, HTML


style_border_center = {'style': 'border: 1px solid black; text-align:center;'}
style_border_right = {'style': 'border: 1px solid black; text-align:right;'}
style_coord_odd = style_border_center.copy()
style_coord_odd["style"] += 'background-color: #ADF3E0;'
style_coord_even = style_border_center.copy()
style_coord_even["style"] += 'background-color: #B9FFEC;'
style_coord = [style_coord_odd, style_coord_even]


def value_to_string(val, precision=3):
    if (not isinstance(val, float)) or (val == 0):
        text = str(val)
    elif (abs(val) >= 10.0**(precision+1)) or \
         (abs(val) <= 10.0**(-precision-1)):
        text = "{val:.{prec}e}".format(val=val, prec=precision)
    else:
        text = "{}".format(val)
        if len(text) > precision + 2 + (text[0] == '-'):
            text = "{val:.{prec}f}".format(val=val, prec=precision)
    return text


def append_with_text(parent, name, text, attrib=style_border_right):
    el = et.SubElement(parent, name, attrib=attrib)
    el.text = text


def table_from_dataset(dataset, coord_dim=None):
    # Declare table
    html = "<table style='border: 1px solid black;'><tr>"
    # Table headers
    if coord_dim is not None:
        coord = dataset.coords[coord_dim]
        html += "<th colspan='{}'>Coord: {}</th>".format(1 + coord.has_variances, coord_dim)
    for key, val in dataset:
        size = val.shape[0]
        html += "<th colspan='{}'>{}</th>".format(1 + val.has_variances, key)
    html += "</tr><tr>"
    if coord_dim is not None:
        html += "<th>Values</th>"
        if coord.has_variances:
            html += "<th>Variances</th>"
    for key, val in dataset:
        html += "<th>Values</th>"
        if val.has_variances:
            html += "<th>Variances</th>"
    html += "</tr>"
    for i in range(size):
        html += "<tr>"
        if coord_dim is not None:
            html += "<td>{}</td>".format(value_to_string(coord.values[i]))
            if coord.has_variances:
                html += "<td>{}</td>".format(value_to_string(coord.variances[i]))
            # if is_hist:
            #     text = '[{}; {}]'.format(
            #         text, value_to_string(coords1d.values[i + 1]))
        for key, val in dataset:
            html += "<td>{}</td>".format(value_to_string(val.values[i]))
            if val.has_variances:
                html += "<td>{}</td>".format(value_to_string(val.variances[i]))
        html += "</tr>"
    html += "</table>"
    display(HTML(html))





# def variable_column(var, container):
#     container



# def table(dataset):

#     data = dict()
#     for name, var in dataset:
#         vals = []
#         v = var.values
#         for i in range(len(var.values)):
#             vals.append(value_to_string(v[i]))
#         data[name] = ["<b>Values</b>"] + vals

#     html = "<table style='border: 1px solid black;'><tr><th>" + "</th><th>".join(data.keys()) + "</th></tr>"
#     for row in zip(*data.values()):
#         html += '<tr><td>' + '</td><td>'.join(row) + '</td></tr>'
#     html += '</table>'

#     # Render the HTML code
#     from IPython.display import display, HTML
#     display(HTML(html))


def table(dataset):

    return table_from_dataset(dataset)

    # try:
    #     coords = dataset.coords
    # except AttributeError:
    #     coords = None

    # if coords is not None:
    #     ndims = len(coords)
    #     if ndims > 1:
    #         raise RuntimeError("Only 0D & 1D datasets can be rendered as a table")

    tabledict = {"default": [],
                 "0D Variables": {},
                 "1D Variables": {}}

    # datum1d = defaultdict(dict)
    # datum0d = defaultdict(list)
    # datum1d = dict()
    # datum0d = dict()
    # coords1d = None
    tp = type(dataset)
    if tp is sp.Dataset or tp is sp.DatasetProxy or tp is sp.DataProxy:
        for name, var in dataset:
            if len(var.coords) == 1:
                if var.dims[0] not in tabledict["1D Variables"].keys():
                    tabledict["1D Variables"][var.dims[0]] = {name: var}
                else:
                    tabledict["1D Variables"][var.dims[0]][name] = var
                # coords1d = var.coords[var.dims[0]]
            else:
                tabledict["0D Variables"][name] = var
    else:
        tabledict["default"].append(var)

    body = et.Element('body')
    headline = et.SubElement(body, 'h3')
    headline.text = str(type(dataset)).replace("<class 'scipp._scipp.",
                                               "").replace("'>", "")

    # for key, val in tabledict.items():


    # 0 - dimensional data
    if datum0d:
        itab = et.SubElement(body, 'table')
        tab = et.SubElement(itab, 'tbody', attrib=style_border_center)
        cap = et.SubElement(tab, 'caption')
        cap.text = '0D Variables:'
        tr = et.SubElement(tab, 'tr')

        # Data fields
        for key, val in datum0d.items():
            append_with_text(tr, 'th', axis_label(val, name=key),
                             attrib=dict({'colspan':
                                         str(1 + val.has_variances)}.items() |
                                         style_border_center.items()))

        tr = et.SubElement(tab, 'tr')
        # Go through all items in dataset and add headers
        for key, val in datum0d.items():
            append_with_text(
                tr, 'th', "Values", style_border_center)
            if val.has_variances:
                append_with_text(
                    tr, 'th', "Variances", style_border_center)

        tr = et.SubElement(tab, 'tr')
        for key, val in datum0d.items():
            append_with_text(tr, 'td', value_to_string(val.value))
            if val.has_variances:
                append_with_text(tr, 'td', value_to_string(val.variance))

    # 1 - dimensional data
    if datum1d or coords1d:

        itab = et.SubElement(body, 'table')
        tab = et.SubElement(itab, 'tbody', attrib=style_border_center)
        cap = et.SubElement(tab, 'caption')
        cap.text = '1D Variables:'
        tr = et.SubElement(tab, 'tr')

        # Coordinate
        append_with_text(tr, 'th', "Coord: " + axis_label(coords1d),
                         attrib=dict({'colspan':
                                     str(1 + coords1d.has_variances)}.items() |
                                     style_coord[0].items()))

        # Data fields
        for key, val in datum1d.items():
            append_with_text(tr, 'th', axis_label(val, name=key),
                             attrib=dict({'colspan':
                                         str(1 + val.has_variances)}.items() |
                                         style_border_center.items()))
            length = val.shape[0]

        # Check if is histogram
        # TODO: what if the Dataset contains one coordinate with N+1 elements,
        # one variable with N elements, and another variable with N+1 elements.
        # How do we render this as a table if we only have one column for the
        # coordinate?
        is_hist = length == (len(coords1d.values) - 1)

        # Make table row for "Values" and "Variances"
        tr = et.SubElement(tab, 'tr')
        append_with_text(tr, 'th', "Values", style_coord[1])
        if coords1d.has_variances:
            append_with_text(tr, 'th', "Variances", style_coord[1])
        # Go through all items in dataset and add headers
        for key, val in datum1d.items():
            append_with_text(
                tr, 'th', "Values", style_border_center)
            if val.has_variances:
                append_with_text(
                    tr, 'th', "Variances", style_border_center)
        # Now write all the data row by row
        for i in range(length):
            tr = et.SubElement(tab, 'tr')
            text = value_to_string(coords1d.values[i])
            if is_hist:
                text = '[{}; {}]'.format(
                    text, value_to_string(coords1d.values[i + 1]))
            append_with_text(tr, 'td', text, attrib=style_coord[i % 2])
            if coords1d.has_variances:
                text = value_to_string(coords1d.variances[i])
                if is_hist:
                    text = '[{}; {}]'.format(
                        text, value_to_string(coords1d.variances[i + 1]))
                append_with_text(tr, 'td', text, attrib=style_coord[i % 2])
            for key, val in datum1d.items():
                append_with_text(tr, 'td', value_to_string(val.values[i]))
                if val.has_variances:
                    append_with_text(tr, 'td',
                                     value_to_string(val.variances[i]))

    # html = """<html><table border="1">
    # <tr><th>Object</th><th>Good</th><th>Bad</th><th>Ugly</th></tr>"""
    # for fruit in d:
    #     html += "<tr><td>{}</td>".format(fruit)
    #     for state in "good", "bad", "ugly":
    #         html += "<td>{}</td>".format('<br>'.join(f for f in d[fruit] if ".{}.".format(state) in f))
    #     html += "</tr>"
    # html += "</table></html>"

    data = dict()
    for name, var in dataset:
        data[name] = ["Values"] + var.values

    html = '<table><tr><th>' + '</th><th>'.join(data.keys()) + '</th></tr>'
    for row in zip(*data.values()):
        html += '<tr><td>' + '</td><td>'.join(row) + '</td></tr>'
    html += '</table>'

    # Render the HTML code
    from IPython.display import display, HTML
    display(HTML(html))


# def table_var(variable):
#     dims = variable.dims
#     if len(dims) > 1:
#         raise RuntimeError("Only 0D & 1D variables can be rendered as a table")
#     nx = variable.shape[0]

#     body = et.Element('body')
#     headline = et.SubElement(body, 'h3')
#     if isinstance(variable, sp.Variable):
#         headline.text = 'Variable:'
#     else:
#         headline.text = 'VariableProxy:'
#     tab = et.SubElement(body, 'table')

#     tr = et.SubElement(tab, 'tr')
#     append_with_text(tr, 'th', axis_label(variable, name=str(dims[0])),
#                          attrib=dict({'colspan':
#                                      str(1 + variable.has_variances)}.items() |
#                                      style_border_center.items()))

#     # Aligned data
#     tr = et.SubElement(tab, 'tr')

#     append_with_text(
#         tr, 'th', "Values", style_border_center)
#     if variable.has_variances:
#         append_with_text(
#             tr, 'th', "Variances", style_border_center)
#     for i in range(nx):
#         tr_val = et.SubElement(tab, 'tr')
#         append_with_text(tr_val, 'td', value_to_string(variable.values[i]))
#         if variable.has_variances:
#             append_with_text(tr_val, 'td',
#                              value_to_string(variable.variances[i]))

#     from IPython.display import display, HTML
#     display(HTML(et.tostring(body).decode('UTF-8')))


# def table(some):
#     tp = type(some)
#     if tp is sp.Dataset or tp is sp.DatasetProxy or tp is sp.DataProxy:
#         table_ds(some)
#     elif tp is sp.Variable or tp is sp.VariableProxy:
#         table_var(some)
#     else:
#         raise RuntimeError("Type {} is not supported by table "
#                            "output".format(tp))
