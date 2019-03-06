def table(dataset):
    if len(dataset.dimensions) != 1:
        raise RuntimeError("Only 1-D datasets can be rendered as a table")
    s = str()
    s += '<table>'
    s += '<caption>Dataset:</caption>'

    # Header line 1
    names = { var.name for var in dataset if var.is_data }
    s += '<tr>'
    s += '<th colspan="1"></th>'
    for name in names:
        s += '<th colspan="2" style="text-align:center;">{}</th>'.format(name)
    s += '</tr>'

    # Header line 2
    # TODO tags in correct order, first coords, then data
    s += '<tr>'
    coords = [ var for var in dataset if var.is_coord ]
    for coord in coords:
        s += '<th>{}</th>'.format(coord.tag)

    datas = []
    for name in names:
        datas += [ var for var in dataset.subset[name] if var.is_data ]
    for data in datas:
        s += '<th>{}</th>'.format(data.tag)
    s += '</tr>'

    # Header line 3
    s += '<tr>'
    for coord in coords:
        s += '<th>[{}]</th>'.format(coord.unit)
    for data in datas:
        s += '<th>[{}]</th>'.format(data.unit)
    s += '</tr>'

    # Data lines
    for i in range(len(coords[0])):
        s += '<tr>'
        for coord in coords:
            s += '<td>{}</td>'.format(coord.data[i])
        for data in datas:
            s += '<td>{}</td>'.format(data.data[i])
        s += '</tr>'


    s += '</table>'

    from IPython.display import display, HTML
    display(HTML(s))
