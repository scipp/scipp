# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Igor Gudich & Neil Vaytet

from . import config
from .utils import value_to_string, name_with_unit
from ._scipp import core as sc

import ipywidgets as widgets
import numpy as np


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
        return "<th {} colspan='{}'>{}</th>".format(style, col_separators,
                                                    name)
    else:
        return ""


def _make_table_sections(dataset, coord, base_style):
    coord_style = "{} background-color: {};text-align: center;'".format(
        base_style, config.colors["coord"])
    label_style = "{} background-color: {};text-align: center;'".format(
        base_style, config.colors["labels"])
    mask_style = "{} background-color: {};text-align: center;'".format(
        base_style, config.colors["mask"])
    data_style = "{} background-color: {};text-align: center;'".format(
        base_style, config.colors["data"])

    colsp_coord = 0
    if coord is not None:
        colsp_coord = 1 + (coord.variances is not None)
    html = ["<tr>"]

    if colsp_coord > 0:
        html.append("<th {} colspan='{}'>Coordinate</th>".format(
            coord_style, colsp_coord))
    html.append(
        _make_table_section_name_header("Labels", dataset.labels, label_style))
    html.append(
        _make_table_section_name_header("Masks", dataset.masks, mask_style))
    html.append(_make_table_section_name_header("Data", dataset, data_style))

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


def _make_value_rows(section, coord, index, base_style, edge_style, row_start):
    html = []
    for key, val in section:
        header_line_for_bin_edges = False
        if coord is not None:
            if len(val.values) == len(coord.values) - 1:
                header_line_for_bin_edges = True
        if header_line_for_bin_edges:
            if index == row_start:
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


def _make_overflow_cells(section, base_style):
    html = []
    for key, val in section:
        html.append("<td {}>...</td>".format(base_style))
        if val.variances is not None:
            html.append("<td {}>...</td>".format(base_style))
    return "".join(html)


def _make_overflow_row(dataset, coord, base_style, hover_style):
    html = ["<tr {}>".format(hover_style)]
    if coord is not None:
        html.append(_make_overflow_cells([(" ", coord)], base_style))
    html.append(_make_overflow_cells(dataset.labels, base_style))
    html.append(_make_overflow_cells(dataset.masks, base_style))
    html.append(_make_overflow_cells(dataset, base_style))
    html.append("</tr>")
    return "".join(html)


def table_from_dataset(dataset,
                       is_hist=False,
                       headers=2,
                       row_start=0,
                       max_rows=None):
    base_style = ("style='border: 1px solid black; padding: 0px 5px 0px 5px; "
                  "text-align: right;")

    mstyle = base_style + "text-align: center;"
    vstyle = mstyle + "background-color: #f0f0f0;'"
    mstyle += "'"
    edge_style = ("style='border: 0px solid white;background-color: #ffffff; "
                  "height:1.2em;'")
    hover_style = ("onMouseOver=\"this.style.backgroundColor='" +
                   config.colors["hover"] +
                   "'\" onMouseOut=\"this.style.backgroundColor='#ffffff'\"")

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
        html += "<tr {}>".format(hover_style)

        if coord is not None:
            html += "<th {} colspan='{}'>{}</th>".format(
                mstyle, 1 + (coord.variances is not None),
                name_with_unit(coord, replace_dim=False))

        html += _make_table_unit_headers(dataset.labels, mstyle)
        html += _make_table_unit_headers(dataset.masks, mstyle)
        html += _make_table_unit_headers(dataset, mstyle)

        html += "</tr><tr {}>".format(hover_style)

        if coord is not None:
            html += "<th {}>Values</th>".format(vstyle)
            if coord.variances is not None:
                html += "<th {}>Variances</th>".format(vstyle)
        html += _make_table_subsections(dataset.labels, vstyle)
        html += _make_table_subsections(dataset.masks, vstyle)
        html += _make_table_subsections(dataset, vstyle)
        html += "</tr>"

    # the base style still does not have a closing quote, so we add it here
    base_style += "'"

    if size is None:  # handle 0D variable
        html += "<tr {}>".format(hover_style)
        for key, val in dataset:
            html += "<td {}>{}</td>".format(base_style,
                                            value_to_string(val.value))
            if val.variances is not None:
                html += "<td {}>{}</td>".format(base_style,
                                                value_to_string(val.variance))
        html += "</tr>"
    else:
        row_end = min(size, row_start + max_rows)
        if row_start > 0:
            html += _make_overflow_row(dataset, coord, base_style, hover_style)
        for i in range(row_start, row_end):
            html += "<tr {}>".format(hover_style)
            # Add coordinates
            if coord is not None:
                text = value_to_string(coord.values[i])
                html += "<td rowspan='2' {}>{}</td>".format(base_style, text)
                if coord.variances is not None:
                    text = value_to_string(coord.variances[i])
                    html += "<td rowspan='2' {}>{}</td>".format(
                        base_style, text)

            html += _make_value_rows(dataset.labels, coord, i, base_style,
                                     edge_style, row_start)
            html += _make_value_rows(dataset.masks, coord, i, base_style,
                                     edge_style, row_start)
            html += _make_value_rows(dataset, coord, i, base_style, edge_style,
                                     row_start)

            html += "</tr><tr {}>".format(hover_style)
            # If there are bin edges, we need to add trailing cells for data
            # and labels
            if coord is not None:
                html += _make_trailing_cells(dataset.labels, coord, i, row_end,
                                             base_style, edge_style)
                html += _make_trailing_cells(dataset.masks, coord, i, row_end,
                                             base_style, edge_style)
                html += _make_trailing_cells(dataset, coord, i, row_end,
                                             base_style, edge_style)
            html += "</tr>"
        if row_end != size:
            html += _make_overflow_row(dataset, coord, base_style, hover_style)

    html += "</table>"
    return html, size


