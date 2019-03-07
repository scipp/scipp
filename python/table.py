from xml.etree import ElementTree as et


def append_with_text(parent, name, text, attrib={}):
    el = et.SubElement(parent, name, attrib=attrib)
    el.text = text


def table(dataset):
    if len(dataset.dimensions) != 1:
        raise RuntimeError("Only 1-D datasets can be rendered as a table")

    body = et.Element('body')
    tab = et.SubElement(body,'table')
    cap = et.SubElement(tab,'capltion')
    cap.text = 'Dataset:'
    tr = et.SubElement(tab, 'tr')
    tr.append(et.Element('th', attrib={'colspan': '1'}))
    names = {var.name for var in dataset if var.is_data}
    for x in names:
        append_with_text(tr, 'th', x, attrib={'colspan': '1', 'style': 'text-align:center'})

    tr = et.SubElement(tab, 'tr')
    coords = [var for var in dataset if var.is_coord]
    for x in coords:
        append_with_text(tr, 'th', str(x.tag))

    datas = []
    for name in names:
        datas.extend([var for var in dataset.subset[name] if var.is_data])
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
    display(HTML(et.tostring(tab).decode('UTF-8')))

