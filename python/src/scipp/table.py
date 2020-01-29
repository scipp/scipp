# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Igor Gudich & Neil Vaytet

# Scipp imports
from . import config
from .utils import (value_to_string, name_with_unit, is_dataset_or_array,
                    is_dataset, is_variable)
from ._scipp import core as sc


def table(scipp_obj):
    """
    Create a html table from the contents of a Dataset (0D and 1D Variables
    only), DataArray, Variable or raw numpy array.
    The entries will be grouped by dimensions/coordinates.
    """

    from IPython.display import display
    tv = TableViewer(scipp_obj)
    display(tv.box)


class TableViewer:
    def __init__(self, scipp_obj):

        # Delayed import
        self.widgets = __import__("ipywidgets")

        groups = ["0D Variables", "1D Variables"]
        self.tabledict = {}
        self.is_bin_centers = {}
        self.sizes = {}
        for group in groups:
            self.tabledict[group] = {}
            self.is_bin_centers[group] = {}
            self.sizes[group] = {}
        self.headers = 0
        self.trigger_update = True

        if is_dataset_or_array(scipp_obj):

            self.headers = 2

            if is_dataset(scipp_obj):
                iterlist = scipp_obj.items()
            else:
                iterlist = [(scipp_obj.name, scipp_obj)]

            # First add the 1D coordinates
            for dim, var in scipp_obj.coords.items():
                if len(var.dims) == 1:
                    key = str(dim)
                    if key not in self.tabledict["1D Variables"]:
                        self.tabledict["1D Variables"][key] = self.make_dict()
                        self.sizes["1D Variables"][key] = []
                        self.is_bin_centers["1D Variables"][
                            key] = self.make_dict()
                    self.is_bin_centers[group][key]["coords"][key] = False
                    self.tabledict["1D Variables"][key]["coords"][key] = var
                    self.sizes["1D Variables"][key].append(var.shape[0])

            # Next add the labels
            for name, lab in scipp_obj.labels.items():
                ndims = len(lab.dims)
                if ndims < 2:
                    group = "{}D Variables".format(ndims)
                    dim = lab.dims[0] if ndims == 1 else sc.Dim.Invalid
                    key = str(dim)
                    if key not in self.tabledict[group]:
                        self.tabledict[group][key] = self.make_dict()
                        self.is_bin_centers[group][key] = self.make_dict()
                        self.sizes[group][key] = []
                    self.is_bin_centers[group][key]["labels"][name] = False
                    if dim in scipp_obj.coords:
                        if scipp_obj.coords[dim].shape[0] == lab.shape[0] + 1:
                            self.is_bin_centers[group][key]["labels"][
                                name] = True

                    self.tabledict[group][key]["labels"][name] = lab
                    self.sizes[group][key].append(
                        lab.shape[0] if ndims > 0 else None)

            # Next add the masks
            for name, mask in scipp_obj.masks.items():
                ndims = len(mask.dims)
                if ndims < 2:
                    group = "{}D Variables".format(ndims)
                    dim = mask.dims[0] if ndims == 1 else sc.Dim.Invalid
                    key = str(dim)
                    if key not in self.tabledict[group]:
                        self.tabledict[group][key] = self.make_dict()
                        self.is_bin_centers[group][key] = self.make_dict()
                        self.sizes[group][key] = []
                    self.is_bin_centers[group][key]["masks"][name] = False
                    if dim in scipp_obj.coords:
                        if scipp_obj.coords[dim].shape[0] == mask.shape[0] + 1:
                            self.is_bin_centers[group][key]["masks"][
                                name] = True
                    self.tabledict[group][key]["masks"][name] = mask
                    self.sizes[group][key].append(
                        mask.shape[0] if ndims > 0 else None)

            # Next add the data
            for name, var in iterlist:
                ndims = len(var.dims)
                if ndims < 2:
                    group = "{}D Variables".format(ndims)
                    dim = var.dims[0] if ndims == 1 else sc.Dim.Invalid
                    key = str(dim)
                    if key not in self.tabledict[group]:
                        self.tabledict[group][key] = self.make_dict()
                        self.is_bin_centers[group][key] = self.make_dict()
                        self.sizes[group][key] = []
                    self.is_bin_centers[group][key]["data"][name] = False
                    if dim in scipp_obj.coords:
                        if scipp_obj.coords[dim].shape[0] == var.shape[0] + 1:
                            self.is_bin_centers[group][key]["data"][
                                name] = True
                    self.tabledict[group][key]["data"][name] = var.data
                    self.sizes[group][key].append(
                        var.shape[0] if ndims > 0 else None)

        else:

            ndims = len(scipp_obj.shape)
            if ndims < 2:
                group = "{}D Variables".format(ndims)
                if is_variable(scipp_obj):
                    self.headers = 1
                    key = str(scipp_obj.dims[0])
                    var = scipp_obj
                else:
                    self.headers = 0
                    key = " "
                    var = sc.Variable([sc.Dim.Row], values=scipp_obj)

                self.tabledict[group][key] = {"data": {key: var}}
                self.is_bin_centers[group][key] = {"data": {key: False}}
                self.sizes[group][key] = scipp_obj.shape

        # Get max size for each dim
        for group in self.sizes.keys():
            for key in self.sizes[group].keys():
                max_size = 0
                size_is_defined = False
                for s in self.sizes[group][key]:
                    if s is not None:
                        max_size = max(max_size, s)
                        size_is_defined = True
                self.sizes[group][key] = max_size if size_is_defined else None

        subtitle = "<span style='font-weight:normal;color:grey;"
        subtitle += "font-style:italic;background-color:#ffffff;"
        subtitle += "text-align:left;font-size:1.2em;padding: 1px;'>"
        subtitle += "{}</span>"
        title = str(type(scipp_obj)).replace("<class '", "").replace(
            "scipp._scipp.core.", "").replace("'>", "")

        self.box = [
            self.widgets.HTML(value="<span style='font-weight:bold;"
                              "font-size:1.5em;'>{}</span>".format(title))
        ]
        self.tables = {}
        self.sliders = {}
        self.readouts = {}
        self.label = self.widgets.Label(value="rows")
        self.nrows = {}

        # Generate 0D and 1D tables
        for group in self.tabledict.keys():

            if len(self.tabledict[group]) > 0:
                self.tables[group] = {}
                output = subtitle.format(group)

                children = []
                for key, val in sorted(self.tabledict[group].items()):
                    html = self.table_from_dict_of_variables(
                        val,
                        is_bin_centers=self.is_bin_centers[group][key],
                        size=self.sizes[group][key],
                        headers=self.headers,
                        max_rows=config.table_max_size)
                    self.tables[group][key] = self.widgets.HTML(value=html)
                    hbox = self.make_hbox(group, key, self.sizes[group][key])
                    children.append(hbox)

                vbox = [self.widgets.HTML(value=output)]

                if group == "1D Variables":
                    self.tabs = self.widgets.Tab(layout=self.widgets.Layout(
                        width="initial"))
                    self.tabs.children = children
                    for i, key in enumerate(
                            sorted(self.tabledict[group].keys())):
                        self.tabs.set_title(i, key)
                    vbox.append(self.tabs)
                else:
                    vbox.append(hbox)

                self.box.append(self.widgets.VBox(vbox))

        self.box = self.widgets.VBox(self.box,
                                     layout=self.widgets.Layout(
                                         border="solid 1px",
                                         width="auto",
                                         display='flex',
                                         flex_flow='column'))
        return

    def make_dict(self):
        return {"coords": {}, "data": {}, "labels": {}, "masks": {}}

    def make_hbox(self, group, key, size):
        hbox = self.tables[group][key]
        if size is not None:
            if size > config.table_max_size:
                self.nrows[key] = self.widgets.BoundedIntText(
                    value=config.table_max_size,
                    min=1,
                    max=size,
                    step=1,
                    description='Show',
                    disabled=False,
                    continuous_update=True,
                    layout=self.widgets.Layout(width='150px'))
                self.nrows[key].observe(self.update_slider, names="value")
                slider_max = size - self.nrows[key].value + 1
                self.sliders[key] = self.widgets.IntSlider(
                    min=0,
                    max=slider_max,
                    value=slider_max,
                    description="Starting row",
                    orientation='vertical',
                    continuous_update=True,
                    readout=False,
                    layout=self.widgets.Layout(height='400px'))
                self.readouts[key] = self.widgets.Label(
                    value=str(self.sliders[key].max - self.sliders[key].value))
                setattr(self.sliders[key], "key", key)
                setattr(self.nrows[key], "key", key)
                setattr(self.sliders[key], "group", group)
                setattr(self.nrows[key], "group", group)
                self.sliders[key].observe(self.update_table, names="value")
                hbox = self.widgets.HBox([
                    hbox,
                    self.widgets.VBox(
                        [
                            self.widgets.HBox([self.nrows[key], self.label]),
                            self.sliders[key], self.readouts[key]
                        ],
                        layout=self.widgets.Layout(align_items="center"))
                ])
        return hbox

    def make_table_sections(self, dict_of_variables, base_style):

        html = ["<tr>"]
        for key, section in dict_of_variables.items():

            col_separators = 0
            for var in section.values():
                col_separators += 1 + (var.variances is not None)
            if col_separators > 0:
                style = "{} background-color: {};text-align: center;'".format(
                    base_style, config.colors[key])
                html.append("<th {} colspan='{}'>{}</th>".format(
                    style, col_separators, key))

        html.append("</tr>")

        return "".join(html)

    def make_table_unit_headers(self, dict_of_variables, text_style):
        """
        Adds a row containing the unit of the section
        """
        html = []
        for key, section in dict_of_variables.items():
            for name, val in section.items():
                html.append("<th {} colspan='{}'>{}</th>".format(
                    text_style, 1 + (val.variances is not None),
                    name_with_unit(val, name=name)))
        return "".join(html)

    def make_table_subsections(self, dict_of_variables, text_style):
        """
        Adds Value | Variance columns for the section.
        """
        html = []
        for key, section in dict_of_variables.items():
            for name, val in section.items():
                html.append("<th {}>Values</th>".format(text_style))
                if val.variances is not None:
                    html.append("<th {}>Variances</th>".format(text_style))
        return "".join(html)

    def make_value_rows(self, dict_of_variables, is_bin_centers, index,
                        base_style, edge_style, row_start):
        html = []
        for key, section in dict_of_variables.items():
            for name, val in section.items():
                if is_bin_centers[key][name]:
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

    def make_trailing_cells(self, dict_of_variables, is_bin_centers, index,
                            size, base_style, edge_style):
        html = []
        for key, section in dict_of_variables.items():
            for name, val in section.items():
                if is_bin_centers[key][name]:
                    if index == size - 1:
                        html.append("<td {}></td>".format(edge_style))
                        if val.variances is not None:
                            html.append("<td {}></td>".format(edge_style))
                    else:
                        html.append("<td rowspan='2' {}>{}</td>".format(
                            base_style, value_to_string(val.values[index])))
                        if val.variances is not None:
                            html.append("<td rowspan='2' {}>{}</td>".format(
                                base_style,
                                value_to_string(val.variances[index])))

        return "".join(html)

    def make_overflow_row(self, dict_of_variables, base_style, hover_style):
        html = ["<tr {}>".format(hover_style)]
        for key, section in dict_of_variables.items():
            for name, val in section.items():
                html.append("<td {}>...</td>".format(base_style))
                if val.variances is not None:
                    html.append("<td {}>...</td>".format(base_style))

        html.append("</tr>")
        return "".join(html)

    def table_from_dict_of_variables(
            self,
            dict_of_variables,
            is_bin_centers=None,
            # dim_key=None,
            size=None,
            headers=2,
            row_start=0,
            max_rows=None):
        base_style = (
            "style='border: 1px solid black; padding: 0px 5px 0px 5px; "
            "text-align: right;")

        mstyle = base_style + "text-align: center;"
        vstyle = mstyle + "background-color: #f0f0f0;'"
        mstyle += "'"
        edge_style = (
            "style='border: 0px solid white;background-color: #ffffff; "
            "height:1.2em;'")
        hover_style = (
            "onMouseOver=\"this.style.backgroundColor='" +
            config.colors["hover"] +
            "'\" onMouseOut=\"this.style.backgroundColor='#ffffff'\"")

        # Declare table
        html = "<table style='border-collapse: collapse;'>"

        if headers > 1:
            html += self.make_table_sections(dict_of_variables, base_style)
        if headers > 0:
            html += "<tr {}>".format(hover_style)
            html += self.make_table_unit_headers(dict_of_variables, mstyle)
            html += "</tr><tr {}>".format(hover_style)
            html += self.make_table_subsections(dict_of_variables, vstyle)
            html += "</tr>"

        # the base style still does not have a closing quote, so we add it here
        base_style += "'"

        if size is None:  # handle 0D variable
            html += "<tr {}>".format(hover_style)
            for key, section in dict_of_variables.items():
                for name, val in section.items():
                    html += "<td {}>{}</td>".format(base_style,
                                                    value_to_string(val.value))
                    if val.variances is not None:
                        html += "<td {}>{}</td>".format(
                            base_style, value_to_string(val.variance))
            html += "</tr>"
        else:
            row_end = min(size, row_start + max_rows)
            # If we are not starting at the first row, add overflow
            if row_start > 0:
                html += self.make_overflow_row(dict_of_variables, base_style,
                                               hover_style)
            for i in range(row_start, row_end):
                html += "<tr {}>".format(hover_style)
                html += self.make_value_rows(dict_of_variables, is_bin_centers,
                                             i, base_style, edge_style,
                                             row_start)
                html += "</tr><tr {}>".format(hover_style)
                # If there are bin edges, we need to add trailing cells
                html += self.make_trailing_cells(dict_of_variables,
                                                 is_bin_centers, i, row_end,
                                                 base_style, edge_style)
                html += "</tr>"
            # If we are not ending at the last row, add overflow
            if row_end != size:
                html += self.make_overflow_row(dict_of_variables, base_style,
                                               hover_style)

        html += "</table>"
        return html

    def update_slider(self, change):
        key = change["owner"].key
        group = change["owner"].group
        self.sliders[key].max = self.sizes[group][key] - change["new"] + 1
        self.update_table(change)

    def update_table(self, change):
        key = change["owner"].key
        group = change["owner"].group
        index = self.sliders[key].max - self.sliders[key].value
        self.readouts[key].value = str(index)
        html = self.table_from_dict_of_variables(
            self.tabledict[group][key],
            is_bin_centers=self.is_bin_centers[group][key],
            size=self.sizes[group][key],
            headers=self.headers,
            row_start=index,
            max_rows=self.nrows[key].value)
        self.tables[group][key].value = html
