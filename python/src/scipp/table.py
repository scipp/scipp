# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Igor Gudich & Neil Vaytet

import scipp as sc
from .tools import axis_label


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


def table_from_dataset(dataset, is_hist=False, headers=True):

    bstyle = "style='border: 1px solid black;"
    cstyle = bstyle + "background-color: #adf3e0;'"
    lstyle = bstyle + "background-color: #d9c0fa;'"
    bstyle += "'"
    estyle = "style='border: 0px solid white;background-color: #ffffff;'"

    # Declare table
    html = "<table style='border-collapse: collapse;'>"
    dims = dataset.dims
    dataset_dim = size = coord = None
    if len(dims) > 0:
        # Get first key in dict
        dataset_dim = next(iter(dims))
        size = dims[dataset_dim]
        if len(dataset.coords) > 0:
            coord = dataset.coords[dataset_dim]
            if is_hist:
                size += 1

    if headers:
        html += "<tr>"
        if coord is not None:
            html += "<th {} colspan='{}'>Coord: {}</th>".format(
                cstyle.replace("style='", "style='text-align: center;"),
                1 + (coord.variances is not None), axis_label(coord))
        for key, val in dataset:
            html += "<th {} colspan='{}'>{}</th>".format(
                bstyle.replace("style='", "style='text-align: center;"),
                1 + (val.variances is not None), axis_label(val, name=key))
        for key, lab in dataset.labels:
            if lab.dims[0] == dataset_dim:
                html += "<th {}>{}</th>".format(
                    lstyle.replace("style='", "style='text-align: center;"),
                    key)
        html += "</tr><tr>"
        if coord is not None:
            html += "<th {}>Values</th>".format(bstyle)
            if coord.variances is not None:
                html += "<th {}>Variances</th>".format(bstyle)
        for key, val in dataset:
            html += "<th {}>Values</th>".format(bstyle)
            if val.variances is not None:
                html += "<th {}>Variances</th>".format(bstyle)
        for key, lab in dataset.labels:
            if lab.dims[0] == dataset_dim:
                html += "<th {}>Labels</th>".format(bstyle)
        html += "</tr>"
    if size is None:
        html += "<tr>"
        for key, val in dataset:
            html += "<td {}>{}</td>".format(bstyle, value_to_string(val.value))
            if val.variances is not None:
                html += "<td {}>{}</td>".format(bstyle,
                                                value_to_string(val.variance))
        html += "</tr>"
    else:
        for i in range(size):
            html += '<tr style="font-weight:normal">'
            # Add coordinates
            if coord is not None:
                text = value_to_string(coord.values[i])
                html += "<td rowspan='2' {}>{}</td>".format(bstyle, text)
                if coord.variances is not None:
                    text = value_to_string(coord.variances[i])
                    html += "<td rowspan='2' {}>{}</td>".format(bstyle, text)
            # Add data fields
            for key, val in dataset:
                header_line_for_bin_edges = False
                if coord is not None:
                    if len(val.values) == len(coord.values) - 1:
                        header_line_for_bin_edges = True
                if header_line_for_bin_edges:
                    if i == 0:
                        html += "<td {}></td>".format(estyle)
                        if val.variances is not None:
                            html += "<td {}></td>".format(estyle)
                else:
                    html += "<td rowspan='2' {}>{}</td>".format(
                        bstyle, value_to_string(val.values[i]))
                    if val.variances is not None:
                        html += "<td rowspan='2' {}>{}</td>".format(
                            bstyle, value_to_string(val.variances[i]))
            # Add labels
            for key, lab in dataset.labels:
                if lab.dims[0] == dataset_dim:
                    header_line_for_bin_edges = False
                    if coord is not None:
                        if len(lab.values) == len(coord.values) - 1:
                            header_line_for_bin_edges = True
                    if header_line_for_bin_edges:
                        if i == 0:
                            html += "<td {}></td>".format(estyle)
                            if lab.variances is not None:
                                html += "<td {}></td>".format(estyle)
                    else:
                        html += "<td rowspan='2' {}>{}</td>".format(
                            bstyle, value_to_string(lab.values[i]))
                        if lab.variances is not None:
                            html += "<td rowspan='2' {}>{}</td>".format(
                                bstyle, value_to_string(lab.variances[i]))
            html += "</tr><tr>"
            # If there are bin edges, we need to add trailing cells for data
            # and labels
            if coord is not None:
                for key, val in dataset:
                    if len(val.values) == len(coord.values) - 1:
                        if i == size - 1:
                            html += "<td {}></td>".format(estyle)
                            if val.variances is not None:
                                html += "<td {}></td>".format(estyle)
                        else:
                            html += "<td rowspan='2' {}>{}</td>".format(
                                bstyle, value_to_string(val.values[i]))
                            if val.variances is not None:
                                html += "<td rowspan='2' {}>{}</td>".format(
                                    bstyle, value_to_string(val.variances[i]))
                for key, lab in dataset.labels:
                    if lab.dims[0] == dataset_dim and \
                       len(lab.values) == len(coord.values) - 1:
                        if i == size - 1:
                            html += "<td {}></td>".format(estyle)
                            if lab.variances is not None:
                                html += "<td {}></td>".format(estyle)
                        else:
                            html += "<td rowspan='2' {}>{}</td>".format(
                                bstyle, value_to_string(lab.values[i]))
                            if lab.variances is not None:
                                html += "<td rowspan='2' {}>{}</td>".format(
                                    bstyle, value_to_string(lab.variances[i]))
            html += "</tr>"

    html += "</table>"
    return html


