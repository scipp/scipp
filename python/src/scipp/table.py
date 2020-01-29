# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Igor Gudich & Neil Vaytet

# Scipp imports
from . import config
from . import detail
from .utils import value_to_string, name_with_unit, is_dataset_or_array, is_dataset, is_variable
from ._scipp import core as sc

# Other imports
import numpy as np





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

        self.widgets = __import__("ipywidgets")

        default_key = " "
        self.tabledict = {
            "default": {},
            "0D Variables": {},
            "1D Variables": {}
        }
        self.is_histogram = self.tabledict.copy()
        empty_dict = {"coord": {}, "data": {}, "labels": {}, "masks": {}}
        # for key in self.tabledict:
        #     self.is_histogram[key] = {"data": {}, "labels": {}, "masks": {}}
        self.headers = 0
        self.trigger_update = True
        is_empty = {}

        # tp = type(scipp_obj)

        if is_dataset_or_array(scipp_obj):

        # if (tp is sc.Dataset) or (tp is sc.DatasetProxy):

            self.headers = 2

            # # First add one entry per dimension
            # for dim in dataset.dims:
            #     key = str(dim)
            #     self.tabledict["1D Variables"][key] = {}
            #     is_empty[key] = True
            #     self.is_histogram[key] = False
            # if (tp is sc.Dataset) or (tp is sc.DatasetProxy):
            if is_dataset(scipp_obj):
                iterlist = scipp_obj.items()
            else:
                iterlist = [(scipp_obj.name, scipp_obj)]

            # First add the 1D coordinates
            for dim, var in scipp_obj.coords.items():
                if len(var.dims) == 1:
                    key = str(dim)
                    if key not in self.tabledict["1D Variables"]:
                        self.tabledict["1D Variables"][key] = empty_dict.copy()
                        # self.tabledict["1D Variables"][key]["dim"] = key
                    self.tabledict["1D Variables"][key]["coord"][key] = var
                        # is_empty[key] = False

            # Next add the labels
            for name, lab in scipp_obj.labels.items():
                ndims = len(lab.dims)
                if ndims < 2:
                    group = "{}D Variables".format(ndims)
                    dim = lab.dims[0] if ndims == 1 else sc.Dim.Invalid

                    # if len(lab.dims) == 1:
                    #     dim = lab.dims[0]
                    # else:
                    #     dim = sc.Dim.Invalid

                    key = str(dim)
                    if key not in self.tabledict[group]:
                        self.tabledict[group][key] = empty_dict.copy()
                        self.is_histogram[group][key] = empty_dict.copy()
                        # self.tabledict[group][key]["dim"] = key
                    self.is_histogram[group][key]["labels"][name] = False
                    if dim in scipp_obj.coords:
                        if len(scipp_obj.coords[dim].values) == len(
                                lab.values) + 1:
                            self.is_histogram[group][key]["labels"][name] = True

                    
                    self.tabledict[group][key]["labels"][name] = lab

                # if dim in scipp_obj.coords:
                #     if len(scipp_obj.coords[dim].values) == len(
                #             lab.values) + 1:
                #         self.is_histogram["1D Variables"][key]["labels"][name] = True
                #     # self.tabledict["1D Variables"][key].coords[dim] = \
                #     #     dataset.coords[dim]
                # # self.tabledict["1D Variables"][key].labels[name] = lab
                # # is_empty[key] = False


            # Next add the masks
            for name, mask in scipp_obj.masks.items():
                ndims = len(mask.dims)
                if ndims < 2:
                    group = "{}D Variables".format(ndims)
                    dim = mask.dims[0] if ndims == 1 else sc.Dim.Invalid

                    key = str(dim)
                    if key not in self.tabledict[group]:
                        self.tabledict[group][key] = empty_dict.copy()
                        self.is_histogram[group][key] = empty_dict.copy()
                        # self.tabledict[group][key]["dim"] = key
                    self.is_histogram[group][key]["masks"][name] = False
                    if dim in scipp_obj.coords:
                        if len(scipp_obj.coords[dim].values) == len(
                                mask.values) + 1:
                            self.is_histogram[group][key]["masks"][name] = True
                    self.tabledict[group][key]["masks"][name] = mask



            # Next add the data
            for name, var in iterlist:
                ndims = len(var.dims)
                if ndims < 2:
                    group = "{}D Variables".format(ndims)
                    dim = var.dims[0] if ndims == 1 else sc.Dim.Invalid

                    key = str(dim)
                    # print(name, key)
                    # print(self.is_histogram)
                    if key not in self.tabledict[group]:
                        self.tabledict[group][key] = empty_dict.copy()
                        self.is_histogram[group][key] = empty_dict.copy()
                        # self.tabledict[group][key]["dim"] = key
                    self.is_histogram[group][key]["data"][name] = False
                    if dim in scipp_obj.coords:
                        if len(scipp_obj.coords[dim].values) == len(
                                var.values) + 1:
                            self.is_histogram[group][key]["data"][name] = True
                    self.tabledict[group][key]["data"][name] = var.data






        #         if len(var.dims) < 2:

        #             group = "{}D Variables".format(len(var.dims))
        #             if len(var.dims) == 1:
        #                 dim = var.dims[0]
        #                 key = str(dim)
        #             else:
        #                 key = row_key

        #             if key not in self.tabledict[group]:
        #                 self.tabledict[group][key] = {"coord": {}, "data": {}, "labels": {}, "masks": {}}

        #             if len(var.coords) > 0:
        #                 if len(var.coords[dim].values) == len(var.values) + 1:
        #                     self.is_histogram["1D Variables"][key] = True
        #                 # self.tabledict["1D Variables"][key]["coord"][key] = \
        #                 #     var.coords[dim]
        #             self.tabledict["1D Variables"][key]["data"][name] = var.data


        #                 # is_empty[key] = False

        #             elif len(var.dims) == 0:
        #                 if row_key not in self.tabledict["0D Variables"]:
        #                     self.tabledict["1D Variables"][row_key] = {}
        #                 self.tabledict["0D Variables"][row_key][name] = var

        #     # # Next add only the 1D coordinates
        #     # for dim, var in dataset.coords.items():
        #     #     if len(var.dims) == 1:
        #     #         key = str(dim)
        #     #         if dim not in self.tabledict["1D Variables"][key].coords:
        #     #             self.tabledict["1D Variables"][key].coords[dim] = var
        #     #             is_empty[key] = False

        #     # Next add the labels
        #     for name, lab in dataset.labels.items():
        #         if len(lab.dims) == 1:
        #             dim = lab.dims[0]
        #             key = str(dim)
        #             if len(dataset.coords) > 0:
        #                 if len(dataset.coords[dim].values) == len(
        #                         lab.values) + 1:
        #                     self.is_histogram[key] = True
        #                 self.tabledict["1D Variables"][key].coords[dim] = \
        #                     dataset.coords[dim]
        #             self.tabledict["1D Variables"][key].labels[name] = lab
        #             is_empty[key] = False

        #     # # Next add the masks
        #     # for name, mask in dataset.masks.items():
        #     #     if len(mask.dims) == 1:
        #     #         dim = mask.dims[0]
        #     #         key = str(dim)
        #     #         if len(dataset.coords) > 0:
        #     #             if len(dataset.coords[dim].values) == len(
        #     #                     mask.values) + 1:
        #     #                 self.is_histogram[key] = True
        #     #             self.tabledict["1D Variables"][key].coords[dim] = \
        #     #                 dataset.coords[dim]
        #     #         self.tabledict["1D Variables"][key].masks[name] = mask
        #     #         is_empty[key] = False

        #     # # Now purge out the empty entries
        #     # for key, val in is_empty.items():
        #     #     if val:
        #     #         del (self.tabledict["1D Variables"][key])

        # # elif (tp is sc.DataArray) or (tp is sc.DataProxy):
        # #     self.headers = 2
        # #     key = dataset.name
        # #     self.tabledict["default"][row_key] = {key: dataset}
        # #     # if len(dataset.coords) > 0:
        # #     #     dim = dataset.dims[0]
        # #     #     self.tabledict["default"][key].coords[dim] = dataset.coords[
        # #     #         dim]
        elif is_variable(scipp_obj):
            self.headers = 1
            key = str(scipp_obj.dims[0])
            # TODO: does moving here invalidate the input?
            self.tabledict["default"][row_key] = {key: detail.move_to_data_array(data=scipp_obj)}
            self.is_histogram[key] = False
        else:
            self.headers = 0
            key = " "
            self.tabledict["default"][row_key] = {key: detail.move_to_data_array(data=sc.Variable([sc.Dim.Row],
                                                         values=scipp_obj))}
            self.is_histogram[key] = False

        print(self.tabledict)

        subtitle = "<span style='font-weight:normal;color:grey;"
        subtitle += "font-style:italic;background-color:#ffffff;"
        subtitle += "text-align:left;font-size:1.2em;padding: 1px;'>"
        subtitle += "{}</span>"
        title = str(type(scipp_obj)).replace("<class 'scipp._scipp.core.",
                                           "").replace("'>", "")

        self.box = [
            self.widgets.HTML(value="<span style='font-weight:bold;"
                              "font-size:1.5em;'>{}</span>".format(title))
        ]
        self.tables = {}
        self.sliders = {}
        self.label = self.widgets.Label(value="rows")
        self.nrows = {}
        self.sizes = {}

        # if len(self.tabledict["default"]) > 0:
        #     html, size = table_from_dataset(self.tabledict["default"],
        #                                     headers=self.headers,
        #                                     max_rows=config.table_max_size)
        #     self.tables["default"] = {" ": self.widgets.HTML(value=html)}
        #     hbox = self.make_hbox("default", " ", size)
        #     self.box.append(hbox)
        #     self.sizes["default"] = {" ": size}
        # if len(self.tabledict["0D Variables"]) > 0:
        #     self.tables["0D Variables"] = {}
        #     output = subtitle.format("0D Variables")
        #     html, size = table_from_dataset(self.tabledict["0D Variables"],
        #                                     headers=self.headers,
        #                                     max_rows=config.table_max_size)
        #     self.tables["0D Variables"][" "] = self.widgets.HTML(value=html)
        #     hbox = self.make_hbox("0D Variables", " ", size)
        #     self.box.append(
        #         self.widgets.VBox([self.widgets.HTML(value=output), hbox]))
        if len(self.tabledict["1D Variables"]) > 0:
            self.tables["1D Variables"] = {}
            self.sizes["1D Variables"] = {}
            output = subtitle.format("1D Variables")
            self.tabs = self.widgets.Tab(layout=self.widgets.Layout(
                width="initial"))
            children = []
            for key, val in sorted(self.tabledict["1D Variables"].items()):
                html, size = self.table_from_dict_of_variables(val,
                                                dim_key=key,
                                                # is_hist=self.is_histogram[key],
                                                headers=self.headers,
                                                max_rows=config.table_max_size)
                self.tables["1D Variables"][key] = self.widgets.HTML(
                    value=html)
                hbox = self.make_hbox("1D Variables", key, size)
                children.append(hbox)
                self.sizes["1D Variables"][key] = size

            self.tabs.children = children
            for i, key in enumerate(
                    sorted(self.tabledict["1D Variables"].keys())):
                self.tabs.set_title(i, key)
            self.box.append(
                self.widgets.VBox([self.widgets.HTML(value=output),
                                   self.tabs]))

        self.box = self.widgets.VBox(self.box,
                                     layout=self.widgets.Layout(
                                         border="solid 1px",
                                         width="auto",
                                         display='flex',
                                         flex_flow='column'))
        return

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
                self.sliders[key] = self.widgets.SelectionSlider(
                    options=np.arange(size - self.nrows[key].value + 1)[::-1],
                    value=0,
                    description="Starting row",
                    orientation='vertical',
                    continuous_update=False,
                    layout=self.widgets.Layout(height='400px'))
                setattr(self.sliders[key], "key", key)
                setattr(self.nrows[key], "key", key)
                setattr(self.sliders[key], "group", group)
                setattr(self.nrows[key], "group", group)
                self.sliders[key].observe(self.update_table, names="value")
                hbox = self.widgets.HBox([
                    hbox,
                    self.widgets.VBox([
                        self.widgets.HBox([self.nrows[key], self.label]),
                        self.sliders[key]
                    ])
                ])
        return hbox




    # def make_table_section_name_header(self, name, section, style):
    #     """
    #     Adds a first row of the table that contains the names of the sections
    #     being displayed in the table. This is usually the first row and will
    #     contain Data, Labels, Masks, etc.
    #     """
    #     col_separators = 0
    #     for key, sec in section.items():
    #         col_separators += 1 + (sec.variances is not None)
    #     if col_separators > 0:
    #         return "<th {} colspan='{}'>{}</th>".format(style, col_separators,
    #                                                     name)
    #     else:
    #         return ""


    def make_table_sections(self, dict_of_variables, base_style):

        html = ["<tr>"]
        for key, section in dict_of_variables.items():

            col_separators = 0
            for var in section.values():
                col_separators += 1 + (var.variances is not None)
            if col_separators > 0:
                style = "{} background-color: {};text-align: center;'".format(base_style, config.colors[key])
                html.append("<th {} colspan='{}'>{}</th>".format(style, col_separators,
                                                            key))
            # else:
            #     return ""

        # coord_style = "{} background-color: {};text-align: center;'".format(
        #     base_style, config.colors["coord"])
        # label_style = "{} background-color: {};text-align: center;'".format(
        #     base_style, config.colors["labels"])
        # mask_style = "{} background-color: {};text-align: center;'".format(
        #     base_style, config.colors["mask"])
        # data_style = "{} background-color: {};text-align: center;'".format(
        #     base_style, config.colors["data"])

        # colsp_coord = 0
        # if coord is not None:
        #     colsp_coord = 1 + (coord.variances is not None)
        # html = ["<tr>"]

        # if colsp_coord > 0:
        #     html.append("<th {} colspan='{}'>Coordinate</th>".format(
        #         coord_style, colsp_coord))
        # html.append(
        #     self.make_table_section_name_header("Labels", labels, label_style))
        # html.append(
        #     self.make_table_section_name_header("Masks", masks, mask_style))
        # html.append(self.make_table_section_name_header("Data", dict_of_data_arrays, data_style))

        html.append("</tr>")

        return "".join(html)


    def make_table_unit_headers(self, dict_of_variables, text_style):
        """
        Adds a row containing the unit of the section
        """

            # if coord is not None:
            #     html += "<th {} colspan='{}'>{}</th>".format(
            #         mstyle, 1 + (coord.variances is not None),
            #         name_with_unit(coord, replace_dim=False))

            # html += self.make_table_unit_headers(labels, mstyle)
            # html += self.make_table_unit_headers(masks, mstyle)
            # html += self.make_table_unit_headers(dict_of_data_arrays, mstyle)


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


    def make_value_rows(self, section, coord, index, base_style, edge_style, row_start):
        html = []
        for key, val in section.items():
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


    def make_trailing_cells(self, section, coord, index, size, base_style, edge_style):
        html = []
        for key, val in section.items():
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


    def make_overflow_cells(self, section, base_style):
        html = []
        for key, val in section.items():
            html.append("<td {}>...</td>".format(base_style))
            if val.variances is not None:
                html.append("<td {}>...</td>".format(base_style))
        return "".join(html)


    def make_overflow_row(self, dict_of_data_arrays, coord, labels, masks, base_style, hover_style):
        html = ["<tr {}>".format(hover_style)]
        if coord is not None:
            html.append(self.make_overflow_cells({" ": coord}, base_style))
        html.append(self.make_overflow_cells(labels, base_style))
        html.append(self.make_overflow_cells(masks, base_style))
        html.append(self.make_overflow_cells(dict_of_data_arrays, base_style))
        html.append("</tr>")
        return "".join(html)


    def table_from_dict_of_variables(self, dict_of_variables,
                           # is_hist=False,
                           dim_key=None,
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


        # # Get first entry in dict of data arrays
        # data_array = next(iter(dict_of_data_arrays.values()))
        # labels = data_array.labels
        # masks = data_array.masks

        # dims = data_array.dims
        coord = None
        # if len(dims) > 0:
        #     # DataArray should contain only one dim, so get the first in list
        #     size = data_array.shape[0]
        #     if len(data_array.coords) > 0:
        #         coord = data_array.coords[dims[0]]
        #         if is_hist:
        #             size += 1

        if len(dict_of_variables["coord"]) > 0:
            coord = dict_of_variables["coord"][dim_key]

        if headers > 1:
            html += self.make_table_sections(dict_of_variables, base_style)

        if headers > 0:
            html += "<tr {}>".format(hover_style)

            html += self.make_table_unit_headers(dict_of_variables, mstyle)
            # if coord is not None:
            #     html += "<th {} colspan='{}'>{}</th>".format(
            #         mstyle, 1 + (coord.variances is not None),
            #         name_with_unit(coord, replace_dim=False))

            # html += self.make_table_unit_headers(labels, mstyle)
            # html += self.make_table_unit_headers(masks, mstyle)
            # html += self.make_table_unit_headers(dict_of_data_arrays, mstyle)

            html += "</tr><tr {}>".format(hover_style)

            html += self.make_table_subsections(dict_of_variables, vstyle)

            # if coord is not None:
            #     html += "<th {}>Values</th>".format(vstyle)
            #     if coord.variances is not None:
            #         html += "<th {}>Variances</th>".format(vstyle)
            # html += self.make_table_subsections(labels, vstyle)
            # html += self.make_table_subsections(masks, vstyle)
            # html += self.make_table_subsections(dict_of_data_arrays, vstyle)
            html += "</tr>"

        html += "</table>"
        return html, 1

        # the base style still does not have a closing quote, so we add it here
        base_style += "'"

        if size is None:  # handle 0D variable
            html += "<tr {}>".format(hover_style)
            for key, val in dict_of_data_arrays.items():
                html += "<td {}>{}</td>".format(base_style,
                                                value_to_string(val.value))
                if val.variances is not None:
                    html += "<td {}>{}</td>".format(base_style,
                                                    value_to_string(val.variance))
            html += "</tr>"
        else:
            row_end = min(size, row_start + max_rows)
            if row_start > 0:
                html += self.make_overflow_row(dict_of_data_arrays, coord, labels, masks, base_style, hover_style)
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

                html += self.make_value_rows(labels, coord, i, base_style,
                                         edge_style, row_start)
                html += self.make_value_rows(masks, coord, i, base_style,
                                         edge_style, row_start)
                html += self.make_value_rows(dict_of_data_arrays, coord, i, base_style, edge_style,
                                         row_start)

                html += "</tr><tr {}>".format(hover_style)
                # If there are bin edges, we need to add trailing cells for data
                # and labels
                if coord is not None:
                    html += self.make_trailing_cells(labels, coord, i, row_end,
                                                 base_style, edge_style)
                    html += self.make_trailing_cells(masks, coord, i, row_end,
                                                 base_style, edge_style)
                    html += self.make_trailing_cells(dict_of_data_arrays, coord, i, row_end,
                                                 base_style, edge_style)
                html += "</tr>"
            if row_end != size:
                html += self.make_overflow_row(dict_of_data_arrays, coord, labels, masks, base_style, hover_style)

        html += "</table>"
        return html, size











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
            # to_table = self.tabledict[group]
            # # This extra indexing step comes from the fact that for the
            # # default key, the tabledict is a Dataset, while for the 1D
            # # Variables it is a dict of Datasets.
            # if group != "default":
            #     to_table = to_table[key]
            html, size = table_from_dict_of_data_arrays(self.tabledict[group][key],
                                            is_hist=self.is_histogram[key],
                                            headers=self.headers,
                                            row_start=self.sliders[key].value,
                                            max_rows=self.nrows[key].value)
            self.tables[group][key].value = html
