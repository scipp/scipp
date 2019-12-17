# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Igor Gudich & Neil Vaytet

from .config import colors
from .utils import value_to_string, name_with_unit
from ._scipp import core as sc


def _make_table_section_name_header(name, section, style):
    """
    Adds a first row of the table that contains the names of the sections
    being displayed in the table. This is usually the first row and will
    contain Data, Labels, Masks, etc.
    """
    col_separators = 0
    for key, sec in section:
        col_separators += 1 + (sec.variances is not None)
    if col_separators > 0:
        return "<th {} colspan='{}'>{}</th>".format(
            style, col_separators, name)
    else:
        return ""


def _make_table_sections(dataset, coord, base_style):
    coord_style = "{} background-color: {};text-align: center;'".format(
        base_style,
        colors.scheme["coord"])
    label_style = "{} background-color: {};text-align: center;'".format(
        base_style,
        colors.scheme["labels"])
    mask_style = "{} background-color: {};text-align: center;'".format(
        base_style,
        colors.scheme["mask"])
    data_style = "{} background-color: {};text-align: center;'".format(
        base_style,
        colors.scheme["data"])

    colsp_coord = 0
    if coord is not None:
        colsp_coord = 1 + (coord.variances is not None)
    html = ["<tr>"]

    if colsp_coord > 0:
        html.append("<th {} colspan='{}'>Coordinate</th>".format(coord_style,
                                                                 colsp_coord))
    html.append(_make_table_section_name_header(
        "Labels", dataset.labels, label_style))
    html.append(_make_table_section_name_header(
        "Masks", dataset.masks, mask_style))
    html.append(_make_table_section_name_header(
        "Data", dataset, data_style))

    html.append("</tr>")

    return "".join(html)


def _make_table_unit_headers(section, text_style):
    """
    Adds a row containing the unit of the section
    """
    html = []
    for key, val in section:
        html.append("<th {} colspan='{}'>{}</th>".format(
            text_style, 1 + (val.variances is not None),
            name_with_unit(val, name=key)))
    return "".join(html)


def _make_table_subsections(section, text_style):
    """
    Adds Value | Variance columns for the section.
    """
    html = []

    for key, val in section:
        html.append("<th {}>Values</th>".format(text_style))
        if val.variances is not None:
            html.append("<th {}>Variances</th>".format(text_style))

    return "".join(html)


def _make_value_rows(section, coord, index, base_style, edge_style):
    html = []
    for key, val in section:
        header_line_for_bin_edges = False
        if coord is not None:
            if len(val.values) == len(coord.values) - 1:
                header_line_for_bin_edges = True
        if header_line_for_bin_edges:
            if index == 0:
                html.append("<td {}></td>".format(edge_style))
                if val.variances is not None:
                    html.append("<td {}></td>".format(edge_style))
        else:
            html.append("<td rowspan='2' {}>{}</td>".format(
                base_style, value_to_string(val.values[index])))
            if val.variances is not None:
                html.append("<td rowspan='2' {}>{}</td>".format(
                    base_style, value_to_string(val.variances[index])))

    return "".join(html)


def _make_trailing_cells(section, coord, index, size, base_style, edge_style):
    html = []
    for key, val in section:
        if len(val.values) == len(coord.values) - 1:
            if index == size - 1:
                html.append("<td {}></td>".format(edge_style))
                if val.variances is not None:
                    html.append("<td {}></td>".format(edge_style))
            else:
                html.append("<td rowspan='2' {}>{}</td>".format(
                    base_style, value_to_string(val.values[index])))
                if val.variances is not None:
                    html.append("<td rowspan='2' {}>{}</td>".format(
                        base_style, value_to_string(val.variances[index])))

    return "".join(html)


def table_from_dataset(dataset, is_hist=False, headers=2):
    base_style = "style='border: 1px solid black;"

    mstyle = base_style + "text-align: center;'"
    edge_style = "style='border: 0px solid white;background-color: #ffffff;'"

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
        html += _make_table_sections(dataset, coord, base_style)

    if headers > 0:
        html += "<tr>"

        if coord is not None:
            html += "<th {} colspan='{}'>{}</th>".format(
                mstyle, 1 + (coord.variances is not None),
                name_with_unit(coord, replace_dim=False))

        html += _make_table_unit_headers(dataset.labels, mstyle)
        html += _make_table_unit_headers(dataset.masks, mstyle)
        html += _make_table_unit_headers(dataset, mstyle)

        html += "</tr><tr>"

        if coord is not None:
            html += "<th {}>Values</th>".format(mstyle)
            if coord.variances is not None:
                html += "<th {}>Variances</th>".format(mstyle)
        html += _make_table_subsections(dataset.labels, mstyle)
        html += _make_table_subsections(dataset.masks, mstyle)
        html += _make_table_subsections(dataset, mstyle)
        html += "</tr>"

    # the base style still does not have a closing quote, so we add it here
    base_style += "'"

    if size is None:  # handle 0D variable
        html += "<tr>"
        for key, val in dataset:
            html += "<td {}>{}</td>".format(base_style,
                                            value_to_string(val.value))
            if val.variances is not None:
                html += "<td {}>{}</td>".format(base_style,
                                                value_to_string(val.variance))
        html += "</tr>"
    else:
        for i in range(size):
            html += '<tr>'
            # Add coordinates
            if coord is not None:
                text = value_to_string(coord.values[i])
                html += "<td rowspan='2' {}>{}</td>".format(base_style, text)
                if coord.variances is not None:
                    text = value_to_string(coord.variances[i])
                    html += "<td rowspan='2' {}>{}</td>".format(
                        base_style, text)

            html += _make_value_rows(
                dataset.labels, coord, i, base_style, edge_style)
            html += _make_value_rows(
                dataset.masks, coord, i, base_style, edge_style)
            html += _make_value_rows(
                dataset, coord, i, base_style, edge_style)

            html += "</tr><tr>"
            # If there are bin edges, we need to add trailing cells for data
            # and labels
            if coord is not None:
                html += _make_trailing_cells(
                    dataset.labels, coord, i, size, base_style, edge_style)
                html += _make_trailing_cells(
                    dataset.masks, coord, i, size, base_style, edge_style)
                html += _make_trailing_cells(
                    dataset, coord, i, size, base_style, edge_style)
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

        # Next add the masks
        for name, mask in dataset.masks:
            if len(mask.dims) == 1:
                dim = mask.dims[0]
                key = str(dim)
                if len(dataset.coords) > 0:
                    if len(dataset.coords[dim].values) == len(mask.values) + 1:
                        is_histogram[key] = True
                    tabledict["1D Variables"][key].coords[dim] = \
                        dataset.coords[dim]
                tabledict["1D Variables"][key].masks[name] = mask
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