def table(dataset):

    from IPython.display import display, HTML

    tabledict = {
        "default": sc.Dataset(),
        "0D Variables": sc.Dataset(),
        "1D Variables": {}
    }
    is_histogram = {}
    headers = True
    is_empty = {}

    tp = type(dataset)

    if (tp is sc.Dataset) or (tp is sc.DatasetProxy):

        # First add one entry per dimension
        for dim in dataset.dims:
            key = str(dim)
            tabledict["1D Variables"][key] = sc.Dataset()
            is_empty[key] = True
            is_histogram[key] = False

        # Next add the variables
        for name, var in dataset:
            if len(var.dims) == 1:
                dim = var.dims[0]
                key = str(dim)
                if len(var.coords) > 0:
                    if len(var.coords[dim].values) == len(var.values) + 1:
                        is_histogram[key] = True
                    tabledict["1D Variables"][key].coords[dim] = \
                        var.coords[dim]
                tabledict["1D Variables"][key][name] = var.data
                is_empty[key] = False

            elif len(var.dims) == 0:
                tabledict["0D Variables"][name] = var

        # Next add only the 1D coordinates
        for dim, var in dataset.coords:
            if len(var.dims) == 1:
                key = str(dim)
                if dim not in tabledict["1D Variables"][key].coords:
                    tabledict["1D Variables"][key].coords[dim] = var
                    is_empty[key] = False

        # Next add the labels
        for name, lab in dataset.labels:
            if len(lab.dims) == 1:
                dim = lab.dims[0]
                key = str(dim)
                if len(dataset.coords) > 0:
                    if len(dataset.coords[dim].values) == len(lab.values) + 1:
                        is_histogram[key] = True
                    tabledict["1D Variables"][key].coords[dim] = \
                        dataset.coords[dim]
                tabledict["1D Variables"][key].labels[name] = lab
                is_empty[key] = False

        # Now purge out the empty entries
        for key, val in is_empty.items():
            if val:
                del(tabledict["1D Variables"][key])

    elif (tp is sc.DataArray) or (tp is sc.DataProxy):
        key = dataset.name
        tabledict["default"][key] = dataset.data
        if len(dataset.coords) > 0:
            dim = dataset.dims[0]
            tabledict["default"][key].coords[dim] = dataset.coords[dim]
    elif (tp is sc.Variable) or (tp is sc.VariableProxy):
        key = str(dataset.dims[0])
        tabledict["default"][key] = dataset
    else:
        tabledict["default"][""] = sc.Variable([sc.Dim.Row], values=dataset)
        headers = False

    subtitle = "<tr><td style='font-weight:normal;color:grey;"
    subtitle += "font-style:italic;background-color:#ffffff;"
    subtitle += "text-align:left;font-size:1.2em;padding: 1px;'>"
    subtitle += "&nbsp;{}</td></tr>"
    whitetd = "<tr><td style='background-color:#ffffff;'>"
    title = str(type(dataset)).replace("<class 'scipp._scipp.core.",
                                       "").replace("'>", "")
    output = "<table style='border: 1px solid black;'><tr>"
    output += "<td style='font-weight:bold;font-size:1.5em;padding:1px;"
    output += "text-align:center;background-color:#ffffff;'>"
    output += "{}</td></tr>".format(title)

    if len(tabledict["default"]) > 0:
        output += whitetd
        output += table_from_dataset(tabledict["default"], headers=headers)
        output += "</td></tr>"
    if len(tabledict["0D Variables"]) > 0:
        output += subtitle.format("0D Variables")
        output += whitetd
        output += table_from_dataset(tabledict["0D Variables"])
        output += "</td></tr>"
    if len(tabledict["1D Variables"].keys()) > 0:
        output += subtitle.format("1D Variables")
        output += whitetd
        for key, val in sorted(tabledict["1D Variables"].items()):
            output += table_from_dataset(val, is_hist=is_histogram[key])
        output += "</td></tr>"
    output += "</table>"

    display(HTML(output))

    return
