from dataset import Data, Dataset, DatasetSlice, Variable, VariableSlice
from xml.etree import ElementTree as et
from collections import defaultdict


style_border_center = {'style': 'border: 1px solid black; text-align:center'}
style_border_right = {'style': 'border: 1px solid black; text-align:right'}


def append_with_text(parent, name, text, attrib=style_border_right):
    el = et.SubElement(parent, name, attrib=attrib)
    el.text = text


def table_ds(dataset):
    if len(dataset.dimensions) > 1:
        raise RuntimeError("Only 1-D datasets can be rendered as a table")

    body = et.Element('body')
    headline = et.SubElement(body, 'h3')
    if type(dataset) is Dataset:
        headline.text = 'Dataset:'
    else:
        headline.text = 'DatasetSlice:'
    names = list(dict.fromkeys([var.name for var in dataset if var.is_data]))
    datum1d = defaultdict(list)
    datum0d = defaultdict(list)
    for name in names:
        sub = dataset.subset[name]
        for var in sub:
            if var.is_data and len(var.dimensions) == 1:
                datum1d[name].append(var)
            elif var.is_data:
                datum0d[name].append(var)

    coord_names = list(dict.fromkeys([var.name for var in dataset if var.is_coord]))
    coords0d = defaultdict(list)
    coords1d = defaultdict(list)
    for name in coord_names:
        for var in [var for var in dataset if var.is_coord and var.name == name]:
            if len(var.dimensions) == 1:
                coords1d[name].append(var)
            else:
                coords0d[name].append(var)

    # 0 - dimensional data
    if datum0d or coords0d:
        tab = et.SubElement(body, 'table')
        cap = et.SubElement(tab, 'caption')
        cap.text = '0D Variables:'
        tr_name = et.SubElement(tab, 'tr')
        tr_tag = et.SubElement(tab, 'tr')
        tr_unit = et.SubElement(tab, 'tr')
        tr_val = et.SubElement(tab, 'tr')

        for key, val in coords0d.items():
            append_with_text(tr_name, 'th', key,
                             attrib=dict({'colspan': str(len(val))}.items() | style_border_center.items()))
            for var in val:
                append_with_text(tr_tag, 'th', str(var.tag))
                append_with_text(tr_val, 'th', str(var.data[0]))
                append_with_text(tr_unit, 'th', '[{}]'.format(var.unit))

        for key, val in datum0d.items():
            append_with_text(tr_name, 'th', key,
                             attrib=dict({'colspan': str(len(val))}.items() | style_border_center.items()))
            for var in val:
                append_with_text(tr_tag, 'th', str(var.tag))
                append_with_text(tr_val, 'th', str(var.data[0]))
                append_with_text(tr_unit, 'th', '[{}]'.format(var.unit))

    # 1 - dimensional data
    if datum1d or coords1d:
        datas = []
        for key, val in datum1d.items():
            datas.extend(val)

        coords = []
        for key, val in coords1d.items():
            coords.extend(val)

        itab = et.SubElement(body, 'table')
        tab = et.SubElement(itab, 'tbody', attrib=style_border_center)
        cap = et.SubElement(tab, 'capltion')
        cap.text = '1D Variables:'
        tr = et.SubElement(tab, 'tr')


        # Aligned names
        for key, val in coords1d.items():
            append_with_text(tr, 'th', key,
                             attrib=dict({'colspan': str(len(val))}.items() | style_border_center.items()))
        for key, val in datum1d.items():
            append_with_text(tr, 'th', key,
                             attrib=dict({'colspan': str(len(val))}.items() | style_border_center.items()))

        length = min([len(x) for x in datas] + [len(x) for x in coords])

        is_hist = [length != len(x) for x in coords]

        tr = et.SubElement(tab, 'tr')

        for x in coords:
            append_with_text(tr, 'th', '{}'.format(x.tag, x.name), style_border_center)

        for x in datas:
            append_with_text(tr, 'th', '{}'.format(x.tag, x.name), style_border_center)

        tr = et.SubElement(tab, 'tr')
        for x in coords:
            append_with_text(tr, 'th', '[{}]'.format(x.unit), style_border_center)
        for x in datas:
            append_with_text(tr, 'th', '[{}]'.format(x.unit), style_border_center)

        for i in range(length):
            tr = et.SubElement(tab, 'tr')
            for x, h in zip(coords, is_hist):
                text = str(x.data[i])
                if h:
                    text = '[{}; {}]'.format(text, str(x.data[i+1]))
                append_with_text(tr, 'th', text)
            for x in datas:
                append_with_text(tr, 'th', str(x.data[i]))

    from IPython.display import display, HTML
    display(HTML(et.tostring(body).decode('UTF-8')))


def table_var(variable):
    if len(variable.dimensions) > 1:
        raise RuntimeError("Only 1-D variable can be rendered")

    body = et.Element('body')
    headline = et.SubElement(body, 'h3')
    if type(variable) is Variable:
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
        append_with_text(tr_val, 'th', str(val))

    from IPython.display import display, HTML
    display(HTML(et.tostring(body).decode('UTF-8')))


def table(some):
    tp = type(some)
    if tp is Dataset or tp is DatasetSlice:
        table_ds(some)
    elif tp is Variable or tp is VariableSlice:
        table_var(some)
    else:
        raise RuntimeError("Type {} is not supported".format(tp))

