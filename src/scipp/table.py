# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Igor Gudich & Neil Vaytet

from __future__ import annotations
from html import escape

# Scipp imports
from . import config
from . import utils as su
from ._scipp import core as sc
from .typing import is_scalar, VariableLike
from .html.resources import load_style


def _make_table_sections(dict_of_variables):

    html = ["<tr>"]
    for key, section in dict_of_variables.items():
        heading = key[0].upper() + key[1:]
        col_separators = 0
        for var in section.values():
            col_separators += 1 + (var.variances is not None)
        if col_separators > 0:
            html.append("<th {} colspan='{}'>{}</th>".format(
                f'class="sc-table sc-{key}"', col_separators, heading))
    html.append("</tr>")

    return "".join(html)


def _make_table_unit_headers(dict_of_variables):
    """
    Adds a row containing the unit of the section
    """
    html = []
    for key, section in dict_of_variables.items():
        for name, val in section.items():
            html.append("<th class='sc-units' colspan='{}'>{}</th>".format(
                1 + (val.variances is not None),
                escape(su.name_with_unit(val, name=name))))
    return "".join(html)


def _make_table_subsections(dict_of_variables, plural):
    """
    Adds Value | Variance columns for the section.
    """
    s = "s" if plural else ""
    html = []
    for key, section in dict_of_variables.items():
        for name, val in section.items():
            html.append(f"<th class='sc-subheader'>Value{s}</th>")
            if val.variances is not None:
                html.append(f"<th class='sc-subheader'>Variance{s}</th>")
    return "".join(html)


def _make_value_rows(dict_of_variables, is_bin_centers, index, row_start):
    html = []
    for key, section in dict_of_variables.items():
        for name, val in section.items():
            if is_bin_centers[key][name]:
                if index == row_start:
                    html.append("<td class='sc-padding'></td>")
                    if val.variances is not None:
                        html.append("<td class='sc-padding'></td>")
            else:
                html.append("<td rowspan='2'>{}</td>".format(
                    escape(su.value_to_string(val.values[index]))))
                if val.variances is not None:
                    html.append("<td rowspan='2'>{}</td>".format(
                        escape(su.value_to_string(val.variances[index]))))

    return "".join(html)


def _make_trailing_cells(dict_of_variables, is_bin_centers, index, size):
    html = []
    for key, section in dict_of_variables.items():
        for name, val in section.items():
            if is_bin_centers[key][name]:
                if index == size - 1:
                    html.append("<td class='sc-padding'></td>")
                    if val.variances is not None:
                        html.append("<td class='sc-padding'></td>")
                else:
                    html.append("<td rowspan='2'>{}</td>".format(
                        escape(su.value_to_string(val.values[index]))))
                    if val.variances is not None:
                        html.append("<td rowspan='2'>{}</td>".format(
                            escape(su.value_to_string(val.variances[index]))))

    return "".join(html)


def _make_overflow_row(dict_of_variables):
    html = ["<tr>"]
    for key, section in dict_of_variables.items():
        for name, val in section.items():
            html.append("<td>...</td>")
            if val.variances is not None:
                html.append("<td>...</td>")

    html.append("</tr>")
    return "".join(html)


def _table_from_dict_of_variables(dict_of_variables,
                                  is_bin_centers=None,
                                  size=None,
                                  headers=2,
                                  row_start=0,
                                  max_rows=None,
                                  group=None):
    # Declare table
    html = "<table class='sc-table'>"

    if headers > 1:
        html += _make_table_sections(dict_of_variables)
    if headers > 0:
        html += "<tr>"
        html += _make_table_unit_headers(dict_of_variables)
        html += "</tr><tr>"
        html += _make_table_subsections(dict_of_variables, group == "1D Variables")
        html += "</tr>"

    if size is None:  # handle 0D variable
        html += "<tr>"
        for key, section in dict_of_variables.items():
            for name, val in section.items():
                html += "<td>{}</td>".format(escape(su.value_to_string(val.value)))
                if val.variances is not None:
                    html += "<td>{}</td>".format(
                        escape(su.value_to_string(val.variance)))
        html += "</tr>"
    else:
        row_end = min(size, row_start + max_rows)
        # If we are not starting at the first row, add overflow
        if row_start > 0:
            html += _make_overflow_row(dict_of_variables)
        for i in range(row_start, row_end):
            html += "<tr>"
            html += _make_value_rows(dict_of_variables, is_bin_centers, i, row_start)
            html += "</tr><tr>"
            # If there are bin edges, we need to add trailing cells
            html += _make_trailing_cells(dict_of_variables, is_bin_centers, i, row_end)
            html += "</tr>"
        # If we are not ending at the last row, add overflow
        if row_end != size:
            html += _make_overflow_row(dict_of_variables)

    html += "</table>"
    return html


