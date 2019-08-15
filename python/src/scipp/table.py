# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Igor Gudich

import scipp as sc
from .tools import axis_label
from IPython.display import display, HTML


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


def table_from_dataset(dataset, coord_dim=None, is_hist=False):
    # border style
    bstyle = "style='border: 1px solid black;"
    cstyle = [bstyle + "background-color: #ADF3E0;'",
              bstyle + "background-color: #B9FFEC;'"]
    bstyle += "'"

    # Declare table
    html = "<table style='border-collapse: collapse;'><tr>"
    # Table headers
    if coord_dim is not None:
        coord = dataset.coords[coord_dim]
        html += "<th {} colspan='{}'>Coord: {}</th>".format(
            cstyle[0].replace("style='", "style='text-align: center;"),
            1 + coord.has_variances, axis_label(coord))
    for key, val in dataset:
        if len(val.shape) > 0:
            size = val.shape[0]
        else:
            size = None
        html += "<th {} colspan='{}'>{}</th>".format(
            bstyle.replace("style='", "style='text-align: center;"),
            1 + val.has_variances, axis_label(val, name=key))
    html += "</tr><tr>"
    if coord_dim is not None:
        html += "<th {}>Values</th>".format(cstyle[1])
        if coord.has_variances:
            html += "<th {}>Variances</th>".format(cstyle[1])
    for key, val in dataset:
        html += "<th {}>Values</th>".format(bstyle)
        if val.has_variances:
            html += "<th {}>Variances</th>".format(bstyle)
    html += "</tr>"
    if size is None:
        html += "<tr>"
        for key, val in dataset:
            html += "<td {}>{}</td>".format(bstyle, value_to_string(val.value))
            if val.has_variances:
                html += "<td {}>{}</td>".format(bstyle,
                                                value_to_string(val.variance))
        html += "</tr>"
    else:
        for i in range(size):
            html += "<tr>"
            if coord_dim is not None:
                text = value_to_string(coord.values[i])
                if is_hist:
                    text = '[{}; {}]'.format(
                        text, value_to_string(coord.values[i + 1]))
                html += "<td {}>{}</td>".format(cstyle[i % 2], text)
                if coord.has_variances:
                    text = value_to_string(coord.variances[i])
                    if is_hist:
                        text = '[{}; {}]'.format(
                            text, value_to_string(coord.variances[i + 1]))
                    html += "< {}td>{}</td>".format(cstyle[i % 2], text)
            for key, val in dataset:
                html += "<td {}>{}</td>".format(bstyle,
                                                value_to_string(val.values[i]))
                if val.has_variances:
                    html += "<td {}>{}</td>".format(
                        bstyle, value_to_string(val.variances[i]))
            html += "</tr>"
    html += "</table>"
    return html


def table(dataset):

    title = "<h3>{}</h3>".format(str(type(dataset)).replace(
        "<class 'scipp._scipp.", "").replace("'>", ""))

    tabledict = {"default": sc.Dataset(),
                 "0D Variables": sc.Dataset(),
                 "1D Variables": {}}
    coord_1d = {}
    is_histogram = {}
    coord_def = None

    tp = type(dataset)
    count = 0
    if (tp is sc.Dataset) or (tp is sc.DatasetProxy):
        for name, var in dataset:
            if len(var.dims) == 1:
                key = str(var.dims[0])
                if key not in tabledict["1D Variables"].keys():
                    temp_dict = {"data": {name: dataset[name].data}}
                    if len(var.coords) > 0:
                        coord_1d[key] = var.dims[0]
                        temp_dict["coords"] = {var.dims[0]:
                                               var.coords[var.dims[0]]}
                        if len(var.coords[var.dims[0]].values) == \
                           len(var.values) + 1:
                            is_histogram[key] = True
                        else:
                            is_histogram[key] = False
                    else:
                        coord_1d[key] = None
                    tabledict["1D Variables"][key] = sc.Dataset(**temp_dict)
                else:
                    tabledict["1D Variables"][key][name] = var
            else:
                tabledict["0D Variables"][name] = var
    else:
        try:
            key = dataset.name
        except AttributeError:
            count += 1
            key = "Unnamed variable {}".format(count)
        tabledict["default"][key] = dataset
        if (tp is sc.DataProxy) and (len(dataset.dims) > 0):
            if len(dataset.coords) > 0:
                coord_def = dataset.dims[0]

    subtitle = "<h6 style='font-weight: normal; color: grey'>"
    output = ""
    if len(tabledict["default"]) > 0:
        output += table_from_dataset(tabledict["default"], coord_dim=coord_def)
    if len(tabledict["0D Variables"]) > 0:
        output += subtitle + "0D Variables</h6>"
        output += table_from_dataset(tabledict["0D Variables"])
    if len(tabledict["1D Variables"].keys()) > 0:
        output += subtitle + "1D Variables</h6>"
        for key, val in sorted(tabledict["1D Variables"].items()):
            output += table_from_dataset(val, coord_dim=coord_1d[key],
                                         is_hist=is_histogram[key])

    display(HTML(title + output))

    return
