from dataset import Data, Dataset, DatasetSlice, Variable, VariableSlice
from xml.etree import ElementTree as et
from collections import defaultdict


def append_with_text(parent, name, text, attrib={}):
    el = et.SubElement(parent, name, attrib=attrib)
    el.text = text


def table_ds(dataset):
    if len(dataset.dimensions) > 1:
        raise RuntimeError("Only 1-D datasets can be rendered as a table")

    body = et.Element('body')
    headline = et.SubElement(body, 'h3')
    headline.text = 'Dataset:'
    names = list(dict.fromkeys([var.name for var in dataset if var.is_data]))
    datum1d = defaultdict(list)
    datum0d = defaultdict(list)
    for name in names:
        if len(dataset[Data.Value, name].dimensions) == 1:
            datum1d[name].extend([var for var in dataset.subset[name] if var.is_data])
        else:
            datum0d[name].extend([var for var in dataset.subset[name] if var.is_data])

    # 0 - dimensional data
    if datum0d:
        tab = et.SubElement(body, 'table')
        cap = et.SubElement(tab, 'capltion')
        cap.text = '0D Variables:'
        trName = et.SubElement(tab, 'tr')
        trTag = et.SubElement(tab, 'tr')
        trUnit = et.SubElement(tab, 'tr')
        trVal = et.SubElement(tab, 'tr')
        for key, val in datum0d.items():
            append_with_text(trName, 'th', key, attrib={'colspan': str(len(val)), 'style': 'text-align:center'})
            for var in val:
                append_with_text(trTag, 'th', str(var.tag))
                append_with_text(trVal, 'th', str(var.data[0]))
                append_with_text(trUnit, 'th', '[{}]'.format(var.unit))

    # 1 - dimensional data
    if datum1d:
        coords = [var for var in dataset if var.is_coord]
        tab = et.SubElement(body, 'table')
        cap = et.SubElement(tab, 'capltion')
        cap.text = '1D Variables:'
        tr = et.SubElement(tab, 'tr')
        tr.append(et.Element('th', attrib={'colspan': '1'}))

        # Alligned names
        for key, val in datum1d.items():
            append_with_text(tr, 'th', key, attrib={'colspan': str(len(val)), 'style': 'text-align:center'})

        tr = et.SubElement(tab, 'tr')
        for x in coords:
            append_with_text(tr, 'th', str(x.tag))

        datas = []
        for key, val in datum1d.items():
            datas.extend(val)

        for x in datas:
            append_with_text(tr, 'th', str(x.tag))

        tr = et.SubElement(tab, 'tr')
        for x in coords:
            append_with_text(tr, 'th', '[{}]'.format(x.unit))
        for x in datas:
            append_with_text(tr, 'th', '[{}]'.format(x.unit))

        # Data lines
        for i in range(len(coords[0])):
            tr = et.SubElement(tab, 'tr')
            for x in coords:
                append_with_text(tr, 'th', str(x.data[i]))
            for x in datas:
                append_with_text(tr, 'th', str(x.data[i]))

    from IPython.display import display, HTML
    display(HTML(et.tostring(body).decode('UTF-8')))


def table_var(variable):
    if len(variable.dimensions) > 1:
        raise RuntimeError("Only 1-D variable can be rendered")

    body = et.Element('body')
    headline = et.SubElement(body, 'h3')
    headline.text = 'Variable:'
    tab = et.SubElement(body, 'table')

    trName = et.SubElement(tab, 'tr')
    trTag = et.SubElement(tab, 'tr')
    trUnit = et.SubElement(tab, 'tr')
    append_with_text(trName, 'th', variable.name)
    append_with_text(trTag, 'th', str(variable.tag))
    append_with_text(trUnit, 'th', '[{}]'.format(variable.unit))

    # Alligned data
    for val in variable.data:
        trVal = et.SubElement(tab, 'tr')
        append_with_text(trVal, 'th', str(val))

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

