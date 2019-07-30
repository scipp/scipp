# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock

import scipp as sc


def _draw_box(origin_x, origin_y, colors=['dde9af', 'bcd35f', '89a02c']):
    return """
  <rect
     style="fill:#{};fill-opacity:1;stroke:#000000;stroke-width:0.05;stroke-linejoin:round;stroke-miterlimit:4;stroke-opacity:1"
     id="rect"
     width="1" height="1" x="origin_x" y="origin_y" />
  <path
     style="fill:#{};fill-opacity:1;fill-rule:evenodd;stroke:#000000;stroke-width:0.05;stroke-linecap:butt;stroke-linejoin:round;stroke-miterlimit:4;stroke-opacity:1"
     d="m origin_x origin_y l 0.3 -0.3 h 1 l -0.3 0.3 z"
     id="path1" />
  <path
     style="fill:#{};fill-opacity:1;fill-rule:evenodd;stroke:#000000;stroke-width:0.05;stroke-linecap:butt;stroke-linejoin:round;stroke-miterlimit:4;stroke-opacity:1"
     d="m origin_x origin_y m 1 0 l 0.3 -0.3 v 1 l -0.3 0.3 z"
     id="path2" />
     """.format(*colors).replace("origin_x", str(origin_x)).replace(
        "origin_y", str(origin_y))


def _draw_variable(variable):
    shape = variable.shape
    margin = 1
    view_height = 2 * margin + 1
    if len(shape) == 0:
        svg = '<svg viewBox="0 0 {} {}">'.format(3, view_height)
        svg += _draw_box(1, 1)
    elif len(shape) == 1:
        svg = '<svg viewBox="0 0 {} {}">'.format(shape[0] + 2 * margin,
                                                 view_height)
        for i in range(shape[0]):
            svg += _draw_box(i + margin, margin)
    elif len(shape) == 2:
        view_height = shape[0] + 2 * margin
        svg = '<svg viewBox="0 0 {} {}">'.format(shape[1] + 2 * margin,
                                                 view_height)
        for y in reversed(range(shape[0])):
            for x in range(shape[1]):
                svg += _draw_box(x + margin, y + margin)
    elif len(shape) == 3:
        view_height = shape[1] + 0.3 * shape[0] + 2 * margin
        svg = '<svg viewBox="0 0 {} {}">'.format(
            shape[2] + 0.3 * shape[0] + 2 * margin, view_height)
        for z in range(shape[0]):
            for y in reversed(range(shape[1])):
                for x in range(shape[2]):
                    svg += _draw_box(
                        x + margin + 0.3 * (shape[0] - z - margin),
                        y + margin + 0.3 * z)
    else:
        svg = '<svg viewBox="0 0 {} {}">'.format(shape[0] + 2 * margin, 3)

    if len(shape) > 0:
        svg += '<text x="{}" y="{}" text-anchor="middle" fill="dim-color" \
                style="font-size:0.2px">{}</text>'.format(
            margin + 0.5 * shape[-1], view_height - margin + 0.1,
            variable.dims[-1])
    if len(shape) > 1:
        svg += '<text x="x_pos" y="y_pos" text-anchor="middle" \
                fill="dim-color" style="font-size:0.2px" \
                transform="rotate(-90, x_pos, y_pos)">{}</text>'.replace(
            'x_pos', str(margin - 0.2)).replace(
                'y_pos', str(view_height - margin - 0.2 -
                             0.5 * shape[-2])).format(variable.dims[-2])
    if len(shape) > 2:
        svg += '<text x="x_pos" y="y_pos" text-anchor="middle" \
                fill="dim-color" style="font-size:0.2px" \
                transform="rotate(-45, x_pos, y_pos)">{}</text>'.replace(
            'x_pos', str(margin + 0.3 * 0.5 * shape[-3] - 0.1)).replace(
                'y_pos', str(margin + 0.3 * 0.5 * shape[-3] - 0.4)).format(
                    variable.dims[-3])
    svg += '</svg>'
    dim_color = '#444444'
    svg = svg.replace('dim-color', dim_color)
    return svg


def _draw_dataset(dataset):
    return ""


def show(container):
    """
    Produce a graphical representation of a variable or dataset.
    """
    if isinstance(container, sc.Variable) or isinstance(
            container, sc.VariableConstProxy):
        print(_draw_variable(container))
    else:
        print(_draw_dataset(container))


# setattr(sc.Variable, '_repr_html_', _draw_variable)
