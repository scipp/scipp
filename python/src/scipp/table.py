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


# def _make_row(coord, dataset, index, size, base_style, edge_style, hover_style):

#     html = '<tr {}>'.format(hover_style)
#     # Add coordinates
#     if coord is not None:
#         text = value_to_string(coord.values[index])
#         html += "<td rowspan='2' {}>{}</td>".format(base_style, text)
#         if coord.variances is not None:
#             text = value_to_string(coord.variances[i])
#             html += "<td rowspan='2' {}>{}</td>".format(
#                 base_style, text)

#     html += _make_value_rows(dataset.labels, coord, index, base_style,
#                              edge_style)
#     html += _make_value_rows(dataset.masks, coord, index, base_style,
#                              edge_style)
#     html += _make_value_rows(dataset, coord, index, base_style, edge_style)

#     html += "</tr><tr {}>".format(hover_style)
#     # If there are bin edges, we need to add trailing cells for data
#     # and labels
#     if coord is not None:
#         html += _make_trailing_cells(dataset.labels, coord, index, size,
#                                      base_style, edge_style)
#         html += _make_trailing_cells(dataset.masks, coord, index, size,
#                                      base_style, edge_style)
#         html += _make_trailing_cells(dataset, coord, index, size,
#                                      base_style, edge_style)
#     html += "</tr>"
#     return html


def table_from_dataset(dataset, is_hist=False, headers=2, row_start=0, max_rows=None):
    base_style = "style='border: 1px solid black; padding: 0px 5px 0px 5px; text-align: right;"

    mstyle = base_style + "text-align: center;"
    vstyle = mstyle + "background-color: #f0f0f0;'"
    mstyle += "'"
    # edge_style = "style='border: 3px solid red;background-color: #ffffff; font-size: xx-small; padding: 0px; height:10px;'"
    edge_style = "style='border: 0px solid white;background-color: #ffffff; height:1.2em;'"
    hover_style = ("onMouseOver=\"this.style.backgroundColor='" +
                   config.colors["hover"] +
                   "'\" onMouseOut=\"this.style.backgroundColor='#ffffff'\"")

    # Declare table
    html = "<table>" # style='border-collapse: collapse;'>"
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
        # print(size, config.table_max_size)
        row_end = min(size, row_start + max_rows)
        for i in range(row_start, row_end):
        # if size > config.table_max_size:
            # for i in range(config.table_max_size // 2):
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
            html += _make_value_rows(dataset, coord, i, base_style, edge_style, row_start)

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
            # html += _make_row(coord, dataset, i, size, base_style, edge_style, hover_style)
        #     for i in range(size - config.table_max_size // 2, size):
        #         html += _make_row(coord, dataset, i, size, base_style, edge_style)
        # else:
        #     for i in range(size):
        #         html += _make_row(coord, dataset, i, size, base_style, edge_style)


        # for i in range(size):
        #     # html += '<tr>'
        #     # # Add coordinates
        #     # if coord is not None:
        #     #     text = value_to_string(coord.values[i])
        #     #     html += "<td rowspan='2' {}>{}</td>".format(base_style, text)
        #     #     if coord.variances is not None:
        #     #         text = value_to_string(coord.variances[i])
        #     #         html += "<td rowspan='2' {}>{}</td>".format(
        #     #             base_style, text)

        #     # html += _make_value_rows(dataset.labels, coord, i, base_style,
        #     #                          edge_style)
        #     # html += _make_value_rows(dataset.masks, coord, i, base_style,
        #     #                          edge_style)
        #     # html += _make_value_rows(dataset, coord, i, base_style, edge_style)

        #     # html += "</tr><tr>"
        #     # # If there are bin edges, we need to add trailing cells for data
        #     # # and labels
        #     # if coord is not None:
        #     #     html += _make_trailing_cells(dataset.labels, coord, i, size,
        #     #                                  base_style, edge_style)
        #     #     html += _make_trailing_cells(dataset.masks, coord, i, size,
        #     #                                  base_style, edge_style)
        #     #     html += _make_trailing_cells(dataset, coord, i, size,
        #     #                                  base_style, edge_style)
        #     # html += "</tr>"
        #     html += _make_row(coord, dataset, i, size, base_style, edge_style)

    html += "</table>"
    return html, size