def table(dataset):

    from IPython.display import display

    tv = TableViewer(dataset)

    display(tv.box)


class TableViewer:
    def __init__(self, dataset):

        self.tabledict = {
            "default": sc.Dataset(),
            "0D Variables": sc.Dataset(),
            "1D Variables": {}
        }
        self.is_histogram = {}
        self.headers = 0
        self.trigger_update = True
        is_empty = {}

        tp = type(dataset)

        if (tp is sc.Dataset) or (tp is sc.DatasetProxy):

            self.headers = 2

            # First add one entry per dimension
            for dim in dataset.dims:
                key = str(dim)
                self.tabledict["1D Variables"][key] = sc.Dataset()
                is_empty[key] = True
                self.is_histogram[key] = False

            # Next add the variables
            for name, var in dataset:
                if len(var.dims) == 1:
                    dim = var.dims[0]
                    key = str(dim)
                    if len(var.coords) > 0:
                        if len(var.coords[dim].values) == len(var.values) + 1:
                            self.is_histogram[key] = True
                        self.tabledict["1D Variables"][key].coords[dim] = \
                            var.coords[dim]
                    self.tabledict["1D Variables"][key][name] = var.data
                    is_empty[key] = False

                elif len(var.dims) == 0:
                    self.tabledict["0D Variables"][name] = var

            # Next add only the 1D coordinates
            for dim, var in dataset.coords:
                if len(var.dims) == 1:
                    key = str(dim)
                    if dim not in self.tabledict["1D Variables"][key].coords:
                        self.tabledict["1D Variables"][key].coords[dim] = var
                        is_empty[key] = False

            # Next add the labels
            for name, lab in dataset.labels:
                if len(lab.dims) == 1:
                    dim = lab.dims[0]
                    key = str(dim)
                    if len(dataset.coords) > 0:
                        if len(dataset.coords[dim].values) == len(
                                lab.values) + 1:
                            self.is_histogram[key] = True
                        self.tabledict["1D Variables"][key].coords[dim] = \
                            dataset.coords[dim]
                    self.tabledict["1D Variables"][key].labels[name] = lab
                    is_empty[key] = False

            # Next add the masks
            for name, mask in dataset.masks:
                if len(mask.dims) == 1:
                    dim = mask.dims[0]
                    key = str(dim)
                    if len(dataset.coords) > 0:
                        if len(dataset.coords[dim].values) == len(
                                mask.values) + 1:
                            self.is_histogram[key] = True
                        self.tabledict["1D Variables"][key].coords[dim] = \
                            dataset.coords[dim]
                    self.tabledict["1D Variables"][key].masks[name] = mask
                    is_empty[key] = False

            # Now purge out the empty entries
            for key, val in is_empty.items():
                if val:
                    del (self.tabledict["1D Variables"][key])

        elif (tp is sc.DataArray) or (tp is sc.DataProxy):
            self.headers = 2
            key = dataset.name
            self.tabledict["default"][key] = dataset.data
            if len(dataset.coords) > 0:
                dim = dataset.dims[0]
                self.tabledict["default"][key].coords[dim] = dataset.coords[
                    dim]
        elif (tp is sc.Variable) or (tp is sc.VariableProxy):
            self.headers = 1
            key = str(dataset.dims[0])
            self.tabledict["default"][key] = dataset
            self.is_histogram[key] = False
        else:
            self.headers = 0
            key = " "
            self.tabledict["default"][key] = sc.Variable([sc.Dim.Row],
                                                         values=dataset)
            self.is_histogram[key] = False

        subtitle = "<span style='font-weight:normal;color:grey;"
        subtitle += "font-style:italic;background-color:#ffffff;"
        subtitle += "text-align:left;font-size:1.2em;padding: 1px;'>"
        subtitle += "{}</span>"
        title = str(type(dataset)).replace("<class 'scipp._scipp.core.",
                                           "").replace("'>", "")

        self.box = [
            widgets.HTML(value="<span style='font-weight:bold;"
                         "font-size:1.5em;'>{}</span>".format(title))
        ]
        self.tables = {}
        self.sliders = {}
        self.label = widgets.Label(value="rows")
        self.nrows = {}
        self.sizes = {}

        if len(self.tabledict["default"]) > 0:
            html, size = table_from_dataset(self.tabledict["default"],
                                            headers=self.headers,
                                            max_rows=config.table_max_size)
            self.tables["default"] = {" ": widgets.HTML(value=html)}
            hbox = self.make_hbox("default", " ", size)
            self.box.append(hbox)
            self.sizes["default"] = {" ": size}
        if len(self.tabledict["0D Variables"]) > 0:
            self.tables["0D Variables"] = {}
            output = subtitle.format("0D Variables")
            html, size = table_from_dataset(self.tabledict["0D Variables"],
                                            headers=self.headers,
                                            max_rows=config.table_max_size)
            self.tables["0D Variables"][" "] = widgets.HTML(value=html)
            hbox = self.make_hbox("0D Variables", " ", size)
            self.box.append(widgets.VBox([widgets.HTML(value=output), hbox]))
        if len(self.tabledict["1D Variables"]) > 0:
            self.tables["1D Variables"] = {}
            self.sizes["1D Variables"] = {}
            output = subtitle.format("1D Variables")
            self.tabs = widgets.Tab(layout=widgets.Layout(width="initial"))
            children = []
            for key, val in sorted(self.tabledict["1D Variables"].items()):
                html, size = table_from_dataset(val,
                                                is_hist=self.is_histogram[key],
                                                headers=self.headers,
                                                max_rows=config.table_max_size)
                self.tables["1D Variables"][key] = widgets.HTML(value=html)
                hbox = self.make_hbox("1D Variables", key, size)
                children.append(hbox)
                self.sizes["1D Variables"][key] = size

            self.tabs.children = children
            for i, key in enumerate(
                    sorted(self.tabledict["1D Variables"].keys())):
                self.tabs.set_title(i, key)
            self.box.append(
                widgets.VBox([widgets.HTML(value=output), self.tabs]))

        self.box = widgets.VBox(self.box,
                                layout=widgets.Layout(border="solid 1px",
                                                      width="auto",
                                                      display='flex',
                                                      flex_flow='column'))
        return

    def make_hbox(self, group, key, size):
        hbox = self.tables[group][key]
        if size is not None:
            if size > config.table_max_size:
                self.nrows[key] = widgets.BoundedIntText(
                    value=config.table_max_size,
                    min=1,
                    max=size,
                    step=1,
                    description='Show',
                    disabled=False,
                    continuous_update=True,
                    layout=widgets.Layout(width='150px'))
                self.nrows[key].observe(self.update_slider, names="value")
                self.sliders[key] = widgets.SelectionSlider(
                    options=np.arange(size - self.nrows[key].value + 1)[::-1],
                    value=0,
                    description="Starting row",
                    orientation='vertical',
                    continuous_update=False,
                    layout=widgets.Layout(height='400px'))
                setattr(self.sliders[key], "key", key)
                setattr(self.nrows[key], "key", key)
                setattr(self.sliders[key], "group", group)
                setattr(self.nrows[key], "group", group)
                self.sliders[key].observe(self.update_table, names="value")
                hbox = widgets.HBox([
                    hbox,
                    widgets.VBox([
                        widgets.HBox([self.nrows[key], self.label]),
                        self.sliders[key]
                    ])
                ])
        return hbox

    def update_slider(self, change):
        key = change["owner"].key
        group = change["owner"].group
        val = self.sliders[key].value
        # Prevent update while options are being changed
        self.trigger_update = False
        self.sliders[key].options = np.arange(self.sizes[group][key] -
                                              change["new"] + 1)[::-1]
        # Re-enable table updating
        self.trigger_update = True
        self.sliders[key].value = min(val, self.sliders[key].options[0])

    def update_table(self, change):
        if self.trigger_update:
            key = change["owner"].key
            group = change["owner"].group
            to_table = self.tabledict[group]
            # This extra indexing step comes from the fact that for the
            # default key, the tabledict is a Dataset, while for the 1D
            # Variables it is a dict of Datasets.
            if group != "default":
                to_table = to_table[key]
            html, size = table_from_dataset(to_table,
                                            is_hist=self.is_histogram[key],
                                            headers=self.headers,
                                            row_start=self.sliders[key].value,
                                            max_rows=self.nrows[key].value)
            self.tables[group][key].value = html
