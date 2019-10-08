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

    # Declare table
    html = "<table style='border-collapse: collapse;'>"
    dims = dataset.dims
    coord_dim = size = coord = None
    # size = None
    if len(dims) > 0:
        # Get first key in dict
        coord_dim = next(iter(dims))
        size = dims[coord_dim]
        if len(dataset.coords) > 0:
            coord = dataset.coords[coord_dim]
    # for key, val in dataset:
    #     print("shape", val.shape, len(val.shape))
    #     if len(val.shape) > 0:
    #         size = val.shape[0]
    #     else:
    #         size = None
    # # Get first key in dict
    # coord_dim = next(iter(dataset.dims))
    # Table headers
    if headers:
        html += "<tr>"
        # if coord_dim is not None:
        if len(dataset.coords) > 0:
            # coord_dim = next(iter(dataset.dims))
            # coord = dataset.coords[coord_dim]
            html += "<th {} colspan='{}'>Coord: {}</th>".format(
                cstyle.replace("style='", "style='text-align: center;"),
                1 + (coord.variances is not None), axis_label(coord))
        for key, val in dataset:
            html += "<th {} colspan='{}'>{}</th>".format(
                bstyle.replace("style='", "style='text-align: center;"),
                1 + (val.variances is not None), axis_label(val, name=key))
        for key, lab in dataset.labels:
            print(lab.dims,coord_dim)
            if lab.dims[0] == coord_dim:
                html += "<th {}>{}</th>".format(
                    lstyle.replace("style='", "style='text-align: center;"),
                    key)
        html += "</tr><tr>"
        if coord_dim is not None:
            html += "<th {}>Values</th>".format(bstyle)
            if coord.variances is not None:
                html += "<th {}>Variances</th>".format(bstyle)
        for key, val in dataset:
            html += "<th {}>Values</th>".format(bstyle)
            if val.variances is not None:
                html += "<th {}>Variances</th>".format(bstyle)
        for key, lab in dataset.labels:
            if lab.dims[0] == coord_dim:
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
            if coord_dim is not None:
                text = value_to_string(coord.values[i])
                if is_hist:
                    text = '[{}; {}]'.format(
                        text, value_to_string(coord.values[i + 1]))
                html += "<td {}>{}</td>".format(bstyle, text)
                if coord.variances is not None:
                    text = value_to_string(coord.variances[i])
                    if is_hist:
                        text = '[{}; {}]'.format(
                            text, value_to_string(coord.variances[i + 1]))
                    html += "<td {}>{}</td>".format(bstyle, text)
            for key, val in dataset:
                html += "<td {}>{}</td>".format(bstyle,
                                                value_to_string(val.values[i]))
                if val.variances is not None:
                    html += "<td {}>{}</td>".format(
                        bstyle, value_to_string(val.variances[i]))
            for key, lab in dataset.labels:
                if lab.dims[0] == coord_dim:
                    html += "<td {}>{}</td>".format(bstyle, lab.values[i])
            html += "</tr>"
    html += "</table>"
    return html


def table(dataset):
    from IPython.display import display, HTML

    title = "<h3>{}</h3>".format(
        str(type(dataset)).replace("<class 'scipp._scipp.",
                                   "").replace("'>", ""))

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

    tp = type(dataset)
    if (tp is sc.Dataset) or (tp is sc.DatasetProxy):
        # First add one entry per dimension
        for dim in dataset.dims:
            key = str(dim)
            # Create two separate entries to accomodate for potential bin-edge
            # coordinates
            for i, prefix in enumerate(["", "hist:"]):
                new_key = "{}{}".format(prefix, key)
                tabledict["1D Variables"][new_key] = sc.Dataset()
                is_empty[new_key] = True
                is_histogram[new_key] = i > 0
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
                        key = "hist:{}".format(key)
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
                # print(dim not in tabledict["1D Variables"][key].coords)
                if dim not in tabledict["1D Variables"][key].coords and \
                    dim not in tabledict["1D Variables"]["hist:{}".format(key)].coords:
                    tabledict["1D Variables"][key].coords[dim] = var
                    is_empty[key] = False
        # Next add the labels
        for name, lab in dataset.labels:
            if len(lab.dims) == 1:
                dim = lab.dims[0]
                key = str(dim)
                if len(dataset.coords) > 0:
                    if len(dataset.coords[dim].values) == len(lab.values) + 1:
                        key = "hist:{}".format(key)
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



    elif (tp is sc.DataArray) or (tp is sc.DataProxy) or (
            tp is sc.Variable) or (tp is sc.VariableProxy):
        print(tp)
        try:
            key = dataset.name
        except AttributeError:
            key = ""
        tabledict["default"][key] = dataset
        if (tp is sc.DataProxy) and (len(dataset.dims) > 0):
            if len(dataset.coords) > 0:
                coord_def = dataset.dims[0]
    else:
        tabledict["default"][""] = sc.Variable([sc.Dim.Row], values=dataset)
        headers = False

    subtitle = "<h6 style='font-weight: normal; color: grey'>"
    output = ""
    if len(tabledict["default"]) > 0:
        print(tabledict["default"])
        output += table_from_dataset(tabledict["default"],
                                     # coord_dim=coord_def,
                                     headers=headers)
    if len(tabledict["0D Variables"]) > 0:
        output += subtitle + "0D Variables</h6>"
        output += table_from_dataset(tabledict["0D Variables"])
    if len(tabledict["1D Variables"].keys()) > 0:
        output += subtitle + "1D Variables</h6>"
        for key, val in sorted(tabledict["1D Variables"].items()):
            print("=========")
            print(key, val)
            # print(val.dims.keys()[0])
            # print(next(iter(val.dims)))
            output += table_from_dataset(val,
                                         # coord_dim=coord_1d[key],
                                         is_hist=is_histogram[key])

    display(HTML(title + output))

    return