def table(dataset):

    from IPython.display import display, HTML

    tv = TableViewer(dataset)

    display(tv.box)
    # display(HTML(output))


class TableViewer:

    def __init__(self, dataset):

        # from IPython.display import display, HTML

        # import ipywidgets as widgets

        self.tabledict = {
            "default": sc.Dataset(),
            "0D Variables": sc.Dataset(),
            "1D Variables": {}
        }
        is_histogram = {}
        self.headers = 0
        is_empty = {}

        tp = type(dataset)

        if (tp is sc.Dataset) or (tp is sc.DatasetProxy):

            self.headers = 2

            # First add one entry per dimension
            for dim in dataset.dims:
                key = str(dim)
                self.tabledict["1D Variables"][key] = sc.Dataset()
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
                        if len(dataset.coords[dim].values) == len(lab.values) + 1:
                            is_histogram[key] = True
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
                        if len(dataset.coords[dim].values) == len(mask.values) + 1:
                            is_histogram[key] = True
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
                self.tabledict["default"][key].coords[dim] = dataset.coords[dim]
        elif (tp is sc.Variable) or (tp is sc.VariableProxy):
            self.headers = 1
            key = str(dataset.dims[0])
            self.tabledict["default"][key] = dataset
        else:
            self.tabledict["default"][""] = sc.Variable([sc.Dim.Row], values=dataset)
            self.headers = 0

        # subtitle = "<tr><td style='font-weight:normal;color:grey;"
        subtitle = "<span style='font-weight:normal;color:grey;"
        subtitle += "font-style:italic;background-color:#ffffff;"
        subtitle += "text-align:left;font-size:1.2em;padding: 1px;'>"
        subtitle += "{}</span>"
        whitetd = "<tr><td style='background-color:#ffffff;'>"
        title = str(type(dataset)).replace("<class 'scipp._scipp.core.",
                                           "").replace("'>", "")
        output = "<table style='border: 1px solid black;'><tr>"
        output += "<td style='font-weight:bold;font-size:1.5em;padding:1px;"
        output += "text-align:center;background-color:#ffffff;'>"
        output += "{}</td></tr>".format(title)

        self.box = [widgets.HTML(value="<span style='font-weight:bold;font-size:1.5em;'>{}</span>".format(title))]
        self.tables = {}
        self.sliders = {}
        self.label = widgets.Label(value="rows")
        self.nrows = {}

        if len(self.tabledict["default"]) > 0:
            # output += whitetd
            html, size = table_from_dataset(self.tabledict["default"], headers=self.headers, max_rows=config.table_max_size)
            self.tables["default"] = {" ": widgets.HTML(value=html)}
            hbox = self.make_hbox("default", " ", size)

            # hbox = self.tables["default"]

            # if size is not None:
            #     if size > config.table_max_size:
            #         self.sliders["default"] = widgets.IntSlider(
            #             value=0, min=0, max=size-config.table_max_size, step=1,
            #             description="Row", orientation='vertical', continuous_update=False)
            #         hbox = widgets.HBox([hbox, self.sliders["default"]])
            # self.box.append(hbox)
            self.box.append(hbox)
            # output += "</td></tr>"
        if len(self.tabledict["0D Variables"]) > 0:
            self.tables["0D Variables"] = {}
            # output = "<table style='border: 1px solid black;'><tr>"
            output = subtitle.format("0D Variables")
            # output += whitetd
            html, size = table_from_dataset(self.tabledict["0D Variables"],
                                         headers=self.headers, max_rows=config.table_max_size)
            # output += html
            # output += "</td></tr>"
            self.tables["0D Variables"][" "] = widgets.HTML(value=html)
            # hbox = self.tables["0D Variables"]
            # if size is not None:
            #     if size > config.table_max_size:
            #         self.sliders["0D Variables"] = widgets.IntSlider(
            #             value=0, min=0, max=size-config.table_max_size, step=1,
            #             description="Row", orientation='vertical', continuous_update=False)
            #         hbox = widgets.HBox([hbox, self.sliders["0D Variables"]])
            hbox = self.make_hbox("0D Variables", " ", size)
            self.box.append(widgets.VBox([widgets.HTML(value=output), hbox]))

            # box.append(widgets.HTML(value=output))
        if len(self.tabledict["1D Variables"]) > 0:
            self.tables["1D Variables"] = {}
            # output = "<table style='border: 1px solid black;'><tr>"
            output = subtitle.format("1D Variables")
            # output += whitetd
            self.tabs = widgets.Tab()
            children = []
            for key, val in sorted(self.tabledict["1D Variables"].items()):
                # print(key)
                # children.append(key)
                html, size = table_from_dataset(val,
                                             is_hist=is_histogram[key],
                                             headers=self.headers, max_rows=config.table_max_size)

                # output += html
                self.tables["1D Variables"][key] = widgets.HTML(value=html)
                hbox = self.make_hbox("1D Variables", key, size)
                children.append(hbox)


            # output += "</td></tr>"
            self.tabs.children = children
            for i, key in enumerate(sorted(self.tabledict["1D Variables"].keys())):
                self.tabs.set_title(i, key)
            # self.tables["1D Variables"] = widgets.HTML(value=output)
            # hbox = self.tables["1D Variables"]
            # if size is not None:
            #     if size > config.table_max_size:
            #         self.sliders["1D Variables"] = widgets.IntSlider(
            #             value=0, min=0, max=size-config.table_max_size, step=1,
            #             description="Row", orientation='vertical', continuous_update=False)
            #         hbox = widgets.HBox([hbox, self.sliders["1D Variables"]])
            # hbox = self.make_hbox("1D Variables", size)
            print(output)
            self.box.append(widgets.VBox([widgets.HTML(value=output), self.tabs]))

            # box.append(widgets.HTML(value=output))
        # output += "</table>"
        self.box = widgets.VBox(self.box, layout=widgets.Layout(border="solid 1px", width="auto", display='flex'))
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
    disabled=False, continuous_update=True, layout=widgets.Layout(width='150px')
)
                self.nrows[key].observe(self.update_table, names="value")
                self.sliders[key] = widgets.SelectionSlider(
                    options=np.arange(size-config.table_max_size + 1)[::-1],
                    value=0,
                    description="Row", orientation='vertical', continuous_update=False,
                    layout=widgets.Layout(height='400px'))
                setattr(self.sliders[key], "key", key)
                setattr(self.nrows[key], "key", key)
                setattr(self.sliders[key], "group", group)
                setattr(self.nrows[key], "group", group)
                self.sliders[key].observe(self.update_table, names="value")
                hbox = widgets.HBox([hbox, widgets.VBox([widgets.HBox([self.nrows[key], self.label]), self.sliders[key]])])
        return hbox

    # def update_nrows(self, change):
    #     key = change["owner"].key
    #     self.update_table()


    def update_table(self, change):
        key = change["owner"].key
        group = change["owner"].group
        
        # tp = type(dataset)
        output = "<table style='border: 1px solid black;'><tr>"
        # if isinstance(self.tabledict[key], dict):
        #     html = ""
        #     for name, var in sorted(self.tabledict[key].items()):
        #         html, size = table_from_dataset(var,
        #                                  headers=self.headers, row_start=self.sliders[key].value, max_rows=self.nrows[key].value)
        # else:
        html, size = table_from_dataset(self.tabledict[group][key],
                                     headers=self.headers, row_start=self.sliders[key].value, max_rows=self.nrows[key].value)
        self.tables[group][key].value = html

