# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Igor Gudich & Neil Vaytet

from .config import member_colors
from ._scipp import core as sc


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


def title_to_string(var, name=None, replace_dim=True):
    """
    Make a column title with "Name [unit]"
    """
    if name is not None:
        text = name
    else:
        text = str(var.dims[0])
        if replace_dim:
            text = text.replace("Dim.", "")

    if var.unit != sc.units.dimensionless:
        text += " [{}]".format(var.unit)
    return text


def table_from_dataset(dataset, is_hist=False, headers=2):

    bstyle = "style='border: 1px solid black;"
    cstyle = bstyle + "background-color: #adf3e0;text-align: center;'"
    lstyle = bstyle + "background-color: #d9c0fa;text-align: center;'"
    mstyle = bstyle + "text-align: center;'"
    bstyle += "'"
    estyle = "style='border: 0px solid white;background-color: #ffffff;'"

    # Declare table
    html = "<table style='border-collapse: collapse;'>"
    dims = dataset.dims
    size = coord = None
    if len(dims) > 0:
        # Dataset should contain only one dim, so get the first in list
        size = dataset.shape[0]
        if len(dataset.coords) > 0:
            coord = dataset.coords[dims[0]]
            if is_hist:
                size += 1

    if headers > 1:
        colsp_coord = 0
        if coord is not None:
            colsp_coord = 1 + (coord.variances is not None)
        colsp_labs = 0
        for key, lab in dataset.labels:
            colsp_labs += 1 + (lab.variances is not None)
        colsp_data = 0
        for key, val in dataset:
            colsp_data += 1 + (val.variances is not None)
        html += "<tr>"
        if colsp_coord > 0:
            html += "<th {} colspan='{}'>Coordinate</th>".format(cstyle,
                                                                 colsp_coord)
        if colsp_labs > 0:
            html += "<th {} colspan='{}'>Labels</th>".format(lstyle,
                                                             colsp_labs)
        if colsp_data > 0:
            html += "<th {} colspan='{}'>Data</th>".format(mstyle,
                                                           colsp_data)
        html += "</tr>"

    if headers > 0:
        html += "<tr>"
        if coord is not None:
            html += "<th {} colspan='{}'>{}</th>".format(
                mstyle, 1 + (coord.variances is not None),
                title_to_string(coord, replace_dim=False))
        for key, lab in dataset.labels:
            html += "<th {} colspan='{}'>{}</th>".format(
                mstyle, 1 + (lab.variances is not None),
                title_to_string(lab, name=key))
        for key, val in dataset:
            html += "<th {} colspan='{}'>{}</th>".format(
                mstyle, 1 + (val.variances is not None),
                title_to_string(val, name=key))
        html += "</tr><tr>"
        if coord is not None:
            html += "<th {}>Values</th>".format(mstyle)
            if coord.variances is not None:
                html += "<th {}>Variances</th>".format(mstyle)
        for key, lab in dataset.labels:
            html += "<th {}>Values</th>".format(mstyle)
            if lab.variances is not None:
                html += "<th {}>Variances</th>".format(mstyle)
        for key, val in dataset:
            html += "<th {}>Values</th>".format(mstyle)
            if val.variances is not None:
                html += "<th {}>Variances</th>".format(mstyle)
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
            # Add labels
            for key, lab in dataset.labels:
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
            html += "</tr><tr>"
            # If there are bin edges, we need to add trailing cells for data
            # and labels
            if coord is not None:
                for key, lab in dataset.labels:
                    if len(lab.values) == len(coord.values) - 1:
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
    headers = 0
    is_empty = {}

    tp = type(dataset)

    if (tp is sc.Dataset) or (tp is sc.DatasetProxy):

        headers = 2

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
        headers = 2
        key = dataset.name
        tabledict["default"][key] = dataset.data
        if len(dataset.coords) > 0:
            dim = dataset.dims[0]
            tabledict["default"][key].coords[dim] = dataset.coords[dim]
    elif (tp is sc.Variable) or (tp is sc.VariableProxy):
        headers = 1
        key = str(dataset.dims[0])
        tabledict["default"][key] = dataset
    else:
        tabledict["default"][""] = sc.Variable([sc.Dim.Row], values=dataset)
        headers = 0

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
        output += table_from_dataset(tabledict["0D Variables"],
                                     headers=headers)
        output += "</td></tr>"
    if len(tabledict["1D Variables"].keys()) > 0:
        output += subtitle.format("1D Variables")
        output += whitetd
        for key, val in sorted(tabledict["1D Variables"].items()):
            output += table_from_dataset(val, is_hist=is_histogram[key],
                                         headers=headers)
        output += "</td></tr>"
    output += "</table>"

    display(HTML(output))

    return
