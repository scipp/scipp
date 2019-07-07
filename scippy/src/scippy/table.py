# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Igor Gudich

import scippy as sp
from xml.etree import ElementTree as et
from collections import defaultdict


style_border_center = {'style': 'border: 1px solid black; text-align:center'}
style_border_right = {'style': 'border: 1px solid black; text-align:right'}


def value_to_string(val):
    if (not isinstance(val, float)) or (val == 0):
        text = str(val)
    elif abs(val) >= 1.0e4 or abs(val) <= 1.0e-4:
        text = "{:.3e}".format(val)
    else:
        text = "{}".format(val)
        if len(text) > 5 + (text[0] == '-'):
            text = "{:.3f}".format(val)
    return text


def append_with_text(parent, name, text, attrib=style_border_right):
    el = et.SubElement(parent, name, attrib=attrib)
    el.text = text


def table_ds(dataset):
    coords = dataset.coords
    ndims = len(coords)
    if ndims > 1:
        raise RuntimeError("Only 0D & 1D datasets can be rendered as a table")

    body = et.Element('body')
    headline = et.SubElement(body, 'h3')
    if isinstance(dataset, sp.Dataset):
        headline.text = 'Dataset:'
    else:
        headline.text = 'DataProxy:'

    # # List the names and count the variances
    # names = dict()
    # for name, var in dataset:
    #     names[name] = var.has_variances



    # names = list(dict.fromkeys([name for name, var in dataset]))




    datum1d = defaultdict(list)
    datum0d = defaultdict(list)
    # coords0d = defaultdict(list)
    # coords1d = defaultdict(list)
    coords1d = None
    coords0d = None
    for name, var in dataset:
    # for name in names:
    #     var = dataset[name]
        if len(var.coords) == 1:
            datum1d[name] = var
            coords = var.coords
            for c in coords:
                coords1d = coords[c[0]]
        else:
            datum0d[name] = var
            coords = var.coords
            for c in coords:
                coords0d = coords[c[0]]
            # coords0d[name].append(var.coords)

    # coord_names = list(dict.fromkeys(
    #     [var.name for var in dataset if var.is_coord]))
    # coords0d = defaultdict(list)
    # coords1d = defaultdict(list)
    # for name in coord_names:
    #     for var in [
    #             var for var in dataset if var.is_coord and var.name == name]:
    #         if len(var.dimensions) == 1:
    #             coords1d[name].append(var)
    #         else:
    #             coords0d[name].append(var)

    # # 0 - dimensional data
    # if datum0d or coords0d:
    #     tab = et.SubElement(body, 'table')
    #     cap = et.SubElement(tab, 'caption')
    #     cap.text = '0D Variables:'
    #     tr_name = et.SubElement(tab, 'tr')
    #     tr_tag = et.SubElement(tab, 'tr')
    #     tr_unit = et.SubElement(tab, 'tr')
    #     tr_val = et.SubElement(tab, 'tr')

    #     for key, val in coords0d.items():
    #         append_with_text(tr_name, 'th', key,
    #                          attrib=dict({'colspan': str(len(val))}.items() |
    #                                      style_border_center.items()))
    #         for var in val:
    #             append_with_text(tr_tag, 'th', str(var.tag))
    #             append_with_text(tr_val, 'th', str(var.data[0]))
    #             append_with_text(tr_unit, 'th', '[{}]'.format(var.unit))

    #     for key, val in datum0d.items():
    #         append_with_text(tr_name, 'th', key,
    #                          attrib=dict({'colspan': str(len(val))}.items() |
    #                                      style_border_center.items()))
    #         for var in val:
    #             append_with_text(tr_tag, 'th', str(var.tag))
    #             append_with_text(tr_val, 'th', str(var.data[0]))
    #             append_with_text(tr_unit, 'th', '[{}]'.format(var.unit))

    # 1 - dimensional data
    if datum1d or coords1d:

        itab = et.SubElement(body, 'table')
        tab = et.SubElement(itab, 'tbody', attrib=style_border_center)
        cap = et.SubElement(tab, 'capltion')
        cap.text = '1D Variables:'
        tr = et.SubElement(tab, 'tr')

        # Coordinates
        append_with_text(tr, 'th', axis_label(coords1d),
                         attrib=dict({'colspan': str(1 + coords1d.has_variances)}.items() |
                                    style_border_center.items()))

        # Data fields
        for key, val in datum1d.items():
            append_with_text(tr, 'th', axis_label(val, name=key),
                             attrib=dict({'colspan': str(1 + val.has_variances)}.items() |
                                         style_border_center.items()))
            dims = val.dims
            length = dims.shape[0]

        is_hist = length == (len(coords1d.values) - 1)

        tr = et.SubElement(tab, 'tr')

        append_with_text(
            tr, 'th', "Values", style_border_center)
        if coords1d.has_variances:
            append_with_text(
                tr, 'th', "Variances", style_border_center)

        for key, val in datum1d.items():
            append_with_text(
                tr, 'th', "Values", style_border_center)
            if val.has_variances:
                append_with_text(
                    tr, 'th', "Variances", style_border_center)

        for i in range(length):
            tr = et.SubElement(tab, 'tr')
            text = value_to_string(coords1d.values[i])
            if is_hist:
                text = '[{}; {}]'.format(
                    text, value_to_string(coords1d.values[i + 1]))
            append_with_text(tr, 'td', text)
            if coords1d.has_variances:
                text = value_to_string(coords1d.variances[i])
                if is_hist:
                    text = '[{}; {}]'.format(
                        text, value_to_string(coords1d.variances[i + 1]))
                append_with_text(tr, 'td', text)
            for key, val in datum1d.items():
                append_with_text(tr, 'td', value_to_string(val.values[i]))
                if val.has_variances:
                    append_with_text(tr, 'td', value_to_string(val.variances[i]))

    from IPython.display import display, HTML
    display(HTML(et.tostring(body).decode('UTF-8')))


def table_var(variable):
    if len(variable.dimensions) > 1:
        raise RuntimeError("Only 1-D variable can be rendered")

    body = et.Element('body')
    headline = et.SubElement(body, 'h3')
    if isinstance(variable, sp.Variable):
        headline.text = 'Variable:'
    else:
        headline.text = 'VariableSlice:'
    tab = et.SubElement(body, 'table')

    tr_tag = et.SubElement(tab, 'tr')
    tr_unit = et.SubElement(tab, 'tr')
    append_with_text(tr_tag, 'th', str(variable.tag))
    append_with_text(tr_unit, 'th', '[{}]'.format(variable.unit))

    if variable.name:
        tr_name = et.SubElement(tab, 'tr')
        append_with_text(tr_name, 'th', variable.name)
    # Aligned data
    for val in variable.data:
        tr_val = et.SubElement(tab, 'tr')
        append_with_text(tr_val, 'th', value_to_string(val))

    from IPython.display import display, HTML
    display(HTML(et.tostring(body).decode('UTF-8')))


def table(some):
    tp = type(some)
    if tp is sp.Dataset or tp is sp.DataProxy:
        table_ds(some)
    elif tp is sp.Variable or tp is sp.VariableProxy:
        table_var(some)
    else:
        raise RuntimeError("Type {} is not supported".format(tp))



def axis_label(var, name=None, log=False):
    """
    Make an axis label with "Name [unit]"
    """
    if name is not None:
        label = name
    else:
        label = str(var.dims.labels[0]).replace("Dim.", "")

    if log:
        label = "log\u2081\u2080(" + label + ")"
    if var.unit != sp.units.dimensionless:
        label += " [{}]".format(var.unit)
    return label
