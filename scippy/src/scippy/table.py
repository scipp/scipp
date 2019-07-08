# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Igor Gudich

import scippy as sp
from xml.etree import ElementTree as et
from collections import defaultdict
from .tools import axis_label


style_border_center = {'style': 'border: 1px solid black; text-align:center'}
style_border_right = {'style': 'border: 1px solid black; text-align:right'}


def value_to_string(val, precision=3):
    if (not isinstance(val, float)) or (val == 0):
        text = str(val)
    elif (abs(val) >= 10.0**(precision+1)) or (abs(val) <= 10.0**(-precision-1)):
        text = "{val:.{prec}e}".format(val=val, prec=precision)
    else:
        text = "{}".format(val)
        if len(text) > precision + 2 + (text[0] == '-'):
            text = "{val:.{prec}f}".format(val=val, prec=precision)
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

    datum1d = defaultdict(list)
    datum0d = defaultdict(list)
    coords1d = None
    coords0d = None
    for name, var in dataset:
        if len(var.coords) == 1:
            datum1d[name] = var
            coords1d = var.coords[var.dims.labels[0]]

        else:
            datum0d[name] = var

    # 0 - dimensional data
    if datum0d:
        itab = et.SubElement(body, 'table')
        tab = et.SubElement(itab, 'tbody', attrib=style_border_center)
        cap = et.SubElement(tab, 'caption')
        cap.text = '0D Variables:'
        tr = et.SubElement(tab, 'tr')

        # Data fields
        for key, val in datum0d.items():
            append_with_text(tr, 'th', axis_label(val, name=key),
                             attrib=dict({'colspan': str(1 + val.has_variances)}.items() |
                                         style_border_center.items()))

        tr = et.SubElement(tab, 'tr')
        # Go through all items in dataset and add headers
        for key, val in datum0d.items():
            append_with_text(
                tr, 'th', "Values", style_border_center)
            if val.has_variances:
                append_with_text(
                    tr, 'th', "Variances", style_border_center)

        tr = et.SubElement(tab, 'tr')
        for key, val in datum0d.items():
            append_with_text(tr, 'td', value_to_string(val.value))
            if val.has_variances:
                append_with_text(tr, 'td', value_to_string(val.variance))


    # 1 - dimensional data
    if datum1d or coords1d:

        itab = et.SubElement(body, 'table')
        tab = et.SubElement(itab, 'tbody', attrib=style_border_center)
        cap = et.SubElement(tab, 'caption')
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

        # Check if is histogram
        # TODO: what if the Dataset contains one coordinate with N+1 elements,
        # one variable with N elements, and another variable with N+1 elements.
        # How do we render this as a table if we only have one column for the
        # coordinate?
        is_hist = length == (len(coords1d.values) - 1)

        # Make table row for "Values" and "Variances"
        tr = et.SubElement(tab, 'tr')
        append_with_text(
            tr, 'th', "Values", style_border_center)
        if coords1d.has_variances:
            append_with_text(
                tr, 'th', "Variances", style_border_center)
        # Go through all items in dataset and add headers
        for key, val in datum1d.items():
            append_with_text(
                tr, 'th', "Values", style_border_center)
            if val.has_variances:
                append_with_text(
                    tr, 'th', "Variances", style_border_center)
        # Now write all the data row by row
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
    # Render the HTML code
    from IPython.display import display, HTML
    display(HTML(et.tostring(body).decode('UTF-8')))


def table_var(variable):
    dims = variable.dims
    labs = dims.labels
    if len(labs) > 1:
        raise RuntimeError("Only 1-D variable can be rendered")
    nx = dims.shape[0]

    body = et.Element('body')
    headline = et.SubElement(body, 'h3')
    if isinstance(variable, sp.Variable):
        headline.text = 'Variable:'
    else:
        headline.text = 'VariableProxy:'
    tab = et.SubElement(body, 'table')

    tr = et.SubElement(tab, 'tr')
    append_with_text(tr, 'th', axis_label(variable, name=str(labs[0])),
                         attrib=dict({'colspan': str(1 + variable.has_variances)}.items() |
                                    style_border_center.items()))

    # Aligned data
    tr = et.SubElement(tab, 'tr')

    append_with_text(
        tr, 'th', "Values", style_border_center)
    if variable.has_variances:
        append_with_text(
            tr, 'th', "Variances", style_border_center)
    for i in range(nx):
        tr_val = et.SubElement(tab, 'tr')
        append_with_text(tr_val, 'td', value_to_string(variable.values[i]))
        if variable.has_variances:
            append_with_text(tr_val, 'td', value_to_string(variable.variances[i]))

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
