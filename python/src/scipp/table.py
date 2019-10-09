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
    # border style
    bstyle = "style='border: 1px solid black;"
    cstyle = bstyle + "background-color: #adf3e0;'"
    lstyle = bstyle + "background-color: #d9c0fa;'"
    # cstyle = [
    #     bstyle + "background-color: #adf3e0;'",
    #     bstyle + "background-color: #b9ffec;'"
    # ]
    # lstyle = [
    #     bstyle + "background-color: #d9c0fa;'",
    #     bstyle + "background-color: #ecdeff;'"
    # ]
    bstyle += "'"
    estyle = "style='border: 0px solid white;background-color: #ffffff;'"

    # Declare table
    html = "<table style='border-collapse: collapse;'>"
    dims = dataset.dims
    dataset_dim = size = coord = None
    # size = None
    if len(dims) > 0:
        # Get first key in dict
        dataset_dim = next(iter(dims))
        size = dims[dataset_dim]
        if len(dataset.coords) > 0:
            coord = dataset.coords[dataset_dim]
            if is_hist:
                size += 1
    # for key, val in dataset:
    #     print("shape", val.shape, len(val.shape))
    #     if len(val.shape) > 0:
    #         size = val.shape[0]
    #     else:
    #         size = None
    # # Get first key in dict
    # dataset_dim = next(iter(dataset.dims))
    # Table headers
    print(size, is_hist)
    if headers:
        html += "<tr>"
        # if dataset_dim is not None:
        if coord is not None:
            # dataset_dim = next(iter(dataset.dims))
            # coord = dataset.coords[dataset_dim]
            html += "<th {} colspan='{}'>Coord: {}</th>".format(
                cstyle.replace("style='", "style='text-align: center;"),
                1 + (coord.variances is not None), axis_label(coord))
        for key, val in dataset:
            html += "<th {} colspan='{}'>{}</th>".format(
                bstyle.replace("style='", "style='text-align: center;"),
                1 + (val.variances is not None), axis_label(val, name=key))
        for key, lab in dataset.labels:
            print(lab.dims,dataset_dim)
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
        # if is_hist:
        #     td_open = "<td rowspan='2' "
        # else:
        #     td_open = "<td "
        for i in range(size):
            html += '<tr style="font-weight:normal">'
            if coord is not None:
                text = value_to_string(coord.values[i])
                # if is_hist:
                #     text = '[{}; {}]'.format(
                #         text, value_to_string(coord.values[i + 1]))
                html += "<td rowspan='2' {}>{}</td>".format(bstyle, text)
                if coord.variances is not None:
                    text = value_to_string(coord.variances[i])
                    # if is_hist:
                    #     text = '[{}; {}]'.format(
                    #         text, value_to_string(coord.variances[i + 1]))
                    html += "<td rowspan='2' {}>{}</td>".format(bstyle, text)
            # Check for bin edges
            # if i == 0:
            #     for key, val in dataset:
            #         if len(val.values) == len(coord.values) - 1:
            #             html += <>
            #         html += "<td {}>{}</td>".format(bstyle,
            #                                         value_to_string(val.values[i]))
            #         if val.variances is not None:
            #             html += "<td {}>{}</td>".format(
            #                 bstyle, value_to_string(val.variances[i]))

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
                    html += "<td rowspan='2' {}>{}</td>".format(bstyle,
                        value_to_string(val.values[i]))
                    if val.variances is not None:
                        html += "<td rowspan='2' {}>{}</td>".format(
                            bstyle, value_to_string(val.variances[i]))
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
                        html += "<td rowspan='2' {}>{}</td>".format(bstyle,
                            value_to_string(lab.values[i]))
                        if lab.variances is not None:
                            html += "<td rowspan='2' {}>{}</td>".format(
                                bstyle, value_to_string(lab.variances[i]))



                    # if i == 0 and len(val.values) == len(coord.values) - 1:
                    # html += "<td {}>{}</td>".format(bstyle, lab.values[i])
            html += "</tr><tr>"
            if coord is not None:
                for key, val in dataset:
                    if len(val.values) == len(coord.values) - 1:
                        if i == size - 1:
                            html += "<td {}></td>".format(estyle)
                            if val.variances is not None:
                                html += "<td {}></td>".format(estyle)
                        else:
                            html += "<td rowspan='2' {}>{}</td>".format(bstyle,
                                value_to_string(val.values[i]))
                            if val.variances is not None:
                                html += "<td rowspan='2' {}>{}</td>".format(
                                    bstyle, value_to_string(val.variances[i]))
                for key, lab in dataset.labels:
                    if (lab.dims[0] == dataset_dim) and \
                        (len(lab.values) == len(coord.values) - 1):
                        if i == size - 1:
                            html += "<td {}></td>".format(estyle)
                            if lab.variances is not None:
                                html += "<td {}></td>".format(estyle)
                        else:
                            html += "<td rowspan='2' {}>{}</td>".format(bstyle,
                                    value_to_string(lab.values[i]))
                            if lab.variances is not None:
                                html += "<td rowspan='2' {}>{}</td>".format(
                                    bstyle, value_to_string(lab.variances[i]))
            html += "</tr>"

    html += "</table>"
    return html


