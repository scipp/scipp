# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock

from IPython.core.display import display, HTML

import scipp as sc


class Draw():
    def __init__(self, container):
        self._colors = {
            'data': ['dde9af', 'bcd35f', '89a02c'],
            'coord': ['ffe680', 'ffd42a', 'd4aa00'],
            'labels': ['afdde9', '5fbcd3', '2c89a0'],
            'attr': ['ff8080', 'ff2a2a', 'd40000'],
            'inactive': ['cccccc', '888888', '444444']
        }
        self._margin = 1
        self._container = container

    def _draw_box(self, origin_x, origin_y, color):
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
         """.format(*color).replace("origin_x", str(origin_x)).replace(
            "origin_y", str(origin_y))

    def _variance_offset(self):
        shape = self._container.shape
        if len(shape) <= 2:
            depth = 2
        else:
            depth = shape[-3] + 1
        return 0.3 * depth

    def _view_size(self):
        width = 2 * self._margin
        height = 2 * self._margin
        shape = self._container.shape
        if len(shape) == 0:
            width += 1
            height += 1
        if len(shape) > 0:
            width += self._container.shape[-1]
        if len(shape) > 1:
            height += self._container.shape[-2]
        if len(shape) <= 2:
            depth = 1
        else:
            depth = self._container.shape[-3]
        if self._container.has_variances:
            depth += depth + 1
        width += 0.3 * depth
        height += 0.3 * depth
        return [width, height]

    def _draw_array(self, color, offset=[0, 0]):
        shape = self._container.shape
        dx = offset[0]
        dy = offset[1] + 0.3  # extra offset for top face of top row of cubes
        if len(shape) == 0:
            return self._draw_box(dx + self._margin, dy + self._margin, color)
        elif len(shape) == 1:
            svg = ''
            for i in range(shape[0]):
                svg += self._draw_box(dx + i + self._margin, dy + self._margin,
                                      color)
        elif len(shape) == 2:
            svg = ''
            for y in reversed(range(shape[0])):
                for x in range(shape[1]):
                    svg += self._draw_box(dx + x + self._margin,
                                          dy + y + self._margin, color)
        elif len(shape) == 3:
            svg = ''
            for z in range(shape[0]):
                for y in reversed(range(shape[1])):
                    for x in range(shape[2]):
                        svg += self._draw_box(
                            dx + x + self._margin + 0.3 *
                            (shape[0] - z - self._margin),
                            dy + y + self._margin + 0.3 * z, color)
        else:
            return ''
        return svg

    def _draw_labels(self, offset=[0, 0]):
        dims = self._container.dims
        shape = self._container.shape
        view_height = self._view_size()[1]
        svg = ''
        if len(shape) > 0:
            svg += '<text x="{}" y="{}" text-anchor="middle" fill="dim-color" \
                    style="font-size:0.2px">{}</text>'.format(
                self._margin + 0.5 * shape[-1],
                view_height - self._margin + 0.3, dims[-1])
        if len(shape) > 1:
            svg += '<text x="x_pos" y="y_pos" text-anchor="middle" \
                    fill="dim-color" style="font-size:0.2px" \
                    transform="rotate(-90, x_pos, y_pos)">{}</text>'.replace(
                'x_pos', str(self._margin - 0.2)).replace(
                    'y_pos',
                    str(view_height - self._margin - 0.2 -
                        0.5 * shape[-2])).format(dims[-2])
        if len(shape) > 2:
            svg += '<text x="x_pos" y="y_pos" text-anchor="middle" \
                    dominant-baseline="central" \
                    fill="dim-color" style="font-size:0.2px" \
                    transform="rotate(-45, x_pos, y_pos)">{}</text>'.replace(
                'x_pos',
                str(self._margin + 0.3 * 0.5 * shape[-3] - 0.2)).replace(
                    'y_pos',
                    str(view_height - self._margin - shape[-2] -
                        0.3 * 0.5 * shape[-3] - 0.2)).format(dims[-3])
        return svg

    def _draw_info(self):
        return '<text x="{}" y="{}" \
                style="font-size:0.3px">\
                dims={}, shape={}, unit={}, variances={}\
                </text>'.format(0, 0.5,
                                self._container.dims, self._container.shape,
                                str(self._container.unit),
                                self._container.has_variances)

    def _draw_variable(self):
        svg = self._draw_info()
        if self._container.has_variances:
            svg += self._draw_array(color=self._colors['data'],
                                    offset=[self._variance_offset(), 0])
            svg += self._draw_labels(offset=[self._variance_offset(), 0])
            svg += self._draw_array(color=self._colors['data'],
                                    offset=[0, self._variance_offset()])
            svg += self._draw_labels(offset=[0, self._variance_offset()])
        else:
            svg += self._draw_array(color=self._colors['data'])
            svg += self._draw_labels()
        return svg

    def _make_content(self):
        return self._draw_variable()

    def _set_colors(self, svg):
        dim_color = '#444444'
        return svg.replace('dim-color', dim_color)

    def make_svg(self):
        return self._set_colors('<svg viewBox="0 0 {} {}">{}</svg>'.format(
            *self._view_size(), self._make_content()))


def _draw_dataset(dataset):
    return ""


def show(container):
    """
    Produce a graphical representation of a variable or dataset.
    """
    if isinstance(container, sc.Variable) or isinstance(
            container, sc.VariableConstProxy):
        draw = Draw(container)
        #print(draw.make_svg())
        display(HTML(draw.make_svg()))
    else:
        display(HTML(_draw_dataset(container)))