def _is_bin_centers(container, var, dim):
    """
    Is var considered to be bin_centers in dimension dim
    Considers all meta including non-dimensional meta

    :param container: scipp object defining all meta
    :param var: variable to determine if bin centers
    :param dim: dimension to consider
    :return: True only if var is bin centers
    """
    largest = [c.shape[0] for _, c in container.meta.items() if dim in c.dims]
    return max(largest) == var.shape[0] + 1 if len(largest) > 0 else False


def table(scipp_obj: VariableLike):
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

        if isinstance(scipp_obj, (sc.DataArray, sc.Dataset)):
            if isinstance(scipp_obj, sc.Dataset):
                iterlist = scipp_obj
                tag_names = ['coords', 'data']
                tag_keys = [scipp_obj.coords, iterlist]
            else:
                iterlist = {scipp_obj.name: scipp_obj}
                tag_names = ['coords', 'attrs', 'masks', 'data']
                tag_keys = [
                    scipp_obj.coords, scipp_obj.attrs, scipp_obj.masks, iterlist
                ]
            self.headers = len(tag_names)
            for tag, cat in zip(tag_names, tag_keys):
                for name, var in cat.items():
                    ndims = len(var.dims)
                    name_str = str(name)
                    if ndims < 2:
                        group = "{}D Variables".format(ndims)
                        dim = var.dims[0] if ndims == 1 else ''
                        key = str(dim)
                        if key not in self.tabledict[group]:
                            self.tabledict[group][key] = self.make_dict()
                            self.is_bin_centers[group][key] = self.make_dict()
                            self.sizes[group][key] = []
                        self.is_bin_centers[group][key][tag][name_str] = False
                        self.is_bin_centers[group][key][tag][
                            name_str] = _is_bin_centers(scipp_obj, var, dim)
                        self.tabledict[group][key][tag][name_str] = var
                        self.sizes[group][key].append(
                            var.shape[0] if ndims > 0 else None)

        else:
            ndims = len(scipp_obj.shape)
            if ndims < 2:
                group = "{}D Variables".format(ndims)
                if isinstance(scipp_obj, sc.Variable):
                    self.headers = 1
                    key = '' if is_scalar(scipp_obj) else str(scipp_obj.dims[0])
                    var = scipp_obj
                else:
                    self.headers = 0
                    key = " "
                    var = sc.Variable(dims=['row'], values=scipp_obj)

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

        title = str(type(scipp_obj)).replace("<class '",
                                             "").replace("scipp._scipp.core.",
                                                         "").replace("'>", "")

        self.box = [
            self.widgets.HTML(value=load_style()),
            self.widgets.HTML(value=f"<span class='sc-title'>{title}</span>")
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
                output = f"<span class='sc-subtitle'>{group}</span>"

                children = []
                for key, val in sorted(self.tabledict[group].items()):
                    html = _table_from_dict_of_variables(
                        val,
                        is_bin_centers=self.is_bin_centers[group][key],
                        size=self.sizes[group][key],
                        headers=self.headers,
                        max_rows=config.table_max_size,
                        group=group)
                    self.tables[group][key] = self.widgets.HTML(value=html)
                    hbox = self.make_hbox(group, key, self.sizes[group][key])
                    children.append(hbox)

                vbox = [self.widgets.HTML(value=output)]

                if group == "1D Variables":
                    self.tabs = self.widgets.Tab(layout=self.widgets.Layout(
                        width="initial"))
                    self.tabs.children = children
                    for i, key in enumerate(sorted(self.tabledict[group].keys())):
                        self.tabs.set_title(i, key)
                    vbox.append(self.tabs)
                else:
                    vbox.append(hbox)

                self.box.append(self.widgets.VBox(vbox))

        self.box = self.widgets.VBox(self.box,
                                     layout=self.widgets.Layout(border="solid 1px",
                                                                width="auto",
                                                                display='flex',
                                                                flex_flow='column'))
        self.box.add_class('sc-root')
        return

    def make_dict(self):
        return {"coords": {}, "data": {}, "masks": {}, "attrs": {}}

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
                    self.widgets.VBox([
                        self.widgets.HBox([self.nrows[key], self.label]),
                        self.sliders[key], self.readouts[key]
                    ],
                                      layout=self.widgets.Layout(align_items="center"))
                ])
        return hbox

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
        html = _table_from_dict_of_variables(
            self.tabledict[group][key],
            is_bin_centers=self.is_bin_centers[group][key],
            size=self.sizes[group][key],
            headers=self.headers,
            row_start=index,
            max_rows=self.nrows[key].value,
            group=group)
        self.tables[group][key].value = html