def table(dataset):
    from IPython.display import display, HTML

    # title = "<h3>{}</h3>".format(
    #     str(type(dataset)).replace("<class 'scipp._scipp.",
    #                                "").replace("'>", ""))

    tabledict = {
        "default": sc.Dataset(),
        "0D Variables": sc.Dataset(),
        "1D Variables": {}
    }
    coord_1d = {}
    is_histogram = {}
    coord_def = None
    headers = True
    is_empty = {}
    hist_tag = ":hist"

    tp = type(dataset)
    if (tp is sc.Dataset) or (tp is sc.DatasetProxy):
        # First add one entry per dimension
        for dim in dataset.dims:
            key = str(dim)
            # Create two separate entries to accomodate for potential bin-edge
            # coordinates
            # for i, suffix in enumerate(["", hist_tag]):
            #     new_key = "{}{}".format(key, suffix)
            #     tabledict["1D Variables"][new_key] = sc.Dataset()
            #     is_empty[new_key] = True
            #     is_histogram[new_key] = i > 0
            tabledict["1D Variables"][key] = sc.Dataset()
            is_empty[key] = True
            is_histogram[key] = False

        # # Next add only the 1D coordinates
        # for dim, var in dataset.coords:
        #     if len(var.dims) == 1:
        #         # dim = coord.dims[0]
        #         key = str(dim)
        #         tabledict["1D Variables"][key].coords[dim] = var
        #         is_empty[key] = False
        # Next add the variables
        for name, var in dataset:
            if len(var.dims) == 1:
                dim = var.dims[0]
                key = str(dim)
                if len(var.coords) > 0:
                    if len(var.coords[dim].values) == len(var.values) + 1:
                        # key = "{}{}".format(key, hist_tag)
                        is_histogram[key] = True
                    tabledict["1D Variables"][key].coords[dim] = var.coords[dim]
                    # else:
                    #     new_key = key

                tabledict["1D Variables"][key][name] = var.data
                is_empty[key] = False

            #     if key not in tabledict["1D Variables"].keys():
            #         temp_dict = {"data": {name: dataset[name].data}}
            #         if len(var.coords) > 0:
            #             coord_1d[key] = var.dims[0]
            #             temp_dict["coords"] = {
            #                 var.dims[0]: var.coords[var.dims[0]]
            #             }
            #             if len(var.coords[var.dims[0]].values) == \
            #                len(var.values) + 1:
            #                 is_histogram[key] = True
            #             else:
            #                 is_histogram[key] = False
            #         else:
            #             coord_1d[key] = None
            #         tabledict["1D Variables"][key] = sc.Dataset(**temp_dict)
            #     else:
            #         tabledict["1D Variables"][key][name] = var
            elif len(var.dims) == 0:
                tabledict["0D Variables"][name] = var
        # print("==================")
        # print(tabledict["1D Variables"])
        # print("==================")
        # Next add only the 1D coordinates
        for dim, var in dataset.coords:
            if len(var.dims) == 1:
                # dim = coord.dims[0]
                key = str(dim)
                # print(key)
                # # print(dim not in tabledict["1D Variables"][key].coords)
                # if dim not in tabledict["1D Variables"][key].coords and \
                #     dim not in tabledict["1D Variables"]["{}{}".format(
                #         key, hist_tag)].coords:
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
                        # key = "{}{}".format(key, hist_tag)
                        is_histogram[key] = True
                    tabledict["1D Variables"][key].coords[dim] = dataset.coords[dim]
                # # hist_key = "hist:{}".format(key)
                # # print(lab)
                # # print(is_empty[key], is_empty[hist_key])
                # if is_empty[key] and (not is_empty[hist_key]):
                #     new_key = hist_key
                # else:
                #     new_key = key
                # # print(new_key)
                tabledict["1D Variables"][key].labels[name] = lab
                is_empty[key] = False

        # Now purge out the empty entries
        for key, val in is_empty.items():
            if val:
                # print("deleting entry", key)
                del(tabledict["1D Variables"][key])

        # # for key in tabledict["1D Variables"].keys():
        # #     print(key)
        # #     if is_empty[key]:
        # #         print("deleting entry", key)
        # #         del(tabledict["1D Variables"][key])
        # print("++++++++++++++++")
        # print(tabledict["1D Variables"])
        # print("++++++++++++++++")



    elif (tp is sc.DataArray) or (tp is sc.DataProxy):
        key = dataset.name
        tabledict["default"][key] = dataset.data
        if len(dataset.coords) > 0:
            dim = dataset.dims[0]
            tabledict["default"][key].coords[dim] = dataset.coords[dim]
                # coord_def = dataset.dims[0]
    elif (tp is sc.Variable) or (tp is sc.VariableProxy):
        key = ""
        tabledict["default"][key] = dataset
    else:
        tabledict["default"][""] = sc.Variable([sc.Dim.Row], values=dataset)
        headers = False

    subtitle = "<tr><td style='font-weight:normal;color:grey;font-style:italic;background-color:#ffffff;text-align:left;font-size:1.2em;padding: 1px;'>&nbsp;{}</td></tr>"
    # title = str(type(dataset))
    whitetd = "<tr><td style='background-color:#ffffff;'>"
    title = str(type(dataset)).replace("<class 'scipp._scipp.core.",
                                       "").replace("'>", "")
    output = "<table style='border: 1px solid black;'><tr><td style='font-weight:bold;font-size:1.5em;padding:1px;"
    output += "text-align:center;background-color:#ffffff;'>"
    output += "{}</td></tr>".format(title)

    if len(tabledict["default"]) > 0:
        # print(tabledict["default"])
        output += whitetd
        # output += "text-align:center;background-color:#ffffff;'>"
        output += table_from_dataset(tabledict["default"],
                                     # coord_dim=coord_def,
                                     headers=headers)
        output += "</td></tr>"
    if len(tabledict["0D Variables"]) > 0:
        output += subtitle.format("0D Variables")
         # "<tr><td {}>0D Variables</td></tr>".format(subtitle)
        output += whitetd
        # output += subtitle + "0D Variables</h6>"
        output += table_from_dataset(tabledict["0D Variables"])
        output += "</td></tr>"
    if len(tabledict["1D Variables"].keys()) > 0:
        # output += "<tr><td {}>1D Variables</td></tr>".format(subtitle)
        output += subtitle.format("1D Variables")
        output += whitetd
        # oldkey = None
        for key, val in sorted(tabledict["1D Variables"].items()):
            # # Here we place the bin-edge and non-bin-edge tables with the same
            # # dimensions on the same row
            # raw_key = key.replace(hist_tag, "")
            # if raw_key in tabledict["1D Variables"].keys() and \
            #     "{}{}".format(raw_key, hist_tag) in tabledict["1D Variables"].keys():
            #     outer_table = True
            # else:
            #     outer_table = False
            # print("=========")
            # print(key, val)
            # if outer_table:
            #     if key.count(hist_tag) < 1:
            #         output += "<table><tr>"
            #     output += "<td style='border: 0px solid white;"
            #     output += "background-color: #ffffff;"
            #     output += "vertical-align:top;'>"
            # print(val.dims.keys()[0])
            # print(next(iter(val.dims)))
            output += table_from_dataset(val,
                                         # coord_dim=coord_1d[key],
                                         is_hist=is_histogram[key])
            # if outer_table:
            #     output += "</td>"
            #     if key.count(hist_tag) > 0:
            #         output += "</tr></table>"
            # oldkey = key.replace(hist_tag, "")
        output += "</td></tr>"
    output += "</table>"

    # print(output)

    display(HTML(output))

    return
